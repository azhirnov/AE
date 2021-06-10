// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'
/*

*/

#include "stl/Containers/ArrayView.h"
#include "stl/Algorithms/StringUtils.h"
#include "stl/Utils/RefCounter.h"

using namespace AE::STL;
using namespace AE::Math;

namespace
{
	class IAsyncTask : public EnableRC<IAsyncTask> {};
	using AsyncTask = RC<IAsyncTask>;

	class TaskScheduler;
	static TaskScheduler*	scheduler = null;
	
	class Registry;
	static Registry*		registry = null;


	class Registry
	{
	public:
		template <typename FN>
		AsyncTask  Enqueue (FN&& fn);
	};
	
	template <typename FN>
	AsyncTask  Registry::Enqueue (FN&& fn)
	{
		return scheduler->Run( fn );
	}


	class TaskScheduler
	{
	private:
		using TaskFn_t	= Function< void (String &) >;

		class AsyncTaskImpl final : public IAsyncTask
		{
		private:
			TaskFn_t			_fn;
			Array< AsyncTask >	_deps;

		public:
			AsyncTaskImpl (TaskFn_t &&fn, ArrayView<AsyncTask> deps) : _fn{RVRef(fn)}, _deps{deps} {}
		};

	private:
		Array<AsyncTask>	_queue;
		String				_str;

	public:
		explicit TaskScheduler (uint numThreads);
		~TaskScheduler ();

		template <typename FN>
		AsyncTask  Run (FN&& fn, ArrayView<AsyncTask> deps = {});

		ND_ String  Build ();
	};
	

	TaskScheduler::TaskScheduler (uint numThreads)
	{
		ASSERT( numThreads > 0 );
	}

	TaskScheduler::~TaskScheduler ()
	{
		CHECK( _queue.empty() );
	}

	template <typename FN>
	AsyncTask  TaskScheduler::Run (FN&& fn, ArrayView<AsyncTask> deps)
	{
		auto	task = MakeRC<AsyncTaskImpl>( RVRef(fn), deps );
		_queue.emplace_back( task );
		return task;
	}
	
	String  TaskScheduler::Build ()
	{
		return _str;
	}

	
	static void ECS_Test1 ()
	{
		scheduler	= new TaskScheduler{2};
		registry	= new Registry{};
		{
			// physics: update position
			registry->Enqueue( [](String& str){ str << "physics: update position\n"; });

			// logic: update position (portals)
			// visibility: check visibility
			// visibility: update LODs
			// renderer: draw
		}
		delete registry;
		delete scheduler;
	}
}

extern void Alg_ECSMultithreaded ()
{
	AE_LOGI( "ECS multithreading" );
	ECS_Test1();
	AE_LOGI( "=============================\n\n" );
}
