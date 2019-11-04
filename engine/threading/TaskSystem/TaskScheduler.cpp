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
	_ForceCanceledState
=================================================
*/
	bool IAsyncTask::_ForceCanceledState ()
	{
		EStatus	old = _status.exchange( EStatus::Canceled, memory_order_relaxed );
		return old < EStatus::_Finished;
	}
	
/*
=================================================
	_SetCanceledState
=================================================
*/
	bool IAsyncTask::_SetCanceledState ()
	{
		for (EStatus expected = EStatus::Pending;
			 not _status.compare_exchange_weak( INOUT expected, EStatus::Canceled, memory_order_relaxed );)
		{
			// status has been changed in another thread
			if ( expected > EStatus::_Finished )
				return false;
		}
		return true;
	}

/*
=================================================
	_SetFailedState
=================================================
*/
	bool IAsyncTask::_SetFailedState ()
	{
		for (EStatus expected = EStatus::Pending;
			 not _status.compare_exchange_weak( INOUT expected, EStatus::Failed, memory_order_relaxed );)
		{
			// status has been changed in another thread
			if ( expected > EStatus::_Finished )
				return false;
		}
		return true;
	}
	
/*
=================================================
	_SetCompletedState
=================================================
*/
	bool IAsyncTask::_SetCompletedState ()
	{
		EStatus	expected = EStatus::InProgress;
		bool	result   = _status.compare_exchange_strong( INOUT expected, EStatus::Completed, memory_order_relaxed, memory_order_relaxed );

		ASSERT( expected == EStatus::InProgress or expected == EStatus::Canceled );
		return result;
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
		AE_VTUNE( __itt_task_end( Scheduler().GetVTuneDomain() ));

		task->_SetCompletedState();
		ThreadFence( memory_order_release );
	}
	
/*
=================================================
	_RunTask
=================================================
*/
	void IThread::_SetFailedState (const AsyncTask &task)
	{
		task->_SetFailedState();
		ThreadFence( memory_order_release );
	}
	
/*
=================================================
	_RunTask
=================================================
*/
	void IThread::_SetCompletedState (const AsyncTask &task)
	{
		task->_SetCompletedState();
		ThreadFence( memory_order_release );
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
		CHECK( _threads.empty() );
	}
	
/*
=================================================
	Setup
=================================================
*/
	bool TaskScheduler::Setup (size_t maxWorkerThreads)
	{
		CHECK_ERR( _threads.empty() );

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
		for (auto& thread : _threads) {
			thread->Detach();
		}
		_threads.clear();

		_WriteProfilerStat( "main",    _mainQueue    );
		_WriteProfilerStat( "worker",  _workerQueue  );
		_WriteProfilerStat( "render",  _renderQueue  );
		_WriteProfilerStat( "file",    _fileQueue    );
		_WriteProfilerStat( "network", _networkQueue );
		
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
		Unused( seed );	// TODO

		AE_SCHEDULER_PROFILING(
			const auto	start_time = TimePoint_t::clock::now();
		)

		for (auto& q : tq.queues)
		{
			if ( not q.guard.try_lock() )
				continue;

			AsyncTask	task;

			// find task
			for (auto iter = q.tasks.begin(); iter != q.tasks.end();)
			{
				bool	ready	= true;
				bool	cancel	= ((*iter)->Status() > EStatus::_Interropted);

				// check input dependencies
				for (size_t i = 0, cnt = (*iter)->_dependsOn.size(); (i < cnt) & ready & (not cancel); ++i)
				{
					const auto&		dep		= (*iter)->_dependsOn[i];
					const EStatus	status	= dep->Status();

					ready	&= (status == EStatus::Completed);
					cancel	|= (status >  EStatus::_Interropted);
				}
				
				if ( ready or cancel ) {
					(*iter)->_dependsOn.clear();
				}

				// cancel task if one of dependencies has been canceled
				if ( cancel )
				{
					(*iter)->_ForceCanceledState();
					(*iter)->OnCancel();
					ThreadFence( memory_order_release );

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

			q.guard.unlock();

			if ( not task )
				continue;

			// try to start task
			EStatus	expected = EStatus::Pending;
			
			if ( task->_status.compare_exchange_strong( INOUT expected, EStatus::InProgress, memory_order_relaxed ) )
			{
				AE_SCHEDULER_PROFILING(
					tq._stallTime += (TimePoint_t::clock::now() - start_time).count();
				)
				return task;
			}
			else
			{
				task->_ForceCanceledState();
				task->OnCancel();
				ThreadFence( memory_order_release );
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
			AE_VTUNE( __itt_task_end( _vtuneDomain ));

			task->_SetCompletedState();
			ThreadFence( memory_order_release );
				
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

		return task->_SetCanceledState();
	}
	
/*
=================================================
	_InsertTask
=================================================
*/
	AsyncTask  TaskScheduler::_InsertTask (const AsyncTask &task, Array<AsyncTask> &&dependsOn)
	{
		task->_dependsOn = std::move(dependsOn);

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

		for (;;)
		{
			for (auto& q : tq.queues)
			{
				if ( q.guard.try_lock() )
				{
					q.tasks.push_back( task );
					q.guard.unlock();
					
					AE_SCHEDULER_PROFILING(
						tq._stallTime += (TimePoint_t::clock::now() - start_time).count();
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

			if ( work_time == 0 and stall_time == 0 )
				return;

			double	stall	= double(stall_time);
			double	work	= double(work_time);
			double	factor	= (work_time ? stall / (stall + work) : 1.0);

			AE_LOGI( String(name)
				<< " queue total work: " << ToString( Nanoseconds(work_time) )
				<< ", stall: " << ToString( factor * 100.0, 2 ) << " %"
				<< ", queue count: " << ToString( tq.queues.size() ) );
		)
	}


}	// AE::Threading
