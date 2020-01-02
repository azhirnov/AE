// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Common.h"

#include <atomic>
#include <mutex>	// for lock_guard
#include <thread>


#ifdef AE_DEBUG
#	define AE_ENABLE_DATA_RACE_CHECK
#else
//#	define AE_OPTIMAL_MEMORY_ORDER
#endif


// exclusive lock
#ifndef EXLOCK
#	define EXLOCK( _syncObj_ ) \
		std::unique_lock	AE_PRIVATE_UNITE_RAW( __scopeLock, __COUNTER__ ) { _syncObj_ }
#endif

// shared lock
#ifndef SHAREDLOCK
#	define SHAREDLOCK( _syncObj_ ) \
		std::shared_lock	AE_PRIVATE_UNITE_RAW( __sharedLock, __COUNTER__ ) { _syncObj_ }
#endif


namespace AE::Threading
{
	using namespace AE::STL;

	template <typename T>	using Atomic	= std::atomic< T >;
	
	using Mutex		= std::mutex;
	using ThreadID	= std::thread::id;


#	ifdef AE_OPTIMAL_MEMORY_ORDER
	struct EMemoryOrder
	{
		static constexpr std::memory_order	Acquire			= std::memory_order_acquire;
		static constexpr std::memory_order	Release			= std::memory_order_release;
		static constexpr std::memory_order	AcquireRelase	= std::memory_order_acq_rel;
		static constexpr std::memory_order	Relaxed			= std::memory_order_relaxed;
	};
#	else
	struct EMemoryOrder
	{
		static constexpr std::memory_order	Acquire			= std::memory_order_seq_cst;
		static constexpr std::memory_order	Release			= std::memory_order_seq_cst;
		static constexpr std::memory_order	AcquireRelase	= std::memory_order_seq_cst;
		static constexpr std::memory_order	Relaxed			= std::memory_order_seq_cst;
	};
#	endif	// AE_OPTIMAL_MEMORY_ORDER

	
/*
=================================================
	ThreadFence
----
	for non-atomic and relaxed atomic accesses.
	ThreadFence( memory_order_acquire ) - invalidate cache
	ThreadFence( memory_order_release ) - flush cache
=================================================
*/
	forceinline void ThreadFence (std::memory_order order)
	{
		return std::atomic_thread_fence( order );
	}
	
/*
=================================================
	CompilerFence
----
	take effect only for compiler and CPU instruction reordering
=================================================
*/
	forceinline void CompilerFence (std::memory_order order)
	{
		return std::atomic_signal_fence( order );
	}

}	// AE::Threading


// check definitions
#ifdef AE_CPP_DETECT_MISSMATCH

#  ifdef AE_OPTIMAL_MEMORY_ORDER
#	pragma detect_mismatch( "AE_OPTIMAL_MEMORY_ORDER", "1" )
#  else
#	pragma detect_mismatch( "AE_OPTIMAL_MEMORY_ORDER", "0" )
#  endif

#  ifdef AE_ENABLE_DATA_RACE_CHECK
#	pragma detect_mismatch( "AE_ENABLE_DATA_RACE_CHECK", "1" )
#  else
#	pragma detect_mismatch( "AE_ENABLE_DATA_RACE_CHECK", "0" )
#  endif

#  ifdef AE_STD_BARRIER
#	pragma detect_mismatch( "AE_STD_BARRIER", "1" )
#  else
#	pragma detect_mismatch( "AE_STD_BARRIER", "0" )
#  endif

#endif	// AE_CPP_DETECT_MISSMATCH
