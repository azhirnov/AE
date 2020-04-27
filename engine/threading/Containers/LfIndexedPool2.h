// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'
/*
	This lock-free container has some limitations:
	- write access by same index must be externally synchronized.
	- read access is safe after flush & invalidation (synchronization).
	- Release() must be externally synchronized with all other read and write accesses.
	- Unassign() must be externally synchronized with all other read and write accesses to the 'index'.
	- custom allocator must be thread safe (use mutex or lock-free algorithms).

	This lock-free container designed for large number of elements.
*/

#pragma once

#ifndef AE_LFAS_ENABLED
# include "stl/Containers/FixedArray.h"
# include "stl/CompileTime/Math.h"
# include "stl/Math/BitMath.h"
# include "stl/Memory/UntypedAllocator.h"
# include "threading/Primitives/SpinLock.h"
#endif

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
		STATIC_ASSERT( ToBitMask<size_t>(sizeof(IndexType)*8) >= (ChunkSize_v * MaxChunks_v) );

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
		
		static constexpr HiLevelBits_t	MaxHighLevel		= ToBitMask<HiLevelBits_t>( HiLevel_Count );
		static constexpr HiLevelBits_t	InitialHighLevel	= ~MaxHighLevel;

		static constexpr TopLevelBits_t	MaxTopLevel			= ToBitMask<TopLevelBits_t>( MaxChunks );
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
		~LfIndexedPool2 ()												{ Release(); }
	
		template <typename FN>
		void Release (FN &&dtor, bool checkForAssigned);
		void Release (bool checkForAssigned = true)						{ return Release( [](Value_t& value) { value.~Value_t(); }, checkForAssigned ); }
		
		template <typename FN>
		ND_ bool  Assign (OUT Index_t &outIndex, FN &&ctor);
		ND_ bool  Assign (OUT Index_t &outIndex)						{ return Assign( OUT outIndex, [](Value_t* ptr, Index_t) { PlacementNew<Value_t>( ptr ); }); }
		
		template <typename FN>
		ND_ bool  AssignAt (Index_t index, OUT Value_t* &outValue, FN &&ctor);
		ND_ bool  AssignAt (Index_t index, OUT Value_t* &outValue)		{ return AssignAt( index, OUT outValue, [](Value_t* ptr, Index_t) { PlacementNew<Value_t>( ptr ); }); }

			bool  Unassign (Index_t index);
	
		ND_ bool  IsAssigned (Index_t index);

		ND_ Value_t&  operator [] (Index_t index);
		
		ND_ static constexpr size_t  capacity ()						{ return ChunkSize * MaxChunks; }
		
		ND_ static constexpr BytesU  MaxSize ()							{ return (MaxChunks * SizeOf<ValueChunk_t>) + sizeof(*this); }


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
	inline void  LfIndexedPool2<V,I,CS,MC,A>::Release (FN &&dtor, bool checkForAssigned)
	{
		EXLOCK( _topLevelGuard );
		ThreadFence( EMemoryOrder::Acquire );

		TopLevelBits_t	old_top_level = _topLevel.exchange( InitialTopLevel, EMemoryOrder::Relaxed );
		
		if ( checkForAssigned )
			CHECK( old_top_level == InitialTopLevel );	// some items is still assigned

		for (size_t i = 0; i < MaxChunks; ++i)
		{
			ValueChunk_t*	data = _chunkData[i].exchange( null, EMemoryOrder::Relaxed );
			auto&			info = _chunkInfo[i];

			if ( not data )
				continue;

			EXLOCK( info.guard );	// TODO: may be deadlock

			HiLevelBits_t	old_hi_level = info.hiLevel.exchange( InitialHighLevel, EMemoryOrder::Relaxed );
			
			if ( checkForAssigned )
				CHECK( old_hi_level == InitialHighLevel );	// some items is still assigned

			for (size_t j = 0; j < HiLevel_Count; ++j)
			{
				LowLevelBits_t	old_low_level = info.lowLevel[j].exchange( 0, EMemoryOrder::Relaxed );
				
				if ( checkForAssigned )
					CHECK( old_low_level == 0 );	// some items is still assigned

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
				ASSERT( size_t(idx) < _chunkInfo.size() );

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
		if_unlikely( data == null )
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
			HiLevelBits_t	available	= ~info.hiLevel.load( EMemoryOrder::Relaxed );
			int				idx			= BitScanForward( available );

			for (; idx >= 0;)
			{
				ASSERT( size_t(idx) < info.lowLevel.size() );

				if ( _AssignInLowLevel( OUT outIndex, chunkIndex, idx, *data, ctor ))
					return true;

				available &= ~(HiLevelBits_t(1) << idx);
				idx = BitScanForward( available );
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
				if_unlikely( available == ~mask )
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
		if_unlikely( level.load( EMemoryOrder::Relaxed ) == UMax )
		{
			const auto	hi_bit = (HiLevelBits_t(1) << hiLevelIndex);

			EXLOCK( _topLevelGuard );

			// update top level
			if_unlikely( info.hiLevel.fetch_or( hi_bit, EMemoryOrder::Relaxed ) == ~hi_bit )	// 0 -> 1
			{
				_topLevel.fetch_or( (TopLevelBits_t(1) << chunkIndex), EMemoryOrder::Relaxed );	// 0 -> 1
			}
		}
	}
	
/*
=================================================
	AssignAt
=================================================
*/
	template <typename V, typename I, size_t CS, size_t MC, typename A>
	template <typename FN>
	inline bool  LfIndexedPool2<V,I,CS,MC,A>::AssignAt (Index_t index, OUT Value_t* &outValue, FN &&ctor)
	{
		if_unlikely( index >= capacity() )
			return false;
		
		const uint		chunk_idx	= index / ChunkSize;
		const uint		arr_idx		= index % ChunkSize;
		const uint		hi_lvl_idx	= arr_idx / LowLevel_Count;
		const uint		low_lvl_idx	= arr_idx % LowLevel_Count;
		LowLevelBits_t	mask		= LowLevelBits_t(1) << low_lvl_idx;
		auto&			info		= _chunkInfo[ chunk_idx ];
		auto&			created		= info.lowLevel[ hi_lvl_idx ];
		auto&			level		= info.lowLevel[ hi_lvl_idx ];
		ValueChunk_t*	data		= _chunkData[ chunk_idx ].load( EMemoryOrder::Relaxed );
		
		// allocate
		if_unlikely( data == null )
		{
			data = Cast<ValueChunk_t>(_allocator.Allocate( SizeOf<ValueChunk_t>, AlignOf<ValueChunk_t> ));

			// set new pointer and invalidate cache
			for (ValueChunk_t* expected = null;
				 not _chunkData[ chunk_idx ].compare_exchange_weak( INOUT expected, data, EMemoryOrder::Acquire );)
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

		LowLevelBits_t	old_bits = level.fetch_or( mask, EMemoryOrder::Relaxed );	// 0 -> 1

		if ( (old_bits & mask) )
		{
			// another thread has been assign this bit.
			// wait until the value is initialized.
			for (uint i = 0; (created.load( EMemoryOrder::Relaxed ) & mask) != mask; ++i)
			{
				if ( i > 1000 )
				{
					i = 0;
					std::this_thread::yield();
				}
			}

			// invalidate cache to make visible all changes on value
			ThreadFence( EMemoryOrder::Acquire );
		}
		else
		{
			// if assigned
			if_unlikely( old_bits == ~mask )
				_UpdateHiLevel( chunk_idx, hi_lvl_idx );

			if ( not (created.fetch_or( mask, EMemoryOrder::Relaxed ) & mask) )	// 0 -> 1
			{
				ctor( &data[arr_idx], index );

				// other threads may access by this index without sync, only with cache invalidation
				ThreadFence( EMemoryOrder::Release );
			}
			else
			{
				// already created.
				// invalidate cache to make visible all changes on value.
				ThreadFence( EMemoryOrder::Acquire );
			}
		}
		
		outValue = &data[arr_idx];
		return true;
	}

/*
=================================================
	Unassign
=================================================
*/
	template <typename V, typename I, size_t CS, size_t MC, typename A>
	inline bool  LfIndexedPool2<V,I,CS,MC,A>::Unassign (Index_t index)
	{
		if_unlikely( index >= capacity() )
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
		if_unlikely( old_bits == UMax )
		{
			EXLOCK( info.guard );

			if_unlikely( level.load( EMemoryOrder::Relaxed ) != UMax )
			{
				const auto	hi_bit = (HiLevelBits_t(1) << hi_lvl_idx);

				EXLOCK( _topLevelGuard );
				
				// update top level
				if_unlikely( info.hiLevel.fetch_and( ~hi_bit, EMemoryOrder::Relaxed ) == (hi_bit | InitialHighLevel) )	// 1 -> 0
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
		if ( index < capacity() )
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
		ASSERT( index < capacity() );
		ASSERT( IsAssigned( index ));
		
		const uint		chunk_idx	= index / ChunkSize;
		const uint		elem_idx	= index % ChunkSize;
		ValueChunk_t*	chunk		= _chunkData[ chunk_idx ].load( EMemoryOrder::Relaxed );

		ASSERT( chunk );
		return (*chunk)[ elem_idx ];
	}


}	// AE::Threading
