// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifndef AE_LFAS_ENABLED
# include "threading/Common.h"
# include "stl/Types/Noncopyable.h"
#endif

namespace AE::Threading
{

	//
	// Spin Lock
	//

	struct SpinLock final : public Noncopyable
	{
	// variables
	private:
		Atomic<uint>	_flag { 0 };


	// methods
	public:
		SpinLock ()
		{}

		ND_ forceinline bool try_lock ()
		{
			uint	exp = 0;
			return _flag.compare_exchange_strong( INOUT exp, 1, EMemoryOrder::Acquire, EMemoryOrder::Relaxed );
		}


		// for std::lock_guard
		forceinline void lock ()
		{
			uint	exp = 0;
			for (uint i = 0; not _flag.compare_exchange_weak( INOUT exp, 1, EMemoryOrder::Acquire, EMemoryOrder::Relaxed ); ++i)
			{
				if ( i > 100 ) {
					i = 0;
					std::this_thread::yield();
				}
				exp = 0;
			}
		}

		forceinline void unlock ()
		{
			uint	old = _flag.exchange( 0, EMemoryOrder::Release );
			ASSERT( old == 1 );
			Unused( old );
		}
	};

	

	//
	// Spin Lock Relaxed
	//

	struct SpinLockRelaxed final : public Noncopyable
	{
	// variables
	private:
		Atomic<uint>	_flag { 0 };


	// methods
	public:
		SpinLockRelaxed () {}

		ND_ forceinline bool try_lock ()
		{
			uint	exp = 0;
			return _flag.compare_exchange_strong( INOUT exp, 1, EMemoryOrder::Relaxed, EMemoryOrder::Relaxed );
		}


		// for std::lock_guard
		forceinline void lock ()
		{
			uint	exp = 0;
			for (uint i = 0; not _flag.compare_exchange_weak( INOUT exp, 1, EMemoryOrder::Relaxed, EMemoryOrder::Relaxed ); ++i)
			{
				if ( i > 100 ) {
					i = 0;
					std::this_thread::yield();
				}
				exp = 0;
			}
		}

		forceinline void unlock ()
		{
			uint	old = _flag.exchange( 0, EMemoryOrder::Relaxed );
			ASSERT( old == 1 );
			Unused( old );
		}
	};


}	// AE::Threading
