// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "threading/TaskScheduler.h"

namespace AE::Threading
{
	
/*
=================================================
	constructor
=================================================
*/
	IAsyncTask::IAsyncTask (EThread type, Array<AsyncTask> &&deps) :
		_threadType{type}, _dependsOn{std::move(deps)}
	{}
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
	constructor
=================================================
*/
	TaskScheduler::TaskScheduler ()
	{
	}
	
/*
=================================================
	destructor
=================================================
*/
	TaskScheduler::~TaskScheduler ()
	{
		// detach threads
		for (auto& thread : _threads) {
			thread->Detach();
		}
		_threads.clear();
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
		_workerQueue.Resize( (maxWorkerThreads + 2) / 3 );
		_renderQueue.Resize( 2 );
		_fileQueue.Resize( 2 );
		_networkQueue.Resize( 2 );

		return true;
	}

/*
=================================================
	AddThread
=================================================
*/
	bool TaskScheduler::AddThread (const ThreadPtr &thread)
	{
		CHECK_ERR( thread );
		CHECK_ERR( thread->Attach( this, uint(_threads.size()) ));
		
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
	_ProcessTask
=================================================
*/
	template <size_t N>
	bool  TaskScheduler::_ProcessTask (_TaskQueue<N> &tq, uint seed)
	{
		AE_UNUSED( seed );	// TODO

		for (auto& q : tq.queues)
		{
			if ( not q.guard.try_lock() )
				continue;

			AsyncTask	task;

			// find task
			for (auto iter = q.tasks.begin(); iter != q.tasks.end();)
			{
				bool	ready	= true;
				bool	cancel	= ((*iter)->Status() == EStatus::Canceled);

				// check input dependencies
				for (size_t i = 0, cnt = (*iter)->_dependsOn.size(); (i < cnt) & ready & (not cancel); ++i)
				{
					const auto&		dep		= (*iter)->_dependsOn[i];
					const EStatus	status	= dep->Status();

					ready	&= (status == EStatus::Complete);
					cancel	|= (status == EStatus::Canceled);
				}

				// cancel task if one of dependencies has been canceled
				if ( cancel )
				{
					(*iter)->_status.store( EStatus::Canceled, memory_order_release );
					(*iter)->Cancel();

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
				iter = q.tasks.erase( iter );
				break;
			}

			q.guard.unlock();

			if ( not task )
				continue;

			// run
			EStatus	expected = EStatus::Pending;
			
			if ( task->_status.compare_exchange_strong( INOUT expected, EStatus::InProgress, memory_order_relaxed ) )
			{
				task->Run();

				expected = EStatus::InProgress;
				task->_status.compare_exchange_strong( INOUT expected, EStatus::Complete, memory_order_relaxed );
				ASSERT( expected == EStatus::InProgress or expected == EStatus::Canceled );

				return true;
			}
			else
			{
				task->_status.store( EStatus::Canceled, memory_order_release );
				task->Cancel();
			}
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

		// set new status and flush cache
		for (EStatus expected = EStatus::Pending;
			 not task->_status.compare_exchange_weak( INOUT expected, EStatus::Canceled, memory_order_release, memory_order_relaxed );)
		{
			// status has been changed in another thread
			if ( expected > EStatus::_Finished )
				return false;

			ASSERT( expected < EStatus::_Finished );
		}

		return true;
	}
	
/*
=================================================
	_InsertTask
=================================================
*/
	AsyncTask  TaskScheduler::_InsertTask (AsyncTask &&task)
	{
		BEGIN_ENUM_CHECKS();
		switch ( task->Type() )
		{
			case EThread::Main :		_AddTask( _mainQueue, task );		break;
			case EThread::Worker :		_AddTask( _workerQueue, task );		break;
			case EThread::Renderer :	_AddTask( _renderQueue, task );		break;
			case EThread::FileIO :		_AddTask( _fileQueue, task );		break;
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
	void  TaskScheduler::_AddTask (_TaskQueue<N> &tq, const AsyncTask &task)
	{
		for (;;)
		{
			for (auto& q : tq.queues)
			{
				if ( q.guard.try_lock() )
				{
					q.tasks.push_back( task );
					q.guard.unlock();
					return;
				}
			}
		}
	}


}	// AE::Threading
