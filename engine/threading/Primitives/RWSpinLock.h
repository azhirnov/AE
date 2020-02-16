// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifndef AE_LFAS_ENABLED
# include "threading/Common.h"
# include "stl/Types/Noncopyable.h"
#endif

namespace AE::Threading
{

	//
	// Read-Write Spin Lock
	//

	struct RWSpinLock final : public Noncopyable
	{
	// variables
	private:
		Atomic<int>		_flag { 0 };	// 0 -- unlocked, -1 -- write lock, >0 -- read lock


	// methods
	public:
		RWSpinLock ()
		{}
		
		~RWSpinLock ()
		{
			ASSERT( _flag.load() == 0 );
		}

		ND_ forceinline bool  try_lock ()
		{
			int	exp = 0;
			return _flag.compare_exchange_strong( INOUT exp, -1, EMemoryOrder::Acquire, EMemoryOrder::Relaxed );
		}


		// for std::lock_guard / std::unique_lock
		forceinline void  lock ()
		{
			int	exp = 0;
			for (uint i = 0; not _flag.compare_exchange_weak( INOUT exp, -1, EMemoryOrder::Acquire, EMemoryOrder::Relaxed ); ++i)
			{
				if ( i > 1000 ) {
					i = 0;
					std::this_thread::yield();
				}
				exp = 0;
			}
		}

		forceinline void  unlock ()
		{
		#ifdef AE_DEBUG
			ASSERT( _flag.exchange( 0, EMemoryOrder::Release ) == -1 );
		#else
			_flag.store( 0, EMemoryOrder::Release );
		#endif
		}


		ND_ forceinline bool  try_lock_shared ()
		{
			int	exp = 0;
			for (; not _flag.compare_exchange_weak( INOUT exp, exp + 1, EMemoryOrder::Acquire, EMemoryOrder::Relaxed );)
			{
				if ( exp < 0 )
					return false;
			}
			return true;
		}


		// for std::shared_lock
		forceinline void  lock_shared ()
		{
			int	exp = 0;
			for (uint i = 0; not _flag.compare_exchange_weak( INOUT exp, exp + 1, EMemoryOrder::Acquire, EMemoryOrder::Relaxed ); ++i)
			{
				if ( i > 1000 ) {
					i = 0;
					std::this_thread::yield();
				}
				exp = (exp < 0 ? 0 : exp);
			}
		}


		forceinline void  unlock_shared ()
		{
			int	old = _flag.fetch_sub( 1, EMemoryOrder::Release );
			ASSERT( old >= 0 );
			Unused( old );
		}


		ND_ forceinline bool  try_shared_to_exclusive ()
		{
			int	exp = 1;
			return _flag.compare_exchange_strong( INOUT exp, -1, EMemoryOrder::Acquire, EMemoryOrder::Relaxed );
		}


		forceinline void  exclusive_to_shared ()
		{
			int	exp = -1;
			_flag.compare_exchange_strong( INOUT exp, 1, EMemoryOrder::Acquire, EMemoryOrder::Relaxed );
			ASSERT( exp == -1 );
		}
	};


}	// AE::Threading
