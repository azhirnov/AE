// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "UnitTest_Common.h"

namespace
{
	class WorkerThread : public IThread
	{
	private:
		std::thread			_thread;
		std::atomic<uint>	_looping {1};

	public:
		WorkerThread () {}
		
		bool Attach (Ptr<TaskScheduler> scheduler) override
		{
			_thread = std::thread{[this, scheduler] ()
			{
				for (; _looping.load( memory_order_relaxed );)
				{
					scheduler->ProcessTask( EThread::Worker, 0 );
				}
			}};
			return true;
		}

		void Detach () override
		{
			_looping.store( 0, memory_order_relaxed );
			_thread.join();
		}
	};
}


namespace
{
	void TaskDeps_Test1 ()
	{
		class Task1 : public IAsyncTask
		{
		public:
			uint&	value;

			Task1 (uint &val, Array<AsyncTask> &&deps) : IAsyncTask{ EThread::Worker, std::move(deps) }, value{val} {}

			void Run () override
			{
				value = (value << 4) | 1;
			}
		};

		class Task2 : public IAsyncTask
		{
		public:
			uint&	value;
		
			Task2 (uint &val, Array<AsyncTask> &&deps) : IAsyncTask{ EThread::Worker, std::move(deps) }, value{val} {}

			void Run () override
			{
				value = (value << 4) | 2;
			}
		};

		TaskScheduler	scheduler;

		uint			value	= 1;
		const uint		required = (((1 << 4) | 1) << 4) | 2;

		AsyncTask		task1	= scheduler.Run<Task1>( Array<AsyncTask>{}, std::ref(value) );
		AsyncTask		task2	= scheduler.Run<Task2>( Array<AsyncTask>{task1}, std::ref(value) );

		scheduler.AddThread( MakeShared<WorkerThread>() );
		
		TEST( scheduler.Wait({ task1, task2 }));
		TEST( value == required );
	}
}


extern void UnitTest_TaskDeps ()
{
	TaskDeps_Test1();

	AE_LOGI( "UnitTest_TaskDeps - passed" );
}
