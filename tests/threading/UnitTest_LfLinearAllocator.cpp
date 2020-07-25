// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "threading/Memory/LfLinearAllocator.h"
#include "stl/Containers/FixedTupleArray.h"
#include "UnitTest_Common.h"

namespace
{
	static void  LfLinearAllocator_Test1 ()
	{
		static constexpr uint	MaxThreads	= 8;
		static constexpr uint	ElemSize	= 8;

		FixedArray< std::thread, MaxThreads >	worker_thread;
		StaticArray< Array<void*>, MaxThreads >	thread_data;
		LfLinearAllocator< 4<<10 >				alloc;

		for (uint i = 0; i < MaxThreads; ++i)
		{
			worker_thread.emplace_back( std::thread{
				[&alloc, data = &thread_data[i]] ()
				{
					for (;;)
					{
						std::this_thread::yield();

						void*	ptr = alloc.Alloc( BytesU{ElemSize}, 8_b );
						if ( not ptr )
							return;

						std::memset( ptr, uint8_t(data->size()), ElemSize );
						data->push_back( ptr );
					}
				}
			});
		}
		
		for (uint i = 0; i < MaxThreads; ++i) {
			worker_thread[i].join();
		}

		for (auto& items : thread_data)
		{
			for (size_t i = 0; i < items.size(); ++i)
			{
				StaticArray< uint8_t, ElemSize >	ref;

				ref.fill( uint8_t(i) );

				TEST( std::memcmp( ref.data(), items[i], ElemSize ) == 0 );
			}
		}
		
		// TODO: fix memleak in allocator
	}
}


extern void UnitTest_LfLinearAllocator ()
{
	LfLinearAllocator_Test1();

	AE_LOGI( "UnitTest_LfLinearAllocator - passed" );
}
