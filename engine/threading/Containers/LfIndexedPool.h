// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'
/*
	This lock-free container has some limitations:
	- write access by same index must be externally synchronized.
	- read access is safe after flush & invalidation (synchronization).
	- Release() must be externally synchronized with all other read and write accesses.
	- Unassign() must be externally synchronized with all other read and write accesses to the 'index'.
	- custom allocator must be thread safe (use mutex or lock-free algorithms).

	Valid usage:
	1. Call Assign( index ).
	2. Write data by this 'index'.
	3. Share 'index' with other threads with only read access. Flush and invalidate cache before using in other threads.
	   Usually you will use mutex or other default sync primitive that automaticaly updates cache when share 'index' with other threads.
	   If you use lock-free algorithm use 'release' memory order after writing to 'index' and use 'acquire' memory order before reading from 'index'.
	4. Wait until all threads finish reading from 'index' then you may call Unassign( index ).
*/

#pragma once

#ifndef AE_LFAS_ENABLED
# include "stl/Containers/FixedArray.h"
# include "stl/CompileTime/Math.h"
# include "stl/Math/BitMath.h"
# include "stl/Memory/UntypedAllocator.h"
# include "threading/Common.h"
#endif

namespace AE::Threading
{

	//
	// Lock-Free Indexed Pool
	//

	template <typename ValueType,
			  typename IndexType,
			  size_t ChunkSize = 32,
			  size_t MaxChunks = 16,
			  typename AllocatorType = UntypedAlignedAllocator
			 >
	struct LfIndexedPool final
	{
		STATIC_ASSERT( ChunkSize > 0 );
		STATIC_ASSERT( ((ChunkSize % 32) == 0) or ((ChunkSize % 64) == 0) );
		STATIC_ASSERT( MaxChunks > 0 );
		STATIC_ASSERT( IsPowerOfTwo( ChunkSize ));	// must be power of 2 to increase performance
		STATIC_ASSERT( AllocatorType::IsThreadSafe );
		
	// types
	public:
		using Self				= LfIndexedPool< ValueType, IndexType, ChunkSize, MaxChunks, AllocatorType >;
		using Index_t			= IndexType;
		using Value_t			= ValueType;
		using Allocator_t		= AllocatorType;

	private:
		using Bitfield_t		= Conditional< (ChunkSize % 64) == 0, uint64_t, uint32_t >;

		static constexpr size_t	AtomicSize		= sizeof(Bitfield_t) * 8;
		static constexpr size_t	AtomicsCount	= ChunkSize / AtomicSize;

		using BitfieldArray_t	= StaticArray< Atomic<Bitfield_t>, AtomicsCount * MaxChunks >;
		
		using ValueChunk_t		= StaticArray< Value_t, ChunkSize >;
		using ValueChunks_t		= StaticArray< Atomic<ValueChunk_t *>, MaxChunks >;

		STATIC_ASSERT( BitfieldArray_t::value_type::is_always_lock_free );
		STATIC_ASSERT( ValueChunks_t::value_type::is_always_lock_free );


	// variables
	private:
		BitfieldArray_t		_assignedBits;	// 1 - is unassigned bit, 0 - assigned bit
		BitfieldArray_t		_createdBits;	// 1 - constructor has been called
		ValueChunks_t		_values;
		Allocator_t			_allocator;		// must be thread safe


	// methods
	public:
		LfIndexedPool (const Self &) = delete;
		LfIndexedPool (Self &&) = delete;

		Self& operator = (const Self &) = delete;
		Self& operator = (Self &&) = delete;
		

		explicit LfIndexedPool (const Allocator_t &alloc = Allocator_t()) :
			_allocator{ alloc }
		{
			for (auto& item : _assignedBits) {
				item.store( UMax, EMemoryOrder::Relaxed );
			}
			for (auto& item : _createdBits) {
				item.store( 0, EMemoryOrder::Relaxed );
			}
			for (auto& item : _values) {
				item.store( null, EMemoryOrder::Relaxed );
			}

			// flush cache
			ThreadFence( EMemoryOrder::Release );
		}

		~LfIndexedPool ()
		{
			Release();
		}
		
		
		// Must be externally synchronized.
		// It is unsafe to call destructor for a value that can be used in another thread.
		template <typename FN>
		void Release (FN &&dtor)
		{
			// invalidate cache 
			ThreadFence( EMemoryOrder::Acquire );

			for (size_t i = 0; i < MaxChunks; ++i)
			{
				ValueChunk_t*	chunk = _values[i].exchange( null, EMemoryOrder::Relaxed );

				if ( not chunk )
					continue;

				for (size_t k = 0, j = i * AtomicsCount; k < AtomicsCount; ++k, ++j)
				{
					Bitfield_t	ctor_bits	= _createdBits[j].exchange( UMax, EMemoryOrder::Relaxed );
					Bitfield_t	assigned	= _assignedBits[j].exchange( 0, EMemoryOrder::Relaxed );

					Unused( assigned );
					ASSERT( assigned == UMax );

					// call destructor
					for (size_t c = 0; c < AtomicSize; ++c)
					{
						if ( ctor_bits & (Bitfield_t(1) << c) )
							dtor( (*chunk)[c + k*AtomicSize] );
					}
				}

				_allocator.Deallocate( chunk, SizeOf<ValueChunk_t>, AlignOf<ValueChunk_t> );
			}
		}

		void Release ()
		{
			return Release([] (Value_t& value) { value.~Value_t(); });
		}


		template <typename FN>
		ND_ bool  Assign (OUT Index_t &outIndex, FN &&ctor)
		{
			for (size_t i = 0; i < MaxChunks; ++i)
			{
				// allocate
				ValueChunk_t*	chunk = _values[i].load( EMemoryOrder::Relaxed );

				if ( chunk == null )
				{
					chunk = Cast<ValueChunk_t>(_allocator.Allocate( SizeOf<ValueChunk_t>, AlignOf<ValueChunk_t> ));

					// set new pointer and invalidate cache
					for (ValueChunk_t* expected = null;
						 not _values[i].compare_exchange_weak( INOUT expected, chunk, EMemoryOrder::Acquire );)
					{
						// another thread has been allocated this chunk
						if ( expected != null and expected != chunk )
						{
							_allocator.Deallocate( chunk, SizeOf<ValueChunk_t>, AlignOf<ValueChunk_t> );
							chunk = expected;
							break;
						}
					}
				}

				// find available index
				for (size_t k = 0, j = i * AtomicsCount; k < AtomicsCount; ++k, ++j)
				{
					Bitfield_t	bits = _assignedBits[j].load( EMemoryOrder::Relaxed );

					for (int index = BitScanForward( bits ); index >= 0;)
					{
						const Bitfield_t	mask = Bitfield_t(1) << index;

						if ( _assignedBits[j].compare_exchange_weak( INOUT bits, bits ^ mask, EMemoryOrder::Acquire, EMemoryOrder::Relaxed ))	// 1 -> 0
						{
							outIndex = Index_t(index) | (Index_t(j) * AtomicSize);

							if ( not (_createdBits[j].fetch_or( mask, EMemoryOrder::Relaxed ) & mask) )	// 0 -> 1
							{
								ctor( &(*chunk)[index + k*AtomicSize], outIndex );
							}
							return true;
						}

						index = BitScanForward( bits );
					}
				}
			}

			outIndex = UMax;
			return false;
		}
		
		ND_ bool  Assign (OUT Index_t &outIndex)
		{
			return Assign( OUT outIndex, [](Value_t* ptr, Index_t) { PlacementNew<Value_t>( ptr ); });
		}


		// Must be externally synchronized with all threads that using 'index'
		bool  Unassign (Index_t index)
		{
			if ( index < Capacity() )
			{
				const uint	chunk_idx	= index / AtomicSize;
				const uint	bit_idx		= index % AtomicSize;
				Bitfield_t	mask		= Bitfield_t(1) << bit_idx;
				Bitfield_t	old_bits	= _assignedBits[chunk_idx].fetch_or( mask, EMemoryOrder::Relaxed );	// 0 -> 1

				return not (old_bits & mask);
			}
			return false;
		}


		ND_ bool  IsAssigned (Index_t index)
		{
			if ( index < Capacity() )
			{
				const uint	chunk_idx	= index / AtomicSize;
				const uint	bit_idx		= index % AtomicSize;
				Bitfield_t	mask		= Bitfield_t(1) << bit_idx;
				Bitfield_t	bits		= _assignedBits[chunk_idx].load( EMemoryOrder::Relaxed );
				return not (bits & mask);
			}
			return false;
		}


		// Read access by same index is safe,
		// but write access must be externally synchronized.
		ND_ Value_t&  operator [] (Index_t index)
		{
			ASSERT( index < Capacity() );
			ASSERT( IsAssigned( index ));

			const uint		chunk_idx	= index / ChunkSize;
			const uint		bit_idx		= index % ChunkSize;
			ValueChunk_t*	chunk		= _values[chunk_idx].load( EMemoryOrder::Relaxed );

			ASSERT( chunk );
			return (*chunk)[ bit_idx ];
		}


		ND_ size_t  AssignedBitsCount ()
		{
			size_t	count = 0;
			for (size_t i = 0; i < (MaxChunks * AtomicsCount); ++i)
			{
				Bitfield_t	bits = _assignedBits[i].load( EMemoryOrder::Relaxed );

				count += BitCount( ~bits );	// count of zeros
			}
			return count;
		}
		

		ND_ size_t  CreatedObjectsCount ()
		{
			size_t	count = 0;
			for (size_t i = 0; i < (MaxChunks * AtomicsCount); ++i)
			{
				Bitfield_t	bits = _createdBits[i].load( EMemoryOrder::Relaxed );

				count += BitCount( bits );
			}
			return count;
		}


		ND_ BytesU  DynamicSize () const
		{
			BytesU	result;
			for (size_t i = 0; i < MaxChunks; ++i)
			{
				if ( _values[i].load( EMemoryOrder::Relaxed ) != null )
					result += sizeof(ValueChunk_t);
			}
			return result;
		}


		ND_ static constexpr size_t  Capacity ()
		{
			return ChunkSize * MaxChunks;
		}
	};


}	// AE::Threading
