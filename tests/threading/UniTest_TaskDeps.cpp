// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "threading/TaskSystem/WorkerThread.h"
#include "UnitTest_Common.h"


namespace
{
	void TaskDeps_Test1 ()
	{
		class Task1 : public IAsyncTask
		{
		public:
			uint&	value;

			Task1 (uint &val) : IAsyncTask{ EThread::Worker }, value{val} {}

			void Run () override
			{
				value = (value << 4) | 1;
			}
		};

		class Task2 : public IAsyncTask
		{
		public:
			uint&	value;
		
			Task2 (uint &val) : IAsyncTask{ EThread::Worker }, value{val} {}

			void Run () override
			{
				value = (value << 4) | 2;
			}
		};
		
		LocalTaskScheduler	scheduler {1};

		uint			value	= 1;
		const uint		required = (((1 << 4) | 1) << 4) | 2;

		AsyncTask		task1	= scheduler->Run<Task1>( std::ref(value) );
		AsyncTask		task2	= scheduler->Run<Task2>( StrongDeps{task1}, std::ref(value) );
		TEST( task1 and task2 );

		scheduler->AddThread( MakeShared<WorkerThread>() );
		
		TEST( scheduler->Wait({ task1, task2 }));
		TEST( value == required );
		TEST( task1->Status() == IAsyncTask::EStatus::Completed );
		TEST( task2->Status() == IAsyncTask::EStatus::Completed );
	}

	
	void TaskDeps_Test2 ()
	{
		class Task1 : public IAsyncTask
		{
		public:
			uint&	value;

			Task1 (uint &val) : IAsyncTask{ EThread::Worker }, value{val} {}

			void Run () override
			{
				value = (value << 4) | 1;
			}

			void OnCancel () override
			{
				value = (value << 4) | 2;
			}
		};

		class Task2 : public IAsyncTask
		{
		public:
			uint&	value;
		
			Task2 (uint &val) : IAsyncTask{ EThread::Worker }, value{val} {}

			void Run () override
			{
				value = (value << 4) | 2;
			}

			void OnCancel () override
			{
				value = (value << 4) | 1;
			}
		};
		
		LocalTaskScheduler	scheduler {1};

		uint			value	= 1;
		const uint		required = (((1 << 4) | 2) << 4) | 1;

		AsyncTask		task1	= scheduler->Run<Task1>( std::ref(value) );
		TEST( task1 );
		scheduler->Cancel( task1 );

		AsyncTask		task2	= scheduler->Run<Task2>( StrongDeps{task1}, std::ref(value) );
		TEST( task2 );

		scheduler->AddThread( MakeShared<WorkerThread>() );
		
		TEST( scheduler->Wait({ task1, task2 }));
		TEST( value == required );
		TEST( task1->Status() == IAsyncTask::EStatus::Canceled );
		TEST( task2->Status() == IAsyncTask::EStatus::Canceled );
	}

	
	void TaskDeps_Test3 ()
	{
		class Task1 : public IAsyncTask
		{
		public:
			uint&	value;

			Task1 (uint &val) : IAsyncTask{ EThread::Worker }, value{val} {}

			void Run () override
			{
				value = (value << 4) | 1;
			}

			void OnCancel () override
			{
				value = (value << 4) | 2;
			}
		};

		class Task2 : public IAsyncTask
		{
		public:
			uint&	value;
		
			Task2 (uint &val) : IAsyncTask{ EThread::Worker }, value{val} {}

			void Run () override
			{
				value = (value << 4) | 2;
			}

			void OnCancel () override
			{
				value = (value << 4) | 1;
			}
		};
		
		LocalTaskScheduler	scheduler {1};

		uint			value	= 1;
		const uint		required = (((1 << 4) | 2) << 4) | 2;

		AsyncTask		task1	= scheduler->Run<Task1>( std::ref(value) );
		TEST( task1 );
		scheduler->Cancel( task1 );

		AsyncTask		task2	= scheduler->Run<Task2>( WeakDeps{task1}, std::ref(value) );
		TEST( task2 );

		scheduler->AddThread( MakeShared<WorkerThread>() );
		
		TEST( scheduler->Wait({ task1, task2 }));
		TEST( value == required );
		TEST( task1->Status() == IAsyncTask::EStatus::Canceled );
		TEST( task2->Status() == IAsyncTask::EStatus::Completed );
	}
}


extern void UnitTest_TaskDeps ()
{
	TaskDeps_Test1();
	TaskDeps_Test2();
	TaskDeps_Test3();

	AE_LOGI( "UnitTest_TaskDeps - passed" );
}
