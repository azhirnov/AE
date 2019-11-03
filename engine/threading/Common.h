// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

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
	

#	ifdef AE_OPTIMAL_MEMORY_ORDER
	static constexpr std::memory_order	memory_order_acquire	= std::memory_order_acquire;
	static constexpr std::memory_order	memory_order_release	= std::memory_order_release;
	static constexpr std::memory_order	memory_order_acq_rel	= std::memory_order_acq_rel;
	static constexpr std::memory_order	memory_order_relaxed	= std::memory_order_relaxed;
#	else
	static constexpr std::memory_order	memory_order_acquire	= std::memory_order_seq_cst;
	static constexpr std::memory_order	memory_order_release	= std::memory_order_seq_cst;
	static constexpr std::memory_order	memory_order_acq_rel	= std::memory_order_seq_cst;
	static constexpr std::memory_order	memory_order_relaxed	= std::memory_order_seq_cst;
#	endif	// AE_OPTIMAL_MEMORY_ORDER

	
/*
=================================================
	ThreadFence
----
	for non-atomic and relaxed atomic accesses
=================================================
*/
	forceinline void ThreadFence (std::memory_order mem_order)
	{
		return std::atomic_thread_fence( mem_order );
	}
	
/*
=================================================
	CompilerBarrier
----
	take effect only for compiler and CPU instruction reordering
=================================================
*/
	forceinline void CompilerBarrier (std::memory_order mem_order)
	{
		return std::atomic_signal_fence( mem_order );
	}

}	// AE::Threading


// check definitions
#if defined (COMPILER_MSVC) or defined (COMPILER_CLANG)

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

#endif	// COMPILER_MSVC or COMPILER_CLANG
