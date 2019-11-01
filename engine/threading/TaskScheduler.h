// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Containers/ArrayView.h"
#include "stl/Containers/FixedArray.h"
#include "stl/Containers/NtStringView.h"
#include "stl/Algorithms/Cast.h"
#include "stl/ThreadSafe/SpinLock.h"

#include <chrono>
#include <mutex>

#ifdef AE_ENABLE_VTUNE_API
#	include <ittnotify.h>
#	define AE_VTUNE( ... )	__VA_ARGS__
#else
#	define AE_VTUNE( ... )
#endif

#if 1
#	define AE_SCHEDULER_PROFILING( ... )	__VA_ARGS__
#else
#	define AE_SCHEDULER_PROFILING( ... )
#endif

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
		ND_ EThread  Type () const	{ return _threadType; }

		ND_ EStatus  Status ()		{ return _status.load( memory_order_relaxed ); }

	protected:
		IAsyncTask (EThread type);

		virtual void Run () = 0;
		virtual void Cancel () {}

		ND_ virtual NtStringView  DbgName () const	{ return "unknown"; }
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
		virtual bool  Attach (Ptr<TaskScheduler>, uint uid) = 0;
		virtual void  Detach () = 0;

		ND_ virtual NtStringView  DbgName () const	{ return "thread"; }
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

			AE_SCHEDULER_PROFILING(
				std::atomic<uint64_t>	_stallTime	{0};	// Nanoseconds
				std::atomic<uint64_t>	_workTime	{0};
			)

		// methods
		public:
			_TaskQueue ();

			void  Resize (size_t count);
		};

		using MainQueue_t		= _TaskQueue< 2 >;
		using RenderQueue_t		= _TaskQueue< 2 >;
		using WorkerQueue_t		= _TaskQueue< 16 >;
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
		
		AE_VTUNE(
			__itt_domain*	_vtuneDomain	= null;
		)

	// methods
	public:
		TaskScheduler ();
		~TaskScheduler ();

			bool Setup (size_t maxWorkerThreads);
			
		AE_VTUNE(
		ND_ __itt_domain*	GetVTuneDomain () const	{ return _vtuneDomain; }
		)

	// thread api
			bool AddThread (const ThreadPtr &thread);

			bool ProcessTask (EThread type, uint seed);

	// task api
		template <typename T, typename ...Args>
		ND_ AsyncTask  Run (Array<AsyncTask> &&dependsOn, Args&& ...args);

		ND_ bool  Wait (ArrayView<AsyncTask> tasks, Nanoseconds timeout = Nanoseconds{30'000'000'000});

			bool  Cancel (const AsyncTask &task);

	private:
		AsyncTask  _InsertTask (const AsyncTask &task, Array<AsyncTask> &&dependsOn);

		template <size_t N>
		void  _AddTask (_TaskQueue<N> &tq, const AsyncTask &task) const;

		template <size_t N>
		bool  _ProcessTask (_TaskQueue<N> &tq, uint seed) const;

		template <size_t N>
		static void  _WriteProfilerStat (StringView name, const _TaskQueue<N> &tq);
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
		return _InsertTask( MakeShared<T>( std::forward<Args>(args)... ), std::move(dependsOn) );
	}


}	// AE::Threading
