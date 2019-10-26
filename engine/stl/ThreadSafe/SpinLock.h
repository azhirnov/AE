// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Common.h"
#include <atomic>
#include <mutex>
#include <thread>

namespace AE::STL
{

	//
	// Spin Lock
	//

	struct SpinLock
	{
	// variables
	private:
		alignas(AE_CACHE_LINE) std::atomic<uint>	_flag { 0 };


	// methods
	public:
		SpinLock ()
		{}

		ND_ forceinline bool try_lock ()
		{
			uint	exp = 0;
			return not _flag.compare_exchange_strong( INOUT exp, 1, memory_order_acquire, memory_order_relaxed );
		}


		// for std::lock_guard
		forceinline void lock ()
		{
			uint	exp = 0;
			for (uint i = 0; not _flag.compare_exchange_weak( INOUT exp, 1, memory_order_acquire, memory_order_relaxed ); ++i)
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
			uint	exp = 1;
			_flag.compare_exchange_strong( INOUT exp, 0, memory_order_release, memory_order_relaxed );
		}
	};


}	// AE::STL