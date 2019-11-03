// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "threading/Primise/Promise.h"
#include "threading/TaskSystem/WorkerThread.h"
#include "stl/Algorithms/StringUtils.h"
#include "UnitTest_Common.h"

namespace
{
	template <typename T>
	void TestResult (Promise<T> &p, const T &expected)
	{
		auto	task = AsyncTask{ p.Then( [expected] (const T& res) { TEST( res == expected ); })};

		TEST( Scheduler().Wait( {task} ));
		TEST( task->Status() == IAsyncTask::EStatus::Completed );
	}

	void Promise_Test1 ()
	{
		LocalTaskScheduler	scheduler {1};
		scheduler->AddThread( MakeShared<WorkerThread>() );

		auto p = MakePromise( [] () { return "a"s; })
			.Then([] (const String &in) { return in + "b"; });

		TestResult( p, "ab"s );
	}
	

	void Promise_Test2 ()
	{
		LocalTaskScheduler	scheduler {1};
		scheduler->AddThread( MakeShared<WorkerThread>() );

		auto p0 = MakePromise( [] () { return "a"s; });
		auto p1 = MakePromise( [] () { return "b"s; });
		auto p2 = MakePromise( [] () { return 1u; });
		auto p3 = MakePromiseFromTuple( MakeTuple( p0, p1, p2 ));

		auto p4 = p3.Then( [] (const Tuple<String, String, uint> &in) {
				return std::get<0>(in) + std::get<1>(in) + ToString(std::get<2>(in));
			});

		TestResult( p4, "ab1"s );
	}
}


extern void UnitTest_Promise ()
{
	Promise_Test1();
	Promise_Test2();

	AE_LOGI( "UnitTest_Promise - passed" );
}
