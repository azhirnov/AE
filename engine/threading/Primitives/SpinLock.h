// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifndef AE_LFAS_ENABLED
# include "threading/Common.h"
# include "stl/Utils/Noncopyable.h"
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
		
		~SpinLock ()
		{
			ASSERT( _flag.load() == 0 );
		}

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
				if ( i > 1000 ) {
					i = 0;
					std::this_thread::yield();
				}
				exp = 0;
			}
		}

		forceinline void unlock ()
		{
		#ifdef AE_DEBUG
			ASSERT( _flag.exchange( 0, EMemoryOrder::Release ) == 1 );
		#else
			_flag.store( 0, EMemoryOrder::Release );
		#endif
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
		SpinLockRelaxed ()
		{}

		~SpinLockRelaxed ()
		{
			ASSERT( _flag.load() == 0 );
		}

		ND_ forceinline bool try_lock ()
		{
			uint	exp = 0;
			return _flag.compare_exchange_weak( INOUT exp, 1, EMemoryOrder::Relaxed, EMemoryOrder::Relaxed );
		}


		// for std::lock_guard
		forceinline void lock ()
		{
			uint	exp = 0;
			for (uint i = 0; not _flag.compare_exchange_weak( INOUT exp, 1, EMemoryOrder::Relaxed, EMemoryOrder::Relaxed ); ++i)
			{
				if ( i > 1000 ) {
					i = 0;
					std::this_thread::yield();
				}
				exp = 0;
			}
		}

		forceinline void unlock ()
		{
		#ifdef AE_DEBUG
			ASSERT( _flag.exchange( 0, EMemoryOrder::Relaxed ) == 1 );
		#else
			_flag.store( 0, EMemoryOrder::Relaxed );
		#endif
		}
	};


}	// AE::Threading
