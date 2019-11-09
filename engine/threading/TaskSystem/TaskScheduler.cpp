// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "threading/TaskSystem/TaskScheduler.h"
#include "stl/Algorithms/StringUtils.h"

namespace AE::Threading
{
	
/*
=================================================
	constructor
=================================================
*/
	IAsyncTask::IAsyncTask (EThread type) :
		_threadType{type}
	{}
	
/*
=================================================
	_SetCancellationState
=================================================
*/
	bool IAsyncTask::_SetCancellationState ()
	{
		for (EStatus expected = EStatus::Pending;
			 not _status.compare_exchange_weak( INOUT expected, EStatus::Cancellation, memory_order_relaxed );)
		{
			// status has been changed in another thread
			if ( (expected == EStatus::Cancellation) | (expected > EStatus::_Finished) )
				return false;
		}
		return true;
	}

/*
=================================================
	OnFailure
=================================================
*/
	bool IAsyncTask::OnFailure ()
	{
		for (EStatus expected = EStatus::InProgress;
			 not _status.compare_exchange_weak( INOUT expected, EStatus::Failed, memory_order_relaxed );)
		{
			// status has been changed in another thread
			if ( expected > EStatus::_Finished )
				return false;

			ASSERT( expected != EStatus::Cancellation );	// TODO: Failed or Canceled ?
		}
		return true;
	}

/*
=================================================
	_OnFinish
=================================================
*/
	void IAsyncTask::_OnFinish ()
	{
		EStatus	expected = EStatus::InProgress;
	
		// try to set completed state	
		if ( _status.compare_exchange_strong( INOUT expected, EStatus::Completed, memory_order_relaxed ) or expected == EStatus::Failed )
		{
			// flush cache
			ThreadFence( memory_order_release );
			return;
		}

		if ( expected == EStatus::Cancellation )
		{
			OnCancel();

			// set canceled state and flush cache
			_status.compare_exchange_strong( INOUT expected, EStatus::Canceled, memory_order_release, memory_order_relaxed );
			return;
		}

		ASSERT(!"unknown state");
	}
	
/*
=================================================
	_Cancel
=================================================
*/
	void IAsyncTask::_Cancel ()
	{
		// Pending/InProgress -> Cancellation
		_SetCancellationState();

		OnCancel();

		// set canceled state and flush cache
		CHECK( _status.exchange( EStatus::Canceled, memory_order_release ) == EStatus::Cancellation );
	}
//-----------------------------------------------------------------------------
	

/*
=================================================
	_RunTask
=================================================
*/
	void IThread::_RunTask (const AsyncTask &task)
	{
		using EStatus = IAsyncTask::EStatus;

		AE_VTUNE( __itt_task_begin( Scheduler().GetVTuneDomain(), __itt_null, __itt_null, __itt_string_handle_createA( task->DbgName().c_str() )));
		task->Run();
		task->_OnFinish();
		AE_VTUNE( __itt_task_end( Scheduler().GetVTuneDomain() ));
	}
	
/*
=================================================
	_OnTaskFinish
=================================================
*/
	void IThread::_OnTaskFinish (const AsyncTask &task)
	{
		task->_OnFinish();
	}
//-----------------------------------------------------------------------------


/*
=================================================
	constructor
=================================================
*/
	template <size_t N>
	TaskScheduler::_TaskQueue<N>::_TaskQueue ()
	{
		STATIC_ASSERT( N >= 2 );
		queues.resize( 2 );
	}
	
/*
=================================================
	Resize
=================================================
*/
	template <size_t N>
	void  TaskScheduler::_TaskQueue<N>::Resize (size_t count)
	{
		ASSERT( count >= 2 );
		queues.resize( count );
	}
//-----------------------------------------------------------------------------

	
/*
=================================================
	Instance
=================================================
*/
	TaskScheduler&  TaskScheduler::Instance ()
	{
		static TaskScheduler	scheduler;
		return scheduler;
	}

/*
=================================================
	constructor
=================================================
*/
	TaskScheduler::TaskScheduler ()
	{
		AE_VTUNE( _vtuneDomain = __itt_domain_create( "AE.TaskScheduler" ));
	}
	
/*
=================================================
	destructor
=================================================
*/
	TaskScheduler::~TaskScheduler ()
	{
		// call 'Release' before destroy
		EXLOCK( _threadGuard );
		CHECK( _threads.empty() );
	}
	
/*
=================================================
	Setup
=================================================
*/
	bool TaskScheduler::Setup (size_t maxWorkerThreads)
	{
		{
			EXLOCK( _threadGuard );
			CHECK_ERR( _threads.empty() );
		}

		_mainQueue.Resize( 2 );
		_workerQueue.Resize( Max( 2u, (maxWorkerThreads + 2) / 3 ));
		_renderQueue.Resize( 2 );
		_fileQueue.Resize( 2 );
		_networkQueue.Resize( 2 );

		return true;
	}
	
/*
=================================================
	Release
=================================================
*/
	void TaskScheduler::Release ()
	{
		// detach threads
		{
			EXLOCK( _threadGuard );

			for (auto& thread : _threads) {
				thread->Detach();
			}
			_threads.clear();
		}

		_WriteProfilerStat( "main",    _mainQueue    );
		_WriteProfilerStat( "worker",  _workerQueue  );
		_WriteProfilerStat( "render",  _renderQueue  );
		_WriteProfilerStat( "file",    _fileQueue    );
		//_WriteProfilerStat( "network", _networkQueue );
		
		AE_VTUNE(
			if ( _vtuneDomain )
				_vtuneDomain->flags = 0;
		)
	}

/*
=================================================
	AddThread
=================================================
*/
	bool TaskScheduler::AddThread (const ThreadPtr &thread)
	{
		EXLOCK( _threadGuard );

		CHECK_ERR( thread );
		CHECK_ERR( thread->Attach( uint(_threads.size()) ));
		
		_threads.push_back( thread );
		return true;
	}
	
/*
=================================================
	ProcessTask
=================================================
*/
	bool TaskScheduler::ProcessTask (EThread type, uint seed)
	{
		BEGIN_ENUM_CHECKS();
		switch ( type )
		{
			case EThread::Main :		return _ProcessTask( _mainQueue, seed );
			case EThread::Worker :		return _ProcessTask( _workerQueue, seed );
			case EThread::Renderer :	return _ProcessTask( _renderQueue, seed );
			case EThread::FileIO :		return _ProcessTask( _fileQueue, seed );
			case EThread::Network :		return _ProcessTask( _networkQueue, seed );
			case EThread::_Count :		break;
		}
		END_ENUM_CHECKS();
		RETURN_ERR( "not supported" );
	}
	
/*
=================================================
	PullTask
=================================================
*/
	AsyncTask  TaskScheduler::PullTask (EThread type, uint seed)
	{
		BEGIN_ENUM_CHECKS();
		switch ( type )
		{
			case EThread::Main :		return _PullTask( _mainQueue, seed );
			case EThread::Worker :		return _PullTask( _workerQueue, seed );
			case EThread::Renderer :	return _PullTask( _renderQueue, seed );
			case EThread::FileIO :		return _PullTask( _fileQueue, seed );
			case EThread::Network :		return _PullTask( _networkQueue, seed );
			case EThread::_Count :		break;
		}
		END_ENUM_CHECKS();
		RETURN_ERR( "not supported" );
	}

/*
=================================================
	_PullTask
=================================================
*/
	template <size_t N>
	AsyncTask  TaskScheduler::_PullTask (_TaskQueue<N> &tq, uint seed) const
	{
		AE_SCHEDULER_PROFILING(
			const auto	start_time = TimePoint_t::clock::now();
		)

		for (size_t j = 0; j < tq.queues.size(); ++j)
		{
			auto&	q = tq.queues[ (j + seed) % tq.queues.size() ];

			if ( not q.guard.try_lock() )
				continue;

			AsyncTask	task;

			// find task
			for (auto iter = q.tasks.begin(); iter != q.tasks.end();)
			{
				bool	ready	= true;
				bool	cancel	= ((*iter)->Status() == EStatus::Cancellation);

				// check input dependencies
				for (size_t i = 0, cnt = (*iter)->_dependsOn.size(); (i < cnt) & ready & (not cancel); ++i)
				{
					const auto&	dep = (*iter)->_dependsOn[i];
					
					if ( not dep )
						continue;

					const EStatus	status = dep->Status();

					ready	&= (status > EStatus::_Finished);
					cancel	|= (status > EStatus::_Interropted);
				}
				
				cancel &= (*iter)->_canBeCanceledByDeps;

				if ( ready | cancel ) {
					(*iter)->_dependsOn.clear();
				}

				// cancel task if one of dependencies has been canceled
				if ( cancel )
				{
					(*iter)->_Cancel();
					iter = q.tasks.erase( iter );
					continue;
				}

				// input dependencies is not complete
				if ( not ready )
				{
					++iter;
					continue;
				}

				// remove task
				task = *iter;
				q.tasks.erase( iter );
				break;
			}

			// additionaly this operation flushes and invalidates cache,
			// so you don't need to invalidate cache in 'IAsyncTask::Run()' method
			q.guard.unlock();

			if ( not task )
				continue;

			// try to start task
			EStatus	expected = EStatus::Pending;
			
			if ( task->_status.compare_exchange_strong( INOUT expected, EStatus::InProgress, memory_order_relaxed ))
			{
				AE_SCHEDULER_PROFILING(
					tq._stallTime += (TimePoint_t::clock::now() - start_time).count();
				)
				return task;
			}
			else
			{
				ASSERT( expected == EStatus::Cancellation );
				task->_Cancel();
			}
		}

		AE_SCHEDULER_PROFILING(
			tq._stallTime += (TimePoint_t::clock::now() - start_time).count();
		)
		return null;
	}
	
/*
=================================================
	_ProcessTask
=================================================
*/
	template <size_t N>
	bool  TaskScheduler::_ProcessTask (_TaskQueue<N> &tq, uint seed) const
	{
		if ( AsyncTask task = _PullTask( tq, seed ))
		{
			AE_SCHEDULER_PROFILING(
				const auto	start_time = TimePoint_t::clock::now();
			)

			AE_VTUNE( __itt_task_begin( _vtuneDomain, __itt_null, __itt_null, __itt_string_handle_createA( task->DbgName().c_str() )));
			task->Run();
			task->_OnFinish();
			AE_VTUNE( __itt_task_end( _vtuneDomain ));
				
			AE_SCHEDULER_PROFILING(
				tq._workTime += (TimePoint_t::clock::now() - start_time).count();
			)
			return true;
		}
		return false;
	}

/*
=================================================
	Wait
----
	Warning: deadlock may occur if wait() called in all threads,
	use it only for debugging and testing
=================================================
*/
	bool  TaskScheduler::Wait (ArrayView<AsyncTask> tasks, Nanoseconds timeout)
	{
		const auto	start_time	= TimePoint_t::clock::now();

		for (auto& task : tasks)
		{
			for (uint i = 0; task->Status() < EStatus::_Finished; ++i)
			{
				if ( i > 2000 )
				{
					std::this_thread::yield();
					i = 0;

					const Nanoseconds	dt = TimePoint_t::clock::now() - start_time;

					if ( dt >= timeout )
						return false;	// time out
				}
			}
		}
		return true;
	}
	
/*
=================================================
	Cancel
=================================================
*/
	bool  TaskScheduler::Cancel (const AsyncTask &task)
	{
		if ( not task )
			return false;

		return task->_SetCancellationState();
	}
	
/*
=================================================
	_InsertTask
=================================================
*/
	AsyncTask  TaskScheduler::_InsertTask (const AsyncTask &task, Array<AsyncTask> &&dependsOn, bool canBeCanceledByDeps)
	{
		task->_canBeCanceledByDeps	= canBeCanceledByDeps;
		task->_dependsOn			= std::move(dependsOn);

		BEGIN_ENUM_CHECKS();
		switch ( task->Type() )
		{
			case EThread::Main :		_AddTask( _mainQueue,    task );	break;
			case EThread::Worker :		_AddTask( _workerQueue,  task );	break;
			case EThread::Renderer :	_AddTask( _renderQueue,  task );	break;
			case EThread::FileIO :		_AddTask( _fileQueue,    task );	break;
			case EThread::Network :		_AddTask( _networkQueue, task );	break;
			case EThread::_Count :
			default :					RETURN_ERR( "not supported" );
		}
		END_ENUM_CHECKS();
		return task;
	}

/*
=================================================
	_AddTask
=================================================
*/
	template <size_t N>
	void  TaskScheduler::_AddTask (_TaskQueue<N> &tq, const AsyncTask &task) const
	{
		AE_SCHEDULER_PROFILING(
			const auto	start_time = TimePoint_t::clock::now();
		)

		const size_t	seed = size_t(HashOf( std::this_thread::get_id() ));

		for (;;)
		{
			for (size_t j = 0; j < tq.queues.size(); ++j)
			{
				auto&	q = tq.queues[ (j + seed) % tq.queues.size() ];
			
				if ( q.guard.try_lock() )
				{
					q.tasks.push_back( task );
					q.guard.unlock();
					
					AE_SCHEDULER_PROFILING(
						tq._insertionTime += (TimePoint_t::clock::now() - start_time).count();
					)
					return;
				}
			}
		}
	}
	
/*
=================================================
	_WriteProfilerStat
=================================================
*/
	template <size_t N>
	void  TaskScheduler::_WriteProfilerStat (StringView name, const _TaskQueue<N> &tq)
	{
		AE_SCHEDULER_PROFILING(
			auto	work_time	= tq._workTime.load();
			auto	stall_time	= tq._stallTime.load();
			auto	insert_time	= tq._insertionTime.load();

			if ( work_time == 0 and stall_time == 0 )
				return;

			double	stall	= double(stall_time) + double(insert_time);
			double	work	= double(work_time);
			double	factor	= (work_time ? stall / (stall + work) : 1.0);

			AE_LOGI( String(name)
				<< " queue total work: " << ToString( Nanoseconds(work_time) )
				<< ", stall: " << ToString( factor * 100.0, 2 ) << " %"
				<< ", queue count: " << ToString( tq.queues.size() ) );
		)
	}


}	// AE::Threading
