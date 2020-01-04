// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "threading/Common.h"
#include "stl/Math/BitMath.h"
#include "stl/Math/Bytes.h"
#include "stl/Memory/UntypedAllocator.h"

namespace AE::Threading
{
	using namespace AE::Math;

	//
	// Lock-Free Linear Allocator
	//

	template <size_t ChunkSize,
			  size_t MemAlign = 8,
			  size_t MaxChunks = 16,
			  typename AllocatorType = UntypedAlignedAllocator
			 >
	class LfLinearAllocator final
	{
		STATIC_ASSERT( ChunkSize > 0 );
		STATIC_ASSERT( MaxChunks > 0 and MaxChunks < 64 );

	// types
	public:
		using Self			= LfLinearAllocator< ChunkSize, MemAlign, MaxChunks, AllocatorType >;
		using Index_t		= uint;
		using Allocator_t	= AllocatorType;

	private:
		using ChunkBits_t	= Atomic< Conditional< (MaxChunks > 32), uint64_t, uint32_t >>;
		using ChunkData_t	= StaticArray< Atomic<void *>, MaxChunks >;
		using ChunkOffset_t	= StaticArray< Atomic<BytesU>, MaxChunks >;


	// variables
	private:
		ChunkData_t			_chunks;
		ChunkOffset_t		_offsets;
		ChunkBits_t			_lockedForAlloc;	// 1 bit - chunk is not allocating in another thread
		Allocator_t			_alloc;				// thread-safe allocator

		static constexpr BytesU	_capacity {ChunkSize};
		static constexpr BytesU	_memAlign {MemAlign};


	// methods
	public:
		explicit LfLinearAllocator (const Allocator_t &alloc = Allocator_t()) :
			_alloc{ alloc }
		{
			for (size_t i = 0; i < MaxChunks; ++i) {
				_chunks[i].store( null, EMemoryOrder::Relaxed );
				_offsets[i].store( 0_b, EMemoryOrder::Relaxed );
			}
			_lockedForAlloc.store( 0, EMemoryOrder::Relaxed );
			ThreadFence( EMemoryOrder::Release );
		}


		~LfLinearAllocator ()
		{
			ThreadFence( EMemoryOrder::Acquire );
			_lockedForAlloc.store( UMax, EMemoryOrder::Relaxed );

			for (size_t i = 0; i < MaxChunks; ++i)
			{
				if ( void* ptr = _chunks[i].exchange( null, EMemoryOrder::Relaxed ))
					_alloc.Deallocate( ptr, _capacity, _memAlign );
			}
		}

		
		template <typename T>
		ND_ AE_ALLOCATOR T*  Alloc (size_t count = 1)
		{
			return Cast<T>( Alloc( SizeOf<T> * count, AlignOf<T> ));
		}


		ND_ AE_ALLOCATOR void*  Alloc (const BytesU size, const BytesU align)
		{
			for (size_t i = 0; i < MaxChunks; ++i)
			{
				if ( void* ptr = _chunks[i].load( EMemoryOrder::Relaxed ))
				{
					// find available space
					for (BytesU off = _offsets[i].load( EMemoryOrder::Relaxed );;)
					{
						BytesU	aligned_off = AlignToLarger( size_t(ptr) + off, align ) - size_t(ptr);
						
						if ( size > (_capacity - aligned_off) )
							break;

						if ( _offsets[i].compare_exchange_weak( INOUT off, aligned_off + size, EMemoryOrder::Relaxed ))
							return ptr + aligned_off;
					}
				}
				else
				{
					// lock
					for (ChunkBits_t::value_type exp = _lockedForAlloc.load( EMemoryOrder::Relaxed ), j = 0;
						 not _lockedForAlloc.compare_exchange_weak( INOUT exp, exp | (1u << i), EMemoryOrder::Relaxed );
						 ++j)
					{
						if ( j > 2000 ) {
							j = 0;
							std::this_thread::yield();
						}
					}
					
					// if locked and chunk is not allocated
					if ( (ptr = _chunks[i].load( EMemoryOrder::Relaxed )) == null )
					{
						ptr = _alloc.Allocate( _capacity, _memAlign );
						if ( not ptr )
							return null;	// failed to allocate

						_chunks[i].store( ptr, EMemoryOrder::Relaxed );
					}

					// unlock
					for (auto exp = _lockedForAlloc.load( EMemoryOrder::Relaxed );
						 not _lockedForAlloc.compare_exchange_weak( INOUT exp, exp & ~(1u << i), EMemoryOrder::Relaxed );)
					{}

					--i;
				}
			}
			return null;
		}
	};


}	// AE::Threading
