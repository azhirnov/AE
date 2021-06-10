// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'
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

#include "stl/Algorithms/ArrayUtils.h"
#include "stl/Containers/ArrayView.h"
#include "stl/Containers/FixedArray.h"
#include "stl/Containers/NtStringView.h"
#include "stl/Containers/AnyTypeRef.h"
#include "stl/Algorithms/Cast.h"
#include "stl/Utils/Noncopyable.h"
#include "stl/Utils/RefCounter.h"
#include "stl/CompileTime/TypeList.h"
#include "stl/CompileTime/TemplateUtils.h"

#include "threading/Primitives/SpinLock.h"
#include "threading/Primitives/RWSpinLock.h"

#include <chrono>

#ifdef AE_ENABLE_VTUNE_API
#	include <ittnotify.h>
#	define AE_VTUNE( ... )	__VA_ARGS__
#else
#	define AE_VTUNE( ... )
#endif

#if defined(AE_DEBUG) //or defined(AE_PROFILE)
#	define AE_SCHEDULER_PROFILING( ... )	__VA_ARGS__
#else
#	define AE_SCHEDULER_PROFILING( ... )
#endif

namespace AE::Threading
{
	using AsyncTask					= RC< class IAsyncTask >;
	using TaskDependencyManagerPtr	= RC< class ITaskDependencyManager >;

	
	namespace _hidden_
	{
		template <bool IsStrongDep>
		struct _TaskDependency
		{
			AsyncTask	_task;

			explicit _TaskDependency (AsyncTask &&task) : _task{RVRef(task)} {}
			explicit _TaskDependency (const AsyncTask &task) : _task{task} {}
			_TaskDependency (_TaskDependency &&) = default;
			_TaskDependency (const _TaskDependency &) = default;
		};
	
		template <bool IsStrongDep>
		struct _TaskDependencyArray : ArrayView< AsyncTask >
		{
			_TaskDependencyArray (AsyncTask const* ptr, usize count) : ArrayView{ptr, count} {}

			_TaskDependencyArray (std::initializer_list<AsyncTask> list) : ArrayView{list} {}

			template <typename AllocT>
			_TaskDependencyArray (const std::vector<AsyncTask,AllocT> &vec) : ArrayView{vec} {}

			template <usize S>
			_TaskDependencyArray (const StaticArray<AsyncTask,S> &arr) : ArrayView{arr} {}

			template <usize S>
			_TaskDependencyArray (const AsyncTask (&arr)[S]) : ArrayView{arr} {}
		};

	}	// _hidden_

	using WeakDep   = Threading::_hidden_::_TaskDependency<false>;
	using StrongDep = Threading::_hidden_::_TaskDependency<true>;
	
	using WeakDepArray   = Threading::_hidden_::_TaskDependencyArray<false>;
	using StrongDepArray = Threading::_hidden_::_TaskDependencyArray<true>;




	//
	// Interlock Dependency
	//

	struct InterlockDependency
	{
	// types
	public:
		enum class EAction
		{
			TryLock,
			Unlock,
		};
		using InterlockFn_t	= bool (*) (void* data, EAction act);	// TODO: single pointer


	// variables
	private:
		InterlockFn_t	_fn		= null;
		void *			_data	= null;


	// methods
	public:
		InterlockDependency () {}
		InterlockDependency (InterlockFn_t fn, void *data) : _fn{fn}, _data{data} {}

		ND_ explicit operator bool () const	{ return _fn; }

		ND_ bool  TryLock ()				{ return _fn( _data, EAction::TryLock ); }
		ND_ bool  Unlock ()					{ bool res = _fn ? _fn( _data, EAction::Unlock ) : true;  Clear();  return res; }
			void  Clear ()					{ _fn = null;  _data = null; }
	};



	//
	// Async Task interface
	//
	
	class alignas(AE_CACHE_LINE) IAsyncTask : public EnableRC< IAsyncTask >
	{
		friend class ITaskDependencyManager;	// can change '_waitBits' and '_canceledDepsCount'
		friend class TaskScheduler;				// can change '_status'
		friend class IThread;					// can change '_status'
		
	// types
	public:
		enum class EStatus : uint
		{
			Initial,
			Pending,		// task has been added to the queue and is waiting until input dependencies complete
			InProgress,		// task was acquired by thread
			Cancellation,	// task required to be canceled
			Continue,		// task will be returned to scheduler

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
			Renderer,	// single thread for opengl, multiple for vulkan (can be mixed with 'Worker')
			FileIO,
			Network,
			_Count
		};


	private:
		struct TaskDependency
		{
			AsyncTask		task;
			uint			bitIndex : 8;	// to reset bit in '_waitBits'
			uint			isStrong : 1;	// to increment '_canceledDepsCount'
		};

		struct OutputChunk
		{
			OutputChunk *						next		= null;
			uint								selfIndex	= UMax;
			FixedArray< TaskDependency, 15 >	tasks;

			void Init (uint idx);

			static auto&  _GetPool ();
		};

		using WaitBits_t = ulong;


	// variables
	private:
		Atomic< EStatus >		_status				{EStatus::Initial};
		Atomic< uint >			_canceledDepsCount	{0};		// TODO: pack with '_status'
		Atomic< WaitBits_t >	_waitBits			{~0ull};
		const EThread			_threadType;
		
		SpinLock				_outputGuard;
		OutputChunk *			_output				= null;

		InterlockDependency		_interlockDep;		// TODO: remove

		// TODO: profiling:
		// - added to queue time
		// - start execution time


	// methods
	public:
		virtual ~IAsyncTask ();

		ND_ EThread	Type ()			 const	{ return _threadType; }

		ND_ EStatus	Status ()		 const	{ return _status.load( EMemoryOrder::Relaxed ); }

		ND_ bool	IsInQueue ()	 const	{ return Status() < EStatus::_Finished; }
		ND_ bool	IsFinished ()	 const	{ return Status() > EStatus::_Finished; }
		ND_ bool	IsInterropted () const	{ return Status() > EStatus::_Interropted; }

	protected:
		explicit IAsyncTask (EThread type);

			virtual void			Run () = 0;
			virtual void			OnCancel ()			{}
		ND_ virtual NtStringView	DbgName ()	const	{ return "unknown"; }

			// call this only inside 'Run()' method
			void  OnFailure ();

			// call this only inside 'Run()' method
			bool  Continue ();

			template <typename ...Deps>
			bool  Continue (const Tuple<Deps...> &deps);

			// call this before reusing task
			bool  _ResetState ();

	private:
		// call this methods only after 'Run()' method
		void  _OnFinish (OUT bool& rerun);
		void  _Cancel ();

		bool  _SetCancellationState ();
		void  _FreeOutputChunks (bool isCanceled);
	};




	//
	// Thread interface
	//

	class IThread : public EnableRC< IThread >
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
		static void  _RunTask (const AsyncTask &);
		static void  _OnTaskFinish (const AsyncTask &);
	};



	//
	// Task Dependency Manager interface
	//

	class ITaskDependencyManager : public EnableRC< ITaskDependencyManager >
	{
	// interface
	public:
		virtual ~ITaskDependencyManager () {}

		virtual bool  Resolve (AnyTypeCRef dep, const AsyncTask &task, INOUT uint &bitIndex) = 0;


	// helper functions
	protected:
		static void  _SetDependencyCompletionStatus (const AsyncTask &task, uint depIndex, bool cancel = false);
	};



	//
	// Task Scheduler
	//

	class TaskScheduler final : public Noncopyable
	{
		friend class IAsyncTask;

	// types
	private:
		struct alignas(AE_CACHE_LINE) _PerQueue
		{
			SpinLock			guard;		// TODO: use single atomic bitfield for all queues
			Array<AsyncTask>	tasks;
		};

		template <usize N>
		class _TaskQueue
		{
		// variables
		public:
			FixedArray< _PerQueue, N >	queues;

			AE_SCHEDULER_PROFILING(
				Atomic<ulong>	_stallTime		{0};	// Nanoseconds
				Atomic<ulong>	_workTime		{0};
				Atomic<ulong>	_insertionTime	{0};
			)

		// methods
		public:
			_TaskQueue ();

			void  Resize (usize count);
		};

		using MainQueue_t		= _TaskQueue< 2 >;
		using RenderQueue_t		= _TaskQueue< 2 >;
		using WorkerQueue_t		= _TaskQueue< 16 >;
		using FileQueue_t		= _TaskQueue< 2 >;
		using NetworkQueue_t	= _TaskQueue< 3 >;
		using ThreadPtr			= RC< IThread >;
		
		using TimePoint_t		= std::chrono::high_resolution_clock::time_point;
		using EStatus			= IAsyncTask::EStatus;
		using EThread			= IAsyncTask::EThread;
		using OutputChunk_t		= IAsyncTask::OutputChunk;

		using TaskDepsMngr_t	= HashMap< std::type_index, TaskDependencyManagerPtr >;
		

	// variables
	private:
		MainQueue_t			_mainQueue;
		WorkerQueue_t		_workerQueue;
		RenderQueue_t		_renderQueue;
		FileQueue_t			_fileQueue;
		NetworkQueue_t		_networkQueue;

		SharedMutex			_taskDepsMngrsGuard;
		TaskDepsMngr_t		_taskDepsMngrs;

		Mutex				_threadGuard;
		Array<ThreadPtr>	_threads;
		
		AE_VTUNE(
			__itt_domain*	_vtuneDomain	= null;
		)

	// methods
	public:
		static void  CreateInstance ();
		static void  DestroyInstance ();

		bool  Setup (usize maxWorkerThreads);
		void  Release ();
			
		AE_VTUNE(
		 ND_ __itt_domain*	GetVTuneDomain () const	{ return _vtuneDomain; }
		)

		template <typename T>
		bool  RegisterDependency (TaskDependencyManagerPtr mngr);

		template <typename T>
		bool  UnregisterDependency ();

	// thread api
		bool  AddThread (const ThreadPtr &thread);

		bool  ProcessTask (EThread type, uint seed);

		ND_ AsyncTask  PullTask (EThread type, uint seed);

	// task api
		template <typename TaskType, typename ...Ctor, typename ...Deps>
		AsyncTask  Run (Tuple<Ctor...>&& ctor = Default, const Tuple<Deps...> &deps = Default);
		
		template <typename ...Deps>
		bool  Run (const AsyncTask &task, const Tuple<Deps...> &deps = Default);

		bool  Enqueue (const AsyncTask &task);

		ND_ bool  Wait (ArrayView<AsyncTask> tasks, nanoseconds timeout = nanoseconds{30'000'000'000});

		bool  Cancel (const AsyncTask &task);

		
		friend TaskScheduler&  Scheduler ();


	private:
		TaskScheduler ();
		~TaskScheduler ();
		
		ND_ static TaskScheduler*  _Instance ();

		AsyncTask  _InsertTask (const AsyncTask &task, uint bitIndex);

		template <usize N>
		void  _AddTask (_TaskQueue<N> &tq, const AsyncTask &task) const;

		template <usize N>
		AsyncTask  _PullTask (_TaskQueue<N> &tq, uint seed) const;
		
		template <usize N>
		bool  _ProcessTask (_TaskQueue<N> &tq, uint seed);

		template <usize N>
		static void  _WriteProfilerStat (StringView name, const _TaskQueue<N> &tq);

		template <usize I, typename ...Args>
		constexpr bool  _AddDependencies (const AsyncTask &task, const Tuple<Args...> &args, INOUT uint &bitIndex);

		template <typename T>
		bool  _AddCustomDependency (const AsyncTask &task, T &dep, INOUT uint &bitIndex);

		bool  _AddTaskDependencies (const AsyncTask &task, const AsyncTask &deps, Bool isStrong, INOUT uint &bitIndex);

		bool  _SetInterlockDependency (const AsyncTask &task, const InterlockDependency &dep);
	};

	
/*
=================================================
	RegisterDependency
=================================================
*/
	template <typename T>
	inline bool  TaskScheduler::RegisterDependency (TaskDependencyManagerPtr mngr)
	{
		CHECK_ERR( mngr );
		EXLOCK( _taskDepsMngrsGuard );
		return _taskDepsMngrs.insert_or_assign( typeid(T), mngr ).second;
	}
	
/*
=================================================
	UnregisterDependency
=================================================
*/
	template <typename T>
	inline bool  TaskScheduler::UnregisterDependency ()
	{
		EXLOCK( _taskDepsMngrsGuard );
		return _taskDepsMngrs.erase( typeid(T) ) > 0;
	}

/*
=================================================
	Run
=================================================
*/
	template <typename TaskType, typename ...Ctor, typename ...Deps>
	inline AsyncTask  TaskScheduler::Run (Tuple<Ctor...>&& ctorArgs, const Tuple<Deps...> &deps)
	{
		STATIC_ASSERT( IsBaseOf< IAsyncTask, TaskType > );

		AsyncTask	task		= ctorArgs.Apply([] (auto&& ...args) { return MakeRC<TaskType>( FwdArg<decltype(args)>(args)... ); });
		uint		bit_index	= 0;
		
		CHECK_ERR( _AddDependencies<0>( task, deps, INOUT bit_index ));

		return _InsertTask( task, bit_index );
	}
	
/*
=================================================
	Run
=================================================
*/
	template <typename ...Deps>
	inline bool  TaskScheduler::Run (const AsyncTask &task, const Tuple<Deps...> &deps)
	{
		CHECK_ERR( task );

		uint	bit_index = 0;
		CHECK_ERR( _AddDependencies<0>( task, deps, INOUT bit_index ));

		return _InsertTask( task, bit_index ) != null;
	}

/*
=================================================
	_AddDependencies
=================================================
*/
	template <usize I, typename ...Args>
	inline constexpr bool  TaskScheduler::_AddDependencies (const AsyncTask &task, const Tuple<Args...> &args, INOUT uint &bitIndex)
	{
		if constexpr( I < CountOf<Args...>() )
		{
			using T = typename TypeList< Args... >::template Get<I>;

			// current task will start anyway, regardless of whether dependent tasks are canceled
			if constexpr( IsSameTypes< T, WeakDep >)
				CHECK_ERR( _AddTaskDependencies( task, args.template Get<I>()._task, False{"weak"}, INOUT bitIndex ))
			else
			if constexpr( IsSameTypes< T, WeakDepArray >)
				for (auto& dep : args.template Get<I>()) {
					CHECK_ERR( _AddTaskDependencies( task, dep, False{"weak"}, INOUT bitIndex ));
				}
			else
			// current task will be canceled if one of dependent task are canceled
			if constexpr( IsSameTypes< T, StrongDep >)
				CHECK_ERR( _AddTaskDependencies( task, args.template Get<I>()._task, True{"strong"}, INOUT bitIndex ))
			else
			if constexpr( IsSameTypes< T, StrongDepArray > or IsSameTypes< T, ArrayView<AsyncTask> >)
				for (auto& dep : args.template Get<I>()) {
					CHECK_ERR( _AddTaskDependencies( task, dep, False{"strong"}, INOUT bitIndex ));
				}
			else
			// implicitlly it is strong dependency
			if constexpr( IsSpecializationOf< T, RC > and IsBaseOf< IAsyncTask, RemoveRC<T> >)
				CHECK_ERR( _AddTaskDependencies( task, args.template Get<I>(), True{"strong"}, INOUT bitIndex ))
			else
			// other dependencies
			if constexpr( IsSameTypes< T, InterlockDependency >)
				CHECK_ERR( _SetInterlockDependency( task, args.template Get<I>() ))
			else
				CHECK_ERR( _AddCustomDependency( task, args.template Get<I>(), INOUT bitIndex ));

			return _AddDependencies<I+1>( task, args, INOUT bitIndex );
		}
		else
		{
			Unused( task, args, bitIndex );
			return true;
		}
	}
	
/*
=================================================
	_AddCustomDependency
=================================================
*/
	template <typename T>
	inline bool  TaskScheduler::_AddCustomDependency (const AsyncTask &task, T &dep, INOUT uint &bitIndex)
	{
		SHAREDLOCK( _taskDepsMngrsGuard );

		auto	iter = _taskDepsMngrs.find( typeid(T) );
		CHECK_ERR( iter != _taskDepsMngrs.end() );

		return iter->second->Resolve( AnyTypeCRef{dep}, task, INOUT bitIndex );
	}

/*
=================================================
	Scheduler
=================================================
*/
	ND_ inline TaskScheduler&  Scheduler ()
	{
		return *TaskScheduler::_Instance();
	}
//-----------------------------------------------------------------------------


	
/*
=================================================
	Continue
=================================================
*/
	template <typename ...Deps>
	inline bool  IAsyncTask::Continue (const Tuple<Deps...> &deps)
	{
		uint	bit_index = 0;
		CHECK_ERR( Scheduler()._AddDependencies<0>( GetRC(), deps, INOUT bit_index ));
		
		// some dependencies may be already completed, so merge bit mask with current
		_waitBits.fetch_and( ToBitMask<ulong>( bit_index ), EMemoryOrder::Relaxed );
		
		for (EStatus expected = EStatus::InProgress;
			 not _status.compare_exchange_weak( INOUT expected, EStatus::Continue, EMemoryOrder::Relaxed );)
		{
			// status has been changed in another thread
			if ( expected > EStatus::_Finished )
				return false;
			
			// 'compare_exchange_weak' can return 'false' even if expected value is the same as current value in atomic
			ASSERT( expected == EStatus::InProgress );
		}
		return true;
	}

}	// AE::Threading
