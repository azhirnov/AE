// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'
/*
	Async task states:
		TaskScheduler::Run() {
			pending state
		}
		TaskScheduler::ProcessTask() {
			if cancellation or one of input dependencies was canceled {
				OnCancel()
				set canceled state
			}
			if pending state and all input dependencies are complete {
				set in_progress state
				Run() {
					if cancellation
						return
					if something goes wrong
						set failed state and return
				}
				if seccessfully completed
					set completed state and return
				if cancellation
					OnCancel()
					set canceled state and return
				if canceled or failed
					return
			}
		}

	Order guaranties:
		AsyncTask::Run() will be called after all input dependencies Run() or OnCancel() methods have completed
*/

#pragma once

#include "stl/Containers/ArrayView.h"
#include "stl/Containers/FixedArray.h"
#include "stl/Containers/NtStringView.h"
#include "stl/Algorithms/Cast.h"
#include "stl/Types/Noncopyable.h"

#include "threading/Primitives/SpinLock.h"

#include <chrono>

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
	using Nanoseconds	= std::chrono::nanoseconds;
	using AsyncTask		= SharedPtr< class IAsyncTask >;

	struct WeakDeps
	{
		Array<AsyncTask>	items;

		WeakDeps () {}
		WeakDeps (Array<AsyncTask> &&deps) : items{std::move(deps)} {}
		WeakDeps (std::initializer_list<AsyncTask> deps) : items{deps} {}
		WeakDeps (WeakDeps &&) = default;
		WeakDeps (const WeakDeps &) = delete;
	};
	
	struct StrongDeps
	{
		Array<AsyncTask>	items;

		StrongDeps () {}
		StrongDeps (Array<AsyncTask> &&deps) : items{std::move(deps)} {}
		StrongDeps (std::initializer_list<AsyncTask> deps) : items{deps} {}
		StrongDeps (StrongDeps &&) = default;
		StrongDeps (const StrongDeps &) = delete;
	};



	//
	// Async Task interface
	//
	
	class IAsyncTask : public std::enable_shared_from_this< IAsyncTask >
	{
		friend class TaskScheduler;
		friend class IThread;		// can change task status
		
	// types
	public:
		enum class EStatus : uint
		{
			Pending,		// task has been added to the queue and is waiting until input dependencies complete
			InProgress,		// task was acquired by thread
			Cancellation,	// task required to be canceled

			_Finished,
			Completed,		// successfully completed

			_Interropted,
			Canceled,		// task was externally canceled
			Failed,			// task has internal error and has been failed
		};

		enum class EThread : uint
		{
			Main,		// thread with window message loop
			Worker,
			Renderer,	// for opengl
			FileIO,
			Network,
			_Count
		};


	// variables
	private:
		Atomic<EStatus>		_status					{EStatus::Pending};
		bool				_canBeCanceledByDeps	{true};
		const EThread		_threadType;
		Array<AsyncTask>	_dependsOn;


	// methods
	public:
		ND_ EThread	Type ()			 const	{ return _threadType; }

		ND_ EStatus	Status ()		 const	{ return _status.load( EMemoryOrder::Relaxed ); }

		ND_ bool	IsInQueue ()	 const	{ return Status() < EStatus::_Finished; }
		ND_ bool	IsFinished ()	 const	{ return Status() > EStatus::_Finished; }
		ND_ bool	IsInterropted () const	{ return Status() > EStatus::_Interropted; }

	protected:
		IAsyncTask (EThread type);

			virtual void			Run () = 0;
			virtual void			OnCancel ()			{}
		ND_ virtual NtStringView	DbgName ()	const	{ return "unknown"; }

			// call this only inside 'Run()' method
			bool OnFailure ();

	private:
		// call this methods only after 'Run()' method
		void _OnFinish ();
		void _Cancel ();

		bool _SetCancellationState ();
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
		virtual bool  Attach (uint uid) = 0;
		virtual void  Detach () = 0;

		ND_ virtual NtStringView  DbgName () const	{ return "thread"; }

	// helper functions
	protected:
		static void _RunTask (const AsyncTask &);
		static void _OnTaskFinish (const AsyncTask &);
	};



	//
	// Task Scheduler
	//

	class TaskScheduler final : public Noncopyable
	{
	// types
	private:
		struct alignas(AE_CACHE_LINE) _PerQueue
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
				Atomic<uint64_t>	_stallTime		{0};	// Nanoseconds
				Atomic<uint64_t>	_workTime		{0};
				Atomic<uint64_t>	_insertionTime	{0};
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

		Mutex				_threadGuard;
		Array<ThreadPtr>	_threads;
		
		AE_VTUNE(
			__itt_domain*	_vtuneDomain	= null;
		)

	// methods
	public:
		ND_ static TaskScheduler&  Instance ();

		bool Setup (size_t maxWorkerThreads);
		void Release ();
			
		AE_VTUNE(
		 ND_ __itt_domain*	GetVTuneDomain () const	{ return _vtuneDomain; }
		)

	// thread api
		bool AddThread (const ThreadPtr &thread);

		bool ProcessTask (EThread type, uint seed);

		ND_ AsyncTask  PullTask (EThread type, uint seed);

	// task api
		template <typename T, typename ...Args>
		AsyncTask  Run (Args&& ...args);
		
		template <typename T, typename ...Args>
		AsyncTask  Run (WeakDeps &&dependsOn, Args&& ...args);
		
		template <typename T, typename ...Args>
		AsyncTask  Run (StrongDeps &&dependsOn, Args&& ...args);

		ND_ bool  Wait (ArrayView<AsyncTask> tasks, Nanoseconds timeout = Nanoseconds{30'000'000'000});

		bool  Cancel (const AsyncTask &task);

	private:
		TaskScheduler ();
		~TaskScheduler ();

		AsyncTask  _InsertTask (const AsyncTask &task, Array<AsyncTask> &&dependsOn, bool canBeCanceledByDeps);

		template <size_t N>
		void  _AddTask (_TaskQueue<N> &tq, const AsyncTask &task) const;

		template <size_t N>
		AsyncTask  _PullTask (_TaskQueue<N> &tq, uint seed) const;
		
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
	inline AsyncTask  TaskScheduler::Run (Args&& ...args)
	{
		STATIC_ASSERT( IsBaseOf< IAsyncTask, T > );
		return _InsertTask( MakeShared<T>( std::forward<Args>(args)... ), {}, true );
	}
	
	template <typename T, typename ...Args>
	inline AsyncTask  TaskScheduler::Run (WeakDeps &&dependsOn, Args&& ...args)
	{
		STATIC_ASSERT( IsBaseOf< IAsyncTask, T > );
		return _InsertTask( MakeShared<T>( std::forward<Args>(args)... ), std::move(dependsOn.items), false );
	}
	
	template <typename T, typename ...Args>
	inline AsyncTask  TaskScheduler::Run (StrongDeps &&dependsOn, Args&& ...args)
	{
		STATIC_ASSERT( IsBaseOf< IAsyncTask, T > );
		return _InsertTask( MakeShared<T>( std::forward<Args>(args)... ), std::move(dependsOn.items), true );
	}

/*
=================================================
	Scheduler
=================================================
*/
	ND_ inline TaskScheduler&  Scheduler ()
	{
		return TaskScheduler::Instance();
	}


}	// AE::Threading
