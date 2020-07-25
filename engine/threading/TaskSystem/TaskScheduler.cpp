// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "threading/TaskSystem/TaskScheduler.h"
#include "stl/Algorithms/StringUtils.h"
#include "threading/Containers/LfIndexedPool2.h"

namespace AE::Threading
{
DEBUG_ONLY(
	Atomic<int64_t>	asyncTaskCounter {0};
)

/*
=================================================
	OutputChunk::Init
=================================================
*/
	void  IAsyncTask::OutputChunk::Init (uint idx)
	{
		next		= null;
		selfIndex	= idx;
		tasks.clear();
	}
	
/*
=================================================
	OutputChunk::_GetPool
=================================================
*/
	inline auto&  IAsyncTask::OutputChunk::_GetPool ()
	{
		static LfIndexedPool2< OutputChunk, uint, (1u<<12), 64 >  pool;
		return pool;
	}
//-----------------------------------------------------------------------------


/*
=================================================
	constructor
=================================================
*/
	IAsyncTask::IAsyncTask (EThread type) :
		_threadType{type}
	{
		DEBUG_ONLY( ++asyncTaskCounter );
	}
	
/*
=================================================
	destructor
=================================================
*/
	IAsyncTask::~IAsyncTask ()
	{
		ASSERT( _output == null );
		DEBUG_ONLY( --asyncTaskCounter );
	}

/*
=================================================
	_SetCancellationState
=================================================
*/
	bool  IAsyncTask::_SetCancellationState ()
	{
		for (EStatus expected = EStatus::Pending;
			 not _status.compare_exchange_weak( INOUT expected, EStatus::Cancellation, EMemoryOrder::Relaxed );)
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
	bool  IAsyncTask::OnFailure ()
	{
		for (EStatus expected = EStatus::InProgress;
			 not _status.compare_exchange_weak( INOUT expected, EStatus::Failed, EMemoryOrder::Relaxed );)
		{
			// status has been changed in another thread
			if ( expected > EStatus::_Finished )
				return false;

			ASSERT( expected != EStatus::Cancellation );	// TODO: Failed or Canceled ?
		}
		
		EXLOCK( _outputGuard );
		_FreeOutputChunks( true );
		return true;
	}

/*
=================================================
	_OnFinish
=================================================
*/
	void  IAsyncTask::_OnFinish ()
	{
		EStatus	expected = EStatus::InProgress;
	
		EXLOCK( _outputGuard );
		ASSERT( _waitBits.load() == 0 );
		
		// try to set completed state	
		if ( _status.compare_exchange_strong( INOUT expected, EStatus::Completed, EMemoryOrder::Relaxed ) or expected == EStatus::Failed )
		{
			_FreeOutputChunks( false );
			return;
		}

		// cancel
		if ( expected == EStatus::Cancellation )
		{
			OnCancel();

			_status.store( EStatus::Canceled, EMemoryOrder::Relaxed );

			_FreeOutputChunks( true );
			return;
		}

		ASSERT(!"unknown state");
	}
	
/*
=================================================
	_Cancel
=================================================
*/
	void  IAsyncTask::_Cancel ()
	{
		EXLOCK( _outputGuard );

		// Pending/InProgress -> Cancellation
		_SetCancellationState();

		OnCancel();

		// set canceled state
		CHECK( _status.exchange( EStatus::Canceled, EMemoryOrder::Relaxed ) == EStatus::Cancellation );

		_FreeOutputChunks( true );
	}
	
/*
=================================================
	_FreeOutputChunks
=================================================
*/
	void  IAsyncTask::_FreeOutputChunks (bool isCanceled)
	{
		auto&	chunk_pool = OutputChunk::_GetPool();

		for (OutputChunk* chunk = _output; chunk;)
		{
			for (auto&[dep, idx, is_strong] : chunk->tasks)
			{
				if ( isCanceled and is_strong )
					dep->_canceledDepsCount.fetch_add( 1, EMemoryOrder::Relaxed );

				dep->_waitBits.fetch_and( ~(1ull << idx), EMemoryOrder::Relaxed );
			}
			chunk->tasks.clear();

			OutputChunk*	old_chunk = chunk;

			chunk			= old_chunk->next;
			old_chunk->next = null;
			chunk_pool.Unassign( old_chunk->selfIndex );
		}

		_output = null;
	}
	
/*
=================================================
	_ResetState
=================================================
*/
	bool  IAsyncTask::_ResetState ()
	{
		EXLOCK( _outputGuard );

		for (EStatus expected = EStatus::Completed;
			 not _status.compare_exchange_weak( INOUT expected, EStatus::Initial, EMemoryOrder::Relaxed );)
		{
			if ( expected == EStatus::Completed	or
				 expected == EStatus::Failed	or
				 expected == EStatus::Canceled )
				continue;

			RETURN_ERR( "can't reset task that is not finished" );
		}

		_waitBits.store( UMax, EMemoryOrder::Relaxed );
		_canceledDepsCount.store( 0, EMemoryOrder::Relaxed );
		ASSERT( _output == null );

		ASSERT( not _interlockDep );
		_interlockDep.Clear();

		return true;
	}
//-----------------------------------------------------------------------------
	

/*
=================================================
	_RunTask
=================================================
*/
	void  IThread::_RunTask (const AsyncTask &task)
	{
		using EStatus = IAsyncTask::EStatus;

		AE_VTUNE( __itt_task_begin( Scheduler().GetVTuneDomain(), __itt_null, __itt_null, __itt_string_handle_createA( task->DbgName().c_str() )));
		task->Run();
		task->_OnFinish();
		AE_VTUNE( __itt_task_end( Scheduler().GetVTuneDomain() ));
		
		CHECK( task->_interlockDep.Unlock() );
	}
	
/*
=================================================
	_OnTaskFinish
=================================================
*/
	void  IThread::_OnTaskFinish (const AsyncTask &task)
	{
		task->_OnFinish();

		CHECK( task->_interlockDep.Unlock() );
	}
//-----------------------------------------------------------------------------
	

/*
=================================================
	_SetDependencyCompletionStatus
=================================================
*/
	void  ITaskDependencyManager::_SetDependencyCompletionStatus (const AsyncTask &task, uint depIndex, bool cancel)
	{
		if ( cancel )
			task->_canceledDepsCount.fetch_add( 1, EMemoryOrder::Relaxed );

		task->_waitBits.fetch_and( ~(1ull << depIndex), EMemoryOrder::Relaxed );
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
	_Instance
=================================================
*/
	TaskScheduler*  TaskScheduler::_Instance ()
	{
		static std::aligned_storage_t< sizeof(TaskScheduler), alignof(TaskScheduler) >	scheduler;
		return Cast<TaskScheduler>( &scheduler );
	}
	
/*
=================================================
	CreateInstance
=================================================
*/
	void  TaskScheduler::CreateInstance ()
	{
		new( _Instance() ) TaskScheduler{};
		ThreadFence( EMemoryOrder::Release );
	}
	
/*
=================================================
	DestroyInstance
=================================================
*/
	void  TaskScheduler::DestroyInstance ()
	{
		ThreadFence( EMemoryOrder::Acquire );
		_Instance()->~TaskScheduler();
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
		// you must call 'Release' before destroy
		EXLOCK( _threadGuard );
		CHECK( _threads.empty() );
		ASSERT( asyncTaskCounter == 0 );
	}
	
/*
=================================================
	Setup
=================================================
*/
	bool  TaskScheduler::Setup (size_t maxWorkerThreads)
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
	void  TaskScheduler::Release ()
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

		// free dependency managers
		{
			EXLOCK( _taskDepsMngrsGuard );
			_taskDepsMngrs.clear();
		}
		
		ASSERT( asyncTaskCounter == 0 );
		
		auto&	chunk_pool = OutputChunk_t::_GetPool();
		chunk_pool.Release();
	}

/*
=================================================
	AddThread
=================================================
*/
	bool  TaskScheduler::AddThread (const ThreadPtr &thread)
	{
		EXLOCK( _threadGuard );

		CHECK_ERR( thread );
		CHECK_ERR( thread->Attach( uint(_threads.size()) ));
		
		ASSERT( _threads.size() <= std::thread::hardware_concurrency() );

		_threads.push_back( thread );
		return true;
	}
	
/*
=================================================
	ProcessTask
=================================================
*/
	bool  TaskScheduler::ProcessTask (EThread type, uint seed)
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
				auto&	curr  = *iter;
				bool	ready = false;

				// check input dependencies
				if ( curr->_canceledDepsCount.load( EMemoryOrder::Relaxed ) > 0 or
					 curr->_waitBits.load( EMemoryOrder::Relaxed ) == 0 )
				{
					ready = true;
				}

				// input dependencies is not complete
				if ( not ready )
				{
					++iter;
					continue;
				}

				// try lock
				if ( curr->_interlockDep and not curr->_interlockDep.TryLock() )
				{
					// failed to lock
					continue;
				}

				// remove task
				task = curr;
				q.tasks.erase( iter );
				break;
			}

			// additionaly this operation flushes and invalidates cache,
			// so you don't need to invalidate cache in 'IAsyncTask::Run()' method
			q.guard.unlock();

			if ( not task )
				continue;
			
			// cancel task if one of dependencies has been canceled
			if ( (task->Status() == EStatus::Cancellation) or
				 (task->_canceledDepsCount.load( EMemoryOrder::Relaxed ) > 0) )
			{
				task->_Cancel();
				--j; // try same queue
				continue;
			}

			// try to start task
			EStatus	expected = EStatus::Pending;
			
			if ( task->_status.compare_exchange_strong( INOUT expected, EStatus::InProgress, EMemoryOrder::Relaxed ))
			{
				AE_SCHEDULER_PROFILING(
					tq._stallTime += (TimePoint_t::clock::now() - start_time).count();
				)
				return task;
			}
			else
			{
				ASSERT( expected == EStatus::Cancellation );
				
				CHECK( task->_interlockDep.Unlock() );
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
				
			CHECK( task->_interlockDep.Unlock() );

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
	bool  TaskScheduler::Wait (ArrayView<AsyncTask> tasks, nanoseconds timeout)
	{
		const auto	start_time	= TimePoint_t::clock::now();

		for (auto& task : tasks)
		{
			for (uint i = 0; task->Status() < EStatus::_Finished; ++i)
			{
				if ( i > 2000 )
				{
					std::this_thread::sleep_for( microseconds{100} );
					i = 0;

					const nanoseconds	dt = TimePoint_t::clock::now() - start_time;

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
	_AddTaskDependencies
=================================================
*/
	bool  TaskScheduler::_AddTaskDependencies (const AsyncTask &task, const AsyncTask &dep, bool isStrong, INOUT uint &bitIndex)
	{
		if ( not dep )
			return true;

		CHECK_ERR( bitIndex + 1 < sizeof(IAsyncTask::WaitBits_t)*8 );

		EXLOCK( dep->_outputGuard );	// TODO: optimize

		// if we have lock and task is not finished then we can add output dependency
		// even if status has been changed in another thread
		const EStatus	status	= dep->Status();

		// add to output
		if ( status < EStatus::_Finished )
		{
			for (OutputChunk_t** chunk_ref = &dep->_output;;)
			{
				if ( not *chunk_ref )
				{
					auto&	chunk_pool = OutputChunk_t::_GetPool();

					uint	idx;
					CHECK_ERR( chunk_pool.Assign( OUT idx ));

					*chunk_ref = &chunk_pool[ idx ];
					(*chunk_ref)->Init( idx );
				}

				if ( (*chunk_ref)->next )
				{
					chunk_ref = &(*chunk_ref)->next;
					continue;
				}

				if ( (*chunk_ref)->tasks.try_push_back({ task, bitIndex, uint(isStrong) }))
				{
					++bitIndex;
					break;
				}

				chunk_ref = &(*chunk_ref)->next;
			}
		}

		// cancel current task
		if ( isStrong and status > EStatus::_Interropted )
		{
			task->_Cancel();
			return true;
		}

		return true;
	}
	
/*
=================================================
	_SetInterlockDependency
=================================================
*/
	bool  TaskScheduler::_SetInterlockDependency (const AsyncTask &task, const InterlockDependency &dep)
	{
		CHECK_ERR( not task->_interlockDep );

		task->_interlockDep = dep;
		return true;
	}

/*
=================================================
	_InsertTask
=================================================
*/
	AsyncTask  TaskScheduler::_InsertTask (const AsyncTask &task, uint bitIndex)
	{
		// some dependencies may complete so merge bit mask with current
		task->_waitBits.fetch_and( ToBitMask<uint64_t>( bitIndex ), EMemoryOrder::Relaxed );

		EStatus  old_status = task->_status.exchange( EStatus::Pending, EMemoryOrder::Relaxed );
		CHECK_ERR( old_status == EStatus::Initial );

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
				<< " queue total work: " << ToString( nanoseconds(work_time) )
				<< ", stall: " << ToString( factor * 100.0, 2 ) << " %"
				<< ", queue count: " << ToString( tq.queues.size() ) );
		)
	}


}	// AE::Threading
