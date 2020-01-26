// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'
/*
	This lock-free container has some limitations:
	- write access by same index must be externally synchronized.
	- read access is safe after flush & invalidation (synchronization).
	- Release() must be externally synchronized with all other read and write accesses.
	- Unassign() must be externally synchronized with all other read and write accesses to the 'index'.
	- custom allocator must be thread safe (use mutex or lock-free algorithms).

	This lock-free container designed for large number of data.
*/

#pragma once

#include "stl/Containers/FixedArray.h"
#include "stl/CompileTime/Math.h"
#include "stl/Math/BitMath.h"
#include "stl/Memory/UntypedAllocator.h"

#include "threading/Primitives/SpinLock.h"

namespace AE::Threading
{
	
	//
	// Lock-Free Indexed Pool
	//

	template <typename ValueType,
			  typename IndexType,
			  size_t ChunkSize_v = 256,
			  size_t MaxChunks_v = 16,
			  typename AllocatorType = UntypedAlignedAllocator
			 >
	struct LfIndexedPool2 final
	{
		STATIC_ASSERT( ChunkSize_v > 0 );
		STATIC_ASSERT( ((ChunkSize_v % 32) == 0) or ((ChunkSize_v % 64) == 0) );
		STATIC_ASSERT( ChunkSize_v <= 64*64 );
		STATIC_ASSERT( MaxChunks_v > 0 and MaxChunks_v <= 64 );
		STATIC_ASSERT( IsPowerOfTwo( ChunkSize_v ));	// must be power of 2 to increase performance
		STATIC_ASSERT( AllocatorType::IsThreadSafe );

	// types
	public:
		using Self				= LfIndexedPool2< ValueType, IndexType, ChunkSize_v, MaxChunks_v, AllocatorType >;
		using Index_t			= IndexType;
		using Value_t			= ValueType;
		using Allocator_t		= AllocatorType;

	private:
		static constexpr size_t	ChunkSize		= ChunkSize_v;
		static constexpr size_t	MaxChunks		= MaxChunks_v;
		static constexpr size_t	LowLevel_Count	= (ChunkSize <= 32 ? 32 : 64);
		static constexpr size_t	HiLevel_Count	= Max( 1u, (ChunkSize + LowLevel_Count - 1) / LowLevel_Count );
		static constexpr uint	TopWaitCount	= 8;
		static constexpr uint	HighWaitCount	= 8;

		using LowLevelBits_t	= Conditional< (LowLevel_Count <= 32), uint, uint64_t >;
		using LowLevels_t		= StaticArray< Atomic< LowLevelBits_t >, HiLevel_Count >;
		using HiLevelBits_t		= Conditional< (HiLevel_Count <= 32), uint, uint64_t >;

		struct alignas(AE_CACHE_LINE) ChunkInfo
		{
			SpinLockRelaxed				guard;		// only for 'hiLevel' modification
			Atomic< HiLevelBits_t >		hiLevel;	// 0 - is unassigned bit, 1 - assigned bit
			LowLevels_t					lowLevel;	// 0 - is unassigned bit, 1 - assigned bit
			LowLevels_t					created;	// 0 - uninitialized bit, 1 - constructor has been called
		};

		using ChunkInfos_t		= StaticArray< ChunkInfo, MaxChunks >;
		using ValueChunk_t		= StaticArray< Value_t, ChunkSize >;
		using ChunkData_t		= StaticArray< Atomic< ValueChunk_t *>, MaxChunks >;
		using TopLevelBits_t	= Conditional< (MaxChunks <= 32), uint, uint64_t >;

		STATIC_ASSERT( LowLevels_t::value_type::is_always_lock_free );
		STATIC_ASSERT( decltype(ChunkInfo::hiLevel)::is_always_lock_free );
		STATIC_ASSERT( ChunkData_t::value_type::is_always_lock_free );
		STATIC_ASSERT( Atomic< TopLevelBits_t >::is_always_lock_free );
		
		static constexpr HiLevelBits_t	MaxHighLevel		= (HiLevelBits_t(1) << HiLevel_Count) - 1;
		static constexpr HiLevelBits_t	InitialHighLevel	= ~MaxHighLevel;

		static constexpr TopLevelBits_t	MaxTopLevel			= (TopLevelBits_t(1) << MaxChunks) - 1;
		static constexpr TopLevelBits_t	InitialTopLevel		= ~MaxTopLevel;


	// variables
	private:
		alignas(AE_CACHE_LINE) SpinLockRelaxed	_topLevelGuard;		// only for '_topLevel' modification
		Atomic< TopLevelBits_t >				_topLevel;			// 0 - is unassigned bit, 1 - assigned bit

		ChunkInfos_t	_chunkInfo;
		ChunkData_t		_chunkData;
		Allocator_t		_allocator;


	// methods
	public:
		LfIndexedPool2 (const Self &) = delete;
		LfIndexedPool2 (Self &&) = delete;

		Self& operator = (const Self &) = delete;
		Self& operator = (Self &&) = delete;
		

		explicit LfIndexedPool2 (const Allocator_t &alloc = Default);
		~LfIndexedPool2 ()													{ Release(); }
	
		template <typename FN>
		void Release (FN &&dtor);
		void Release ()														{ return Release([] (Value_t& value) { value.~Value_t(); }); }
		
		template <typename FN>
		ND_ bool  Assign (OUT Index_t &outIndex, FN &&ctor);
		ND_ bool  Assign (OUT Index_t &outIndex)							{ return Assign( OUT outIndex, [](Value_t* ptr, Index_t) { PlacementNew<Value_t>( ptr ); }); }

			bool  Unassign (Index_t index);
	
		ND_ bool  IsAssigned (Index_t index);

		ND_ Value_t&  operator [] (Index_t index);
		
		ND_ static constexpr size_t  Capacity ()							{ return ChunkSize * MaxChunks; }


	private:
		template <typename FN>
		bool  _AssignInChunk (OUT Index_t &outIndex, int chunkIndex, const FN &ctor);

		template <typename FN>
		bool  _AssignInLowLevel (OUT Index_t &outIndex, int chunkIndex, int hiLevelIndex, ValueChunk_t &data, const FN &ctor);

		void _UpdateHiLevel (int chunkIndex, int hiLevelIndex);
	};
	

	
/*
=================================================
	constructor
=================================================
*/
	template <typename V, typename I, size_t CS, size_t MC, typename A>
	inline LfIndexedPool2<V,I,CS,MC,A>::LfIndexedPool2 (const Allocator_t &alloc) :
		_allocator{ alloc }
	{
		_topLevel.store( InitialTopLevel, EMemoryOrder::Relaxed );

		for (size_t i = 0; i < MaxChunks; ++i)
		{
			auto&	info = _chunkInfo[i];

			for (size_t j = 0; j < HiLevel_Count; ++j)
			{
				info.lowLevel[j].store( 0, EMemoryOrder::Relaxed );
				info.created[j].store( 0, EMemoryOrder::Relaxed );
			}

			info.hiLevel.store( InitialHighLevel, EMemoryOrder::Relaxed );

			_chunkData[i].store( null, EMemoryOrder::Relaxed );
		}

		ThreadFence( EMemoryOrder::Release );
	}

/*
=================================================
	Release
----
	Must be externally synchronized
=================================================
*/
	template <typename V, typename I, size_t CS, size_t MC, typename A>
	template <typename FN>
	inline void  LfIndexedPool2<V,I,CS,MC,A>::Release (FN &&dtor)
	{
		EXLOCK( _topLevelGuard );
		ThreadFence( EMemoryOrder::Acquire );

		TopLevelBits_t	old_top_level = _topLevel.exchange( InitialTopLevel, EMemoryOrder::Relaxed );
		Unused( old_top_level );
		ASSERT( old_top_level == InitialTopLevel );

		for (size_t i = 0; i < MaxChunks; ++i)
		{
			ValueChunk_t*	data = _chunkData[i].exchange( null, EMemoryOrder::Relaxed );
			auto&			info = _chunkInfo[i];

			if ( not data )
				continue;

			EXLOCK( info.guard );	// TODO: may be deadlock

			HiLevelBits_t	old_hi_level = info.hiLevel.exchange( InitialHighLevel, EMemoryOrder::Relaxed );
			Unused( old_hi_level );
			ASSERT( old_hi_level == InitialHighLevel );

			for (size_t j = 0; j < HiLevel_Count; ++j)
			{
				LowLevelBits_t	old_low_level = info.lowLevel[j].exchange( 0, EMemoryOrder::Relaxed );
				Unused( old_low_level );
				ASSERT( old_low_level == 0 );

				LowLevelBits_t	ctor_bits = info.created[j].exchange( 0, EMemoryOrder::Relaxed );

				// call destructor
				for (size_t c = 0; c < LowLevel_Count; ++c)
				{
					if ( ctor_bits & (LowLevelBits_t(1) << c) )
						dtor( (*data)[ c + j*LowLevel_Count ] );
				}
			}
					
			_allocator.Deallocate( data, SizeOf<ValueChunk_t>, AlignOf<ValueChunk_t> );
		}
			
		ThreadFence( EMemoryOrder::Release );
	}
		
/*
=================================================
	Assign
=================================================
*/
	template <typename V, typename I, size_t CS, size_t MC, typename A>
	template <typename FN>
	inline bool  LfIndexedPool2<V,I,CS,MC,A>::Assign (OUT Index_t &outIndex, FN &&ctor)
	{
		for (uint j = 0; j < TopWaitCount; ++j)
		{
			TopLevelBits_t	available	= _topLevel.load( EMemoryOrder::Relaxed );
			int				idx			= BitScanForward( ~available );

			if ( idx >= 0 )
			{
				ASSERT( idx < _chunkInfo.size() );

				if ( _AssignInChunk( OUT outIndex, idx, ctor ))
					return true;
			}
		}
		return false;
	}
	
/*
=================================================
	_AssignInChunk
=================================================
*/
	template <typename V, typename I, size_t CS, size_t MC, typename A>
	template <typename FN>
	inline bool  LfIndexedPool2<V,I,CS,MC,A>::_AssignInChunk (OUT Index_t &outIndex, int chunkIndex, const FN &ctor)
	{
		auto&			info = _chunkInfo[ chunkIndex ];
		ValueChunk_t*	data = _chunkData[ chunkIndex ].load( EMemoryOrder::Relaxed );
			
		// allocate
		if ( data == null )
		{
			data = Cast<ValueChunk_t>(_allocator.Allocate( SizeOf<ValueChunk_t>, AlignOf<ValueChunk_t> ));

			// set new pointer and invalidate cache
			for (ValueChunk_t* expected = null;
				 not _chunkData[ chunkIndex ].compare_exchange_weak( INOUT expected, data, EMemoryOrder::Acquire );)
			{
				// another thread has been allocated this chunk
				if ( expected != null and expected != data )
				{
					_allocator.Deallocate( data, SizeOf<ValueChunk_t>, AlignOf<ValueChunk_t> );
					data = expected;
					break;
				}
			}
		}
			
		// find available index
		for (uint j = 0; j < HighWaitCount; ++j)
		{
			HiLevelBits_t	available	= info.hiLevel.load( EMemoryOrder::Relaxed );
			int				idx			= BitScanForward( ~available );

			if ( idx >= 0 )
			{
				ASSERT( idx < info.lowLevel.size() );

				if ( _AssignInLowLevel( OUT outIndex, chunkIndex, idx, *data, ctor ))
					return true;
			}
		}
		return false;
	}
	
/*
=================================================
	_AssignInLowLevel
=================================================
*/
	template <typename V, typename I, size_t CS, size_t MC, typename A>
	template <typename FN>
	inline bool  LfIndexedPool2<V,I,CS,MC,A>::_AssignInLowLevel (OUT Index_t &outIndex, int chunkIndex, int hiLevelIndex, ValueChunk_t &data, const FN &ctor)
	{
		auto&			info		= _chunkInfo[ chunkIndex ];
		auto&			level		= info.lowLevel[ hiLevelIndex ];
		auto&			created		= info.created[ hiLevelIndex ];
		LowLevelBits_t	available	= level.load( EMemoryOrder::Relaxed );
			
		for (int i = BitScanForward( ~available ); i >= 0;)
		{
			const LowLevelBits_t	mask = (LowLevelBits_t(1) << i);

			if ( level.compare_exchange_weak( INOUT available, available | mask, EMemoryOrder::Acquire, EMemoryOrder::Relaxed ))	// 0 -> 1
			{
				if ( available == ~mask )
					_UpdateHiLevel( chunkIndex, hiLevelIndex );

				outIndex = Index_t(i) | (Index_t(hiLevelIndex) * LowLevel_Count) | (Index_t(chunkIndex) * ChunkSize);

				if ( not (created.fetch_or( mask, EMemoryOrder::Relaxed ) & mask) )	// 0 -> 1
				{
					ctor( &data[ i + hiLevelIndex * LowLevel_Count ], outIndex );
				}
				return true;
			}

			i = BitScanForward( ~available );
		}
		return false;
	}
	
/*
=================================================
	_UpdateHiLevel
=================================================
*/
	template <typename V, typename I, size_t CS, size_t MC, typename A>
	inline void  LfIndexedPool2<V,I,CS,MC,A>::_UpdateHiLevel (int chunkIndex, int hiLevelIndex)
	{
		auto&	info	= _chunkInfo[ chunkIndex ];
		auto&	level	= info.lowLevel[ hiLevelIndex ];

		EXLOCK( info.guard );
				
		// update high level
		if ( level.load( EMemoryOrder::Relaxed ) == UMax )
		{
			const auto	hi_bit = (HiLevelBits_t(1) << hiLevelIndex);

			EXLOCK( _topLevelGuard );

			// update top level
			if ( info.hiLevel.fetch_or( hi_bit, EMemoryOrder::Relaxed ) == ~hi_bit )	// 0 -> 1
			{
				_topLevel.fetch_or( (TopLevelBits_t(1) << chunkIndex), EMemoryOrder::Relaxed );	// 0 -> 1
			}
		}
	}
	
/*
=================================================
	Unassign
=================================================
*/
	template <typename V, typename I, size_t CS, size_t MC, typename A>
	inline bool  LfIndexedPool2<V,I,CS,MC,A>::Unassign (Index_t index)
	{
		if_unlikely( index >= Capacity() )
			return false;
		
		const uint		chunk_idx	= index / ChunkSize;
		const uint		hi_lvl_idx	= (index % ChunkSize) / LowLevel_Count;
		const uint		low_lvl_idx	= (index % ChunkSize) % LowLevel_Count;
		LowLevelBits_t	mask		= LowLevelBits_t(1) << low_lvl_idx;
		auto&			info		= _chunkInfo[ chunk_idx ];
		auto&			level		= info.lowLevel[ hi_lvl_idx ];
		LowLevelBits_t	old_bits	= level.fetch_and( ~mask, EMemoryOrder::Relaxed );	// 1 -> 0

		if ( not (old_bits & mask) )
			return false;

		// update high level bits
		if ( old_bits == UMax )
		{
			EXLOCK( info.guard );

			if ( level.load( EMemoryOrder::Relaxed ) != UMax )
			{
				const auto	hi_bit = (HiLevelBits_t(1) << hi_lvl_idx);

				EXLOCK( _topLevelGuard );
				
				// update top level
				if ( info.hiLevel.fetch_and( ~hi_bit, EMemoryOrder::Relaxed ) == (hi_bit | InitialHighLevel) )	// 1 -> 0
				{
					_topLevel.fetch_and( ~(TopLevelBits_t(1) << chunk_idx), EMemoryOrder::Relaxed );	// 1 -> 0
				}
			}
		}

		return true;
	}
	
/*
=================================================
	IsAssigned
=================================================
*/
	template <typename V, typename I, size_t CS, size_t MC, typename A>
	inline bool  LfIndexedPool2<V,I,CS,MC,A>::IsAssigned (Index_t index)
	{
		if ( index < Capacity() )
		{
			const uint		chunk_idx	= index / ChunkSize;
			const uint		elem_idx	= (index % ChunkSize) / LowLevel_Count;
			const uint		bit_idx		= (index % ChunkSize) % LowLevel_Count;
			LowLevelBits_t	mask		= LowLevelBits_t(1) << bit_idx;
			LowLevelBits_t	bits		= _chunkInfo[ chunk_idx ].lowLevel[ elem_idx ].load( EMemoryOrder::Relaxed );
			return (bits & mask);
		}
		return false;
	}
	
/*
=================================================
	operator []
----
	you must invalidate cache before reading data
	and flush cache after writing
=================================================
*/
	template <typename V, typename I, size_t CS, size_t MC, typename A>
	inline V&  LfIndexedPool2<V,I,CS,MC,A>::operator [] (Index_t index)
	{
		ASSERT( index < Capacity() );
		ASSERT( IsAssigned( index ));
		
		const uint		chunk_idx	= index / ChunkSize;
		const uint		elem_idx	= index % ChunkSize;
		ValueChunk_t*	chunk		= _chunkData[ chunk_idx ].load( EMemoryOrder::Relaxed );

		ASSERT( chunk );
		return (*chunk)[ elem_idx ];
	}


}	// AE::Threading
