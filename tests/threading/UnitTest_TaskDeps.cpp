// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "threading/TaskSystem/WorkerThread.h"
#include "threading/Primitives/DataRaceCheck.h"
#include "UnitTest_Common.h"


namespace
{
	
	class Test1_Task1 : public IAsyncTask
	{
	public:
		uint&	value;

		Test1_Task1 (uint &val) : IAsyncTask{ EThread::Worker }, value{val} {}

		void Run () override
		{
			value = (value << 4) | 1;
		}
	};

	class Test1_Task2 : public IAsyncTask
	{
	public:
		uint&	value;
		
		Test1_Task2 (uint &val) : IAsyncTask{ EThread::Worker }, value{val} {}

		void Run () override
		{
			value = (value << 4) | 2;
		}
	};

	static void  TaskDeps_Test1 ()
	{
		LocalTaskScheduler	scheduler {1};

		uint			value	= 1;	// access to value protected by internal synchronizations
		const uint		required = (((1 << 4) | 1) << 4) | 2;

		AsyncTask		task1	= scheduler->Run<Test1_Task1>( Tuple{std::ref(value)} );
		AsyncTask		task2	= scheduler->Run<Test1_Task2>( Tuple{std::ref(value)}, Tuple{StrongDep{task1}} );
		TEST( task1 and task2 );

		scheduler->AddThread( MakeShared<WorkerThread>() );
		
		TEST( scheduler->Wait({ task1, task2 }));
		TEST( task1->Status() == IAsyncTask::EStatus::Completed );
		TEST( task2->Status() == IAsyncTask::EStatus::Completed );
		TEST( value == required );
	}
//-----------------------------------------------------------------------------

	

	class Test2_Task1 : public IAsyncTask
	{
	public:
		uint&	value;

		Test2_Task1 (uint &val) : IAsyncTask{ EThread::Worker }, value{val} {}

		void Run () override
		{
			value = (value << 4) | 1;
		}

		void OnCancel () override
		{
			value = (value << 4) | 2;
		}
	};

	class Test2_Task2 : public IAsyncTask
	{
	public:
		uint&	value;
		
		Test2_Task2 (uint &val) : IAsyncTask{ EThread::Worker }, value{val} {}

		void Run () override
		{
			value = (value << 4) | 2;
		}

		void OnCancel () override
		{
			value = (value << 4) | 1;
		}
	};

	static void  TaskDeps_Test2 ()
	{
		LocalTaskScheduler	scheduler {1};

		uint			value	= 1;	// access to value protected by internal synchronizations
		const uint		required = (((1 << 4) | 2) << 4) | 1;

		AsyncTask		task1	= scheduler->Run<Test2_Task1>( Tuple{std::ref(value)} );
		TEST( task1 );
		scheduler->Cancel( task1 );

		AsyncTask		task2	= scheduler->Run<Test2_Task2>( Tuple{std::ref(value)}, Tuple{StrongDep{task1}} );
		TEST( task2 );

		scheduler->AddThread( MakeShared<WorkerThread>() );
		
		TEST( scheduler->Wait({ task1, task2 }));
		TEST( task1->Status() == IAsyncTask::EStatus::Canceled );
		TEST( task2->Status() == IAsyncTask::EStatus::Canceled );
		TEST( value == required );
	}
//-----------------------------------------------------------------------------
	


	class Test3_Task1 : public IAsyncTask
	{
	public:
		uint&	value;

		Test3_Task1 (uint &val) : IAsyncTask{ EThread::Worker }, value{val} {}

		void Run () override
		{
			value = (value << 4) | 1;
		}

		void OnCancel () override
		{
			value = (value << 4) | 2;
		}
	};

	class Test3_Task2 : public IAsyncTask
	{
	public:
		uint&	value;
		
		Test3_Task2 (uint &val) : IAsyncTask{ EThread::Worker }, value{val} {}

		void Run () override
		{
			value = (value << 4) | 2;
		}

		void OnCancel () override
		{
			value = (value << 4) | 1;
		}
	};
	
	static void  TaskDeps_Test3 ()
	{
		LocalTaskScheduler	scheduler {1};

		uint			value	 = 1;	// access to value protected by internal synchronizations
		const uint		required = (((1 << 4) | 2) << 4) | 2;

		AsyncTask		task1	= scheduler->Run<Test3_Task1>( Tuple{std::ref(value)} );
		TEST( task1 );
		scheduler->Cancel( task1 );

		AsyncTask		task2	= scheduler->Run<Test3_Task2>( Tuple{std::ref(value)}, Tuple{WeakDep{task1}} );
		TEST( task2 );

		scheduler->AddThread( MakeShared<WorkerThread>() );
		
		TEST( scheduler->Wait({ task1, task2 }));
		TEST( task1->Status() == IAsyncTask::EStatus::Canceled );
		TEST( task2->Status() == IAsyncTask::EStatus::Completed );
		TEST( value == required );
	}
//-----------------------------------------------------------------------------
	


	struct Test4_CustomDep
	{
		Atomic<bool>*	flagRef = null;
	};
	
	class Test4_TaskDepManager : public ITaskDependencyManager
	{
	private:
		struct TaskDependency
		{
			Test4_CustomDep		dep;
			AsyncTask			task;
			uint				bitIndex;

			TaskDependency (const Test4_CustomDep &dep, const AsyncTask &task, uint index) : dep{dep}, task{task}, bitIndex{index} {}
		};
			
		class UpdateTask : public IAsyncTask
		{
		public:
			SharedPtr<Test4_TaskDepManager>	_mngr;

			UpdateTask (const SharedPtr<Test4_TaskDepManager> &mngr) : IAsyncTask{ EThread::Worker }, _mngr{mngr}
			{}

			void Run () override
			{
				_mngr->Update();
			}
		};

	private:
		Mutex					_depsListGuard;
		Array<TaskDependency>	_depsList;


	public:
		void  Update ()
		{
			EXLOCK( _depsListGuard );

			for (auto iter = _depsList.begin(); iter != _depsList.end();)
			{
				if ( iter->dep.flagRef->load() )
				{
					_SetDependencyCompletionStatus( iter->task, iter->bitIndex, false );
					iter = _depsList.erase( iter );
				}
				else
					++iter;
			}

			if ( _depsList.size() > 0 )
				Scheduler().Run<UpdateTask>( Tuple{Cast<Test4_TaskDepManager>(shared_from_this())} );
		}

		bool  Resolve (AnyTypeCRef dep, const AsyncTask &task, INOUT uint &bitIndex) override
		{
			CHECK_ERR( dep.Is<Test4_CustomDep>() );
			EXLOCK( _depsListGuard );

			_depsList.emplace_back( dep.As<Test4_CustomDep>(), task, bitIndex );
			++bitIndex;

			if ( _depsList.size() == 1 )
				Scheduler().Run<UpdateTask>( Tuple{Cast<Test4_TaskDepManager>(shared_from_this())} );

			return true;
		}
	};
	
	class Test4_Task1 : public IAsyncTask
	{
	public:
		uint&	value;

		Test4_Task1 (uint &val) : IAsyncTask{ EThread::Worker }, value{val} {}

		void Run () override
		{
			value = (value << 4) | 1;
		}

		void OnCancel () override
		{
			value = (value << 4) | 2;
		}
	};

	class Test4_Task2 : public IAsyncTask
	{
	public:
		uint&	value;
		
		Test4_Task2 (uint &val) : IAsyncTask{ EThread::Worker }, value{val} {}

		void Run () override
		{
			value = (value << 4) | 2;
		}

		void OnCancel () override
		{
			value = (value << 4) | 1;
		}
	};

	static void  TaskDeps_Test4 ()
	{
		LocalTaskScheduler	scheduler	{1};
		Atomic<bool>		flag		{false};
		Test4_CustomDep		custom_dep	{&flag};
		auto				task_mngr	= MakeShared<Test4_TaskDepManager>();
		
		uint			value	 = 1;	// access to value protected by internal synchronizations
		const uint		required = (((1 << 4) | 1) << 4) | 2;
		
		scheduler->RegisterDependency<Test4_CustomDep>( task_mngr );

		AsyncTask		task1	= scheduler->Run<Test4_Task1>( Tuple{std::ref(value)} );
		TEST( task1 );
		
		AsyncTask		task2	= scheduler->Run<Test4_Task2>( Tuple{std::ref(value)}, Tuple{task1, custom_dep} );
		TEST( task2 );
		
		scheduler->AddThread( MakeShared<WorkerThread>() );
		
		TEST( scheduler->Wait( {task1} ));
		TEST( task1->Status() == IAsyncTask::EStatus::Completed );
		TEST( not scheduler->Wait( {task2}, nanoseconds{100'000} ));

		flag.store( true );
		
		TEST( scheduler->Wait({ task1, task2 }));
		TEST( task2->Status() == IAsyncTask::EStatus::Completed );
		TEST( flag.load() == true );
		TEST( value == required );
	}
//-----------------------------------------------------------------------------

	

	struct Test5_SharedData
	{
		RWDataRaceCheck		drCheck;
		Atomic<uint>		completedTasks {0};
		Atomic<uint64_t>	sum {0};
		Array<uint>			values;
	};

	class Test5_Task1 : public IAsyncTask
	{
	public:
		Test5_SharedData&	data;
		
		Test5_Task1 (Test5_SharedData &data) : IAsyncTask{ EThread::Worker }, data{data} {}

		void Run () override
		{
			// add value
			{
				EXLOCK( data.drCheck );

				data.values.push_back( std::rand() );
			}

			++data.completedTasks;
		}

		void OnCancel () override
		{
			++data.completedTasks;
		}
	};
	
	class Test5_Task2 : public IAsyncTask
	{
	public:
		Test5_SharedData&	data;
		
		Test5_Task2 (Test5_SharedData& data) : IAsyncTask{ EThread::Worker }, data{data} {}

		void Run () override
		{
			// find min value
			uint	min_val = UMax;
			{
				SHAREDLOCK( data.drCheck );
				for (size_t i = 0; i < data.values.size(); ++i) {
					min_val = Min( min_val, data.values[i] );
				}
			}

			data.sum += min_val;

			++data.completedTasks;
		}

		void OnCancel () override
		{
			++data.completedTasks;
		}
	};

	static void  TaskDeps_Test5 ()
	{
		LocalTaskScheduler	scheduler	{2};
		SharedMutex			rw_lock;
		Test5_SharedData	shared;

		InterlockDependency	w_dep{	[](void* data, InterlockDependency::EAction act)
									{
										auto&	rw_lock = *Cast<SharedMutex>(data);
										if ( act == InterlockDependency::EAction::TryLock ) {
											return rw_lock.try_lock();
										}
										if ( act == InterlockDependency::EAction::Unlock ) {
											rw_lock.unlock();
											return true;
										}
										return false;
									},
									&rw_lock
								 };

		InterlockDependency	r_dep{	[](void* data, InterlockDependency::EAction act)
									{
										auto&	rw_lock = *Cast<SharedMutex>(data);
										if ( act == InterlockDependency::EAction::TryLock ) {
											return rw_lock.try_lock_shared();
										}
										if ( act == InterlockDependency::EAction::Unlock ) {
											rw_lock.unlock_shared();
											return true;
										}
										return false;
									},
									&rw_lock
								 };
		
		scheduler->AddThread( MakeShared<WorkerThread>() );
		scheduler->AddThread( MakeShared<WorkerThread>() );

		const uint	total_tasks = 300;

		for (uint i = 0; i < total_tasks/3; ++i)
		{
			TEST( scheduler->Run<Test5_Task2>( Tuple{std::ref(shared)}, Tuple{r_dep} ));
			TEST( scheduler->Run<Test5_Task1>( Tuple{std::ref(shared)}, Tuple{w_dep} ));
			TEST( scheduler->Run<Test5_Task2>( Tuple{std::ref(shared)}, Tuple{r_dep} ));
		}

		for (;;)
		{
			if ( shared.completedTasks.load() >= total_tasks )
				break;

			std::this_thread::sleep_for( milliseconds{100} );
		}

		TEST( shared.values.size() == total_tasks/3 );
	}
}


extern void UnitTest_TaskDeps ()
{
	TaskDeps_Test1();
	TaskDeps_Test2();
	TaskDeps_Test3();
	TaskDeps_Test4();
	TaskDeps_Test5();

	AE_LOGI( "UnitTest_TaskDeps - passed" );
}
