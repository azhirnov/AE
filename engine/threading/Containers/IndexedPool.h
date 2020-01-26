// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'
/*
	TODO: use LfIndexedPool instead
*/

#pragma once

#include "stl/Containers/FixedArray.h"
#include "stl/CompileTime/Math.h"
#include "stl/Memory/UntypedAllocator.h"
#include "threading/Common.h"

namespace AE::Threading
{

	//
	// Indexed Pool
	//
	
	template <typename ValueType,
			  typename IndexType,
			  size_t ChunkSize,
			  size_t MaxChunks = 16,
			  typename AllocatorType = UntypedAlignedAllocator
			 >
	struct IndexedPool final
	{
		STATIC_ASSERT( ChunkSize > 0 and MaxChunks > 0 );
		STATIC_ASSERT( IsPowerOfTwo( ChunkSize ) );	// must be power of 2 for best performance

	// types
	public:
		using Self			= IndexedPool< ValueType, IndexType, ChunkSize, MaxChunks, AllocatorType >;
		using Index_t		= IndexType;
		using Value_t		= ValueType;
		using Allocator_t	= AllocatorType;

	private:
		static constexpr size_t	MaxSize = ChunkSize * MaxChunks;

		using RawIndex_t		= Conditional< (MaxSize > std::numeric_limits<uint32_t>::max()), uint64_t, 
									Conditional< (MaxSize > std::numeric_limits<uint16_t>::max()), uint32_t, uint16_t >>;

		using IndexCountArray_t	= FixedArray< RawIndex_t, MaxChunks >;
		using IndexChunk		= StaticArray< RawIndex_t, ChunkSize >;
		using ValueChunk		= StaticArray< Value_t, ChunkSize >;

		using IndexChunks_t		= StaticArray< IndexChunk*, MaxChunks >;
		using ValueChunks_t		= StaticArray< Atomic<ValueChunk *>, MaxChunks >;

		STATIC_ASSERT( ValueChunks_t::value_type::is_always_lock_free );


	// variables
	private:
		mutable Mutex			_assignOpGuard;
		IndexCountArray_t		_indexCount;
		IndexChunks_t			_indices;
		ValueChunks_t			_values;
		Allocator_t				_alloc;


	// methods
	public:
		IndexedPool (const Self &) = delete;
		IndexedPool (Self &&) = default;

		Self& operator = (const Self &) = delete;
		Self& operator = (Self &&) = default;


		explicit IndexedPool (const Allocator_t &alloc = Allocator_t()) :
			_alloc{ alloc }
		{
			EXLOCK( _assignOpGuard );

			_indices.fill( null );

			for (auto& chunk : _values) {
				chunk.store( null, EMemoryOrder::Relaxed );
			}

			_indexCount.push_back( ChunkSize );
			_CreateChunk( 0 );
		}

		~IndexedPool()
		{
			Release();
		}

		
		void Release ()
		{
			EXLOCK( _assignOpGuard );

			_indexCount.clear();

			for (auto& chunk : _indices)
			{
				chunk->~IndexChunk();
				_alloc.Deallocate( chunk, SizeOf<IndexChunk>, AlignOf<IndexChunk> );
				chunk = null;
			}

			for (auto& chunk : _values)
			{
				auto*	value = chunk.load( EMemoryOrder::Relaxed );
				if ( value ) {
					value->~ValueChunk();
					_alloc.Deallocate( value, SizeOf<ValueChunk>, AlignOf<ValueChunk> );
					chunk.store( null, EMemoryOrder::Relaxed );
				}
			}
		}


		void Swap (Self &other)
		{
			std::scoped_lock	lock{ _assignOpGuard, other._assignOpGuard };

			CHECK( _alloc == other._alloc );
			std::swap( _indexCount,	other._indexCount );
			std::swap( _indices,	other._indices );
			std::swap( _values,		other._values );
		}
		

		template <typename ArrayType>
		ND_ size_t  Assign (size_t numIndices, INOUT ArrayType &arr)
		{
			EXLOCK( _assignOpGuard );

			numIndices = Min( numIndices, arr.capacity() - arr.size(), ChunkSize );
			ASSERT( numIndices > 0 );

			size_t	result = 0;

			for (size_t i = 0, count = _indexCount.size();
				 i < count and result < numIndices;
				 ++i)
			{
				auto&			idx_count	= _indexCount[i];
				auto&			indices		= *_indices[i];
				const size_t	cnt			= Min( numIndices - result, size_t(idx_count) );
				idx_count -= RawIndex_t(cnt);

				for (size_t j = 0; j < cnt; ++j, ++result)
				{
					arr.emplace_back( indices[idx_count + j] );
				}
			}

			if ( result >= numIndices )
				return result;

			if ( _indexCount.size() == _indexCount.capacity() )
				return result;

			const size_t	chunk_idx = _indexCount.size();
			_indexCount.push_back( ChunkSize );
			_CreateChunk( chunk_idx );
		
			{
				auto&			idx_count	= _indexCount [chunk_idx];
				auto&			indices		= *_indices [chunk_idx];
				const size_t	cnt			= Min( numIndices - result, size_t(idx_count) );
				idx_count -= RawIndex_t(cnt);

				for (size_t j = 0; j < cnt; ++j, ++result)
				{
					arr.emplace_back( indices[idx_count + j] );
				}
			}
			return result;
		}


		ND_ bool  Assign (OUT Index_t &index)
		{
			EXLOCK( _assignOpGuard );

			for (size_t i = 0, count = _indexCount.size(); i < count; ++i)
			{
				auto&	idx_count = _indexCount[i];

				if ( idx_count > 0 )
				{
					ASSERT( _indices[i] );
					index = (*_indices [i])[ --idx_count ];
					return true;
				}
			}

			if ( _indexCount.size() == _indexCount.capacity() ) {
				ASSERT(!"out of memory!");
				return false;
			}

			const size_t	chunk_idx = _indexCount.size();
			_indexCount.push_back( ChunkSize );
			_CreateChunk( chunk_idx );

			index = (*_indices [chunk_idx])[ --_indexCount[chunk_idx] ];
			return true;
		}
		

		template <typename ArrayType>
		void  Unassign (size_t count, INOUT ArrayType &arr)
		{
			EXLOCK( _assignOpGuard );

			count = Min( count, arr.size() );

			// unassign the last 'count' indices
			for (size_t i = 0, j = arr.size() - count; i < count; ++i, ++j)
			{
				_Unassign( arr[j] );
			}

			arr.resize( arr.size() - count );
		}


		void  Unassign (Index_t index)
		{
			EXLOCK( _assignOpGuard );
			return _Unassign( index );
		}

		
		ND_ Value_t&  operator [] (Index_t index)
		{
			const Index_t	chunk_idx	= index / ChunkSize;
			const Index_t	idx			= index % ChunkSize;
			ASSERT( chunk_idx < _indexCount.size() );

			auto*	val_chunk = _values[chunk_idx].load( EMemoryOrder::Relaxed );
			ASSERT( val_chunk );

			return (*val_chunk) [idx];
		}


		ND_ Value_t const&	operator [] (Index_t index) const
		{
			const Index_t	chunk_idx	= index / ChunkSize;
			const Index_t	idx			= index % ChunkSize;
			ASSERT( chunk_idx < _indexCount.size() );

			auto*	val_chunk = _values[chunk_idx].load( EMemoryOrder::Relaxed );
			ASSERT( val_chunk );

			return (*val_chunk) [idx];
		}


		ND_ size_t  size () const
		{
			EXLOCK( _assignOpGuard );
			return _indexCount.size() * ChunkSize;
		}

		ND_ static constexpr size_t  capacity ()
		{
			return MaxChunks * ChunkSize;
		}

		ND_ bool  empty () const
		{
			EXLOCK( _assignOpGuard );

			for (auto cnt : _indexCount)
			{
				if ( cnt != ChunkSize )
					return false;
			}
			return true;
		}


		ND_ BytesU  DynamicSize () const
		{
			EXLOCK( _assignOpGuard );
			BytesU	sz { sizeof(*this) };

			for (auto& idx : _indices) {
				sz += (idx ? sizeof(*idx) : 0);
			}
			for (auto& chunk : _values) {
				sz += (chunk.load( EMemoryOrder::Relaxed ) ? sizeof(ValueChunk) : 0);
			}
			return sz;
		}


	private:
		void _CreateChunk (size_t chunkIndex)
		{
			// '_indices' must be protected by '_assignOpGuard'

			auto*	idx_chunk = Cast<IndexChunk>(_alloc.Allocate( SizeOf<IndexChunk>, AlignOf<IndexChunk> ));
			_indices[ chunkIndex ] = idx_chunk;

			for (size_t i = 0; i < ChunkSize; ++i)
			{
				size_t	idx = (ChunkSize-1 - i) + (chunkIndex * ChunkSize);
				ASSERT( idx <= std::numeric_limits<RawIndex_t>::max() );

				(*idx_chunk)[i] = RawIndex_t(idx);
			}
		
			// '_alloc' must be protected by '_assignOpGuard'
			// '_values' chunk allocation protected by '_assignOpGuard',
			// but '_values' reading is not protected, so we need to use atomic store operation
			ASSERT( _values[ chunkIndex ].load( EMemoryOrder::Relaxed ) == null );

			auto*	val_chunk = Cast<ValueChunk>(_alloc.Allocate( SizeOf<ValueChunk>, AlignOf<ValueChunk> ));
			PlacementNew<ValueChunk>( val_chunk );
			_values[ chunkIndex ].store( val_chunk, EMemoryOrder::Relaxed );
		}


		void _Unassign (Index_t index)
		{
			const Index_t	chunk_idx = index / ChunkSize;

			ASSERT( chunk_idx < _indexCount.size() );
			ASSERT( index >= (chunk_idx * ChunkSize) and index < ((chunk_idx+1) * ChunkSize) );

			auto&	idx_count	= _indexCount[chunk_idx];
			auto&	indices		= *_indices [chunk_idx];
			ASSERT( idx_count < ChunkSize );

			DEBUG_ONLY(
			for (size_t i = 0; i < idx_count; ++i) {
				ASSERT( indices[i] != index );
			})

			indices[ idx_count++ ] = RawIndex_t(index);
		}
	};
	

}	// AE::Threading
