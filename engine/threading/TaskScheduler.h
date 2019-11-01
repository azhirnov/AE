// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Containers/ArrayView.h"
#include "stl/Containers/FixedArray.h"
#include "stl/Algorithms/Cast.h"
#include "stl/ThreadSafe/SpinLock.h"

#include <chrono>
#include <mutex>

namespace AE::Threading
{
	using namespace AE::STL;

	using Nanoseconds	= std::chrono::nanoseconds;
	


	//
	// Async Task interface
	//
	
	using AsyncTask = SharedPtr< class IAsyncTask >;

	class IAsyncTask : public std::enable_shared_from_this< IAsyncTask >
	{
		friend class TaskScheduler;
		
	// types
	public:
		enum class EStatus : uint
		{
			Pending,
			InProgress,

			_Finished,
			Complete,
			Canceled,
		};

		enum class EThread : uint
		{
			Main,			// thread with window message loop
			Worker,
			Renderer,		// for opengl
			FileIO,
			Network,
			_Count
		};


	// variables
	private:
		std::atomic<EStatus>	_status		{EStatus::Pending};
		const EThread			_threadType;
		Array<AsyncTask>		_dependsOn;


	// methods
	public:
		virtual void Cancel () {}

		ND_ EThread  Type () const	{ return _threadType; }

		ND_ EStatus  Status ()		{ return _status.load( memory_order_relaxed ); }

	protected:
		IAsyncTask (EThread type, Array<AsyncTask> &&deps);

		virtual void Run () = 0;

		virtual StringView DbgName () const	{ return ""; }
	};




	//
	// Thread interface
	//

	class IThread : public std::enable_shared_from_this< IThread >
	{
	// types
	public:
		using EThread = IAsyncTask::EThread;

	// interface
	public:
		virtual bool  Attach (Ptr<TaskScheduler>) = 0;
		virtual void  Detach () = 0;
	};



	//
	// Task Scheduler
	//

	class TaskScheduler
	{
	// types
	private:
		struct _PerQueue
		{
			SpinLock			guard;
			Array<AsyncTask>	tasks;
		};

		template <size_t N>
		class _TaskQueue
		{
		// variables
		public:
			FixedArray< _PerQueue, N >	queues;

		// methods
		public:
			_TaskQueue ();

			void  Resize (size_t count);
		};

		using MainQueue_t		= _TaskQueue< 2 >;
		using RenderQueue_t		= _TaskQueue< 2 >;
		using WorkerQueue_t		= _TaskQueue< 8 >;
		using FileQueue_t		= _TaskQueue< 2 >;
		using NetworkQueue_t	= _TaskQueue< 3 >;
		using ThreadPtr			= SharedPtr< IThread >;
		
		using TimePoint_t		= std::chrono::high_resolution_clock::time_point;
		using EStatus			= IAsyncTask::EStatus;
		using EThread			= IAsyncTask::EThread;
		

	// variables
	private:
		MainQueue_t			_mainQueue;
		WorkerQueue_t		_workerQueue;
		RenderQueue_t		_renderQueue;
		FileQueue_t			_fileQueue;
		NetworkQueue_t		_networkQueue;

		Array<ThreadPtr>	_threads;


	// methods
	public:
		TaskScheduler ();
		~TaskScheduler ();

	// thread api
			bool AddThread (const ThreadPtr &thread);

			bool ProcessTask (EThread type, uint seed);

	// task api
		template <typename T, typename ...Args>
		ND_ AsyncTask  Run (Array<AsyncTask> &&dependsOn, Args&& ...args);

		ND_ bool  Wait (ArrayView<AsyncTask> tasks, Nanoseconds timeout = Nanoseconds{30'000'000'000});

			bool  Cancel (const AsyncTask &task);

	private:
		AsyncTask  _InsertTask (AsyncTask &&task);

		template <size_t N>
		void  _AddTask (_TaskQueue<N> &tq, const AsyncTask &task);

		template <size_t N>
		bool  _ProcessTask (_TaskQueue<N> &tq, uint seed);

		
	};

	
/*
=================================================
	Run
=================================================
*/
	template <typename T, typename ...Args>
	inline AsyncTask  TaskScheduler::Run (Array<AsyncTask> &&dependsOn, Args&& ...args)
	{
		STATIC_ASSERT( IsBaseOf< IAsyncTask, T > );
		return _InsertTask( MakeShared<T>( std::forward<Args>(args)..., std::move(dependsOn) ));
	}


}	// AE::Threading
