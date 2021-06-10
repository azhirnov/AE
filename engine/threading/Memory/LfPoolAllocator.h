// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'
/*
	Same as LfIndexedPool2, but for memory allocation.
	Also designed to support GPU memory allocation (for Vulkan).
*/

#pragma once
#if 0
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
	// Default Chunk Allocator
	//

	template <usize ElementSize,
			  usize MemAlign,
			  typename AllocatorType = UntypedAlignedAllocator
			 >
	struct LfPoolAllocator_ChunkAllocator
	{
	// types
		using Self		= LfPoolAllocator_ChunkAllocator< ElementSize, MemAlign, AllocatorType >;
		using Result_t	= void *;

		static constexpr bool	IsThreadSafe = true;

	// methods
		ND_ Result_t  Allocate (usize count)
		{
			return AllocatorType::Allocate( MemAlign, ElementSize * count );
		}

		void Deallocate (Result_t ptr, usize count)
		{
			AllocatorType::Deallocate( ptr, MemAlign, ElementSize * count );
		}
	};



	//
	// Lock-Free Pool Allocator
	//
	
	template <usize ChunkSize_v,
			  usize MaxChunks_v,
			  typename AllocatorType
			 >
	struct LfPoolAllocator final
	{
		STATIC_ASSERT( ChunkSize_v > 0 );
		STATIC_ASSERT( ((ChunkSize_v % 32) == 0) or ((ChunkSize_v % 64) == 0) );
		STATIC_ASSERT( ChunkSize_v <= 64*64 );
		STATIC_ASSERT( MaxChunks_v > 0 and MaxChunks_v <= 64 );
		STATIC_ASSERT( IsPowerOfTwo( ChunkSize_v ));	// must be power of 2 to increase performance
		STATIC_ASSERT( AllocatorType::IsThreadSafe );

	// types
	public:
		using Self				= LfPoolAllocator< ChunkSize_v, MaxChunks_v, AllocatorType >;
		using Index_t			= uint;
		using Allocator_t		= AllocatorType;
		using AllocResult_t		= typename AllocatorType::Result_t;

		struct Result
		{
			Index_t		index;
			uint		size : 4;
		};

	private:
		static constexpr usize	ChunkSize		= ChunkSize_v;
		static constexpr usize	MaxChunks		= MaxChunks_v;
		static constexpr usize	LowLevel_Count	= (ChunkSize <= 32 ? 32 : 64);
		static constexpr usize	HiLevel_Count	= Max( 1u, (ChunkSize + LowLevel_Count - 1) / LowLevel_Count );
		static constexpr uint	TopWaitCount	= 8;
		static constexpr uint	HighWaitCount	= 8;

		using LowLevelBits_t	= Conditional< (LowLevel_Count <= 32), uint, ulong >;
		using LowLevels_t		= StaticArray< Atomic< LowLevelBits_t >, HiLevel_Count >;
		using HiLevelBits_t		= Conditional< (HiLevel_Count <= 32), uint, ulong >;

		struct alignas(AE_CACHE_LINE) ChunkInfo
		{
			SpinLockRelaxed				guard;		// only for 'hiLevel' modification
			Atomic< HiLevelBits_t >		hiLevel;	// 0 - is unassigned bit, 1 - assigned bit
			LowLevels_t					lowLevel;	// 0 - is unassigned bit, 1 - assigned bit
		};

		using ChunkInfos_t		= StaticArray< ChunkInfo, MaxChunks >;
		using ValueChunk_t		= StaticArray< AllocResult_t, ChunkSize >;
		using ChunkData_t		= StaticArray< Atomic< ValueChunk_t *>, MaxChunks >;
		using TopLevelBits_t	= Conditional< (MaxChunks <= 32), uint, ulong >;

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
		LfPoolAllocator (const Self &) = delete;
		LfPoolAllocator (Self &&) = delete;

		Self& operator = (const Self &) = delete;
		Self& operator = (Self &&) = delete;
		
		explicit LfPoolAllocator (const Allocator_t &alloc = Default);
		~LfPoolAllocator ()												{ Release(); }
	
		void Release (bool checkForAssigned = true);
		
		ND_ bool  Assign (Index_t numBlocks, OUT Result &outBlock);
			bool  Unassign (const Result &);
		ND_ bool  IsAssigned (const Result &);
		
		ND_ static constexpr usize  Capacity ()						{ return ChunkSize * MaxChunks; }


	private:
		bool  _AssignInChunk (OUT Result &outBlock, LowLevelBits_t bitMask, int chunkIndex);
		bool  _AssignInLowLevel (OUT Result &outBlock, LowLevelBits_t bitMask, int chunkIndex, int hiLevelIndex);

		void _UpdateHiLevel (int chunkIndex, int hiLevelIndex);
	};
	

	
/*
=================================================
	constructor
=================================================
*/
	template <usize CS, usize MC, typename A>
	inline LfPoolAllocator<CS,MC,A>::LfPoolAllocator (const Allocator_t &alloc) :
		_allocator{ alloc }
	{
		_topLevel.store( InitialTopLevel, EMemoryOrder::Relaxed );

		for (usize i = 0; i < MaxChunks; ++i)
		{
			auto&	info = _chunkInfo[i];

			for (usize j = 0; j < HiLevel_Count; ++j)
			{
				info.lowLevel[j].store( 0, EMemoryOrder::Relaxed );
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
	template <usize CS, usize MC, typename A>
	inline void  LfPoolAllocator<CS,MC,A>::Release (bool checkForAssigned)
	{
		EXLOCK( _topLevelGuard );
		ThreadFence( EMemoryOrder::Acquire );

		TopLevelBits_t	old_top_level = _topLevel.exchange( InitialTopLevel, EMemoryOrder::Relaxed );
		
		if ( checkForAssigned )
			CHECK( old_top_level == InitialTopLevel );	// some items is still assigned

		for (usize i = 0; i < MaxChunks; ++i)
		{
			ValueChunk_t*	data = _chunkData[i].exchange( null, EMemoryOrder::Relaxed );
			auto&			info = _chunkInfo[i];

			if ( not data )
				continue;

			EXLOCK( info.guard );	// TODO: may be deadlock

			HiLevelBits_t	old_hi_level = info.hiLevel.exchange( InitialHighLevel, EMemoryOrder::Relaxed );
			
			if ( checkForAssigned )
				CHECK( old_hi_level == InitialHighLevel );	// some items is still assigned

			for (usize j = 0; j < HiLevel_Count; ++j)
			{
				LowLevelBits_t	old_low_level = info.lowLevel[j].exchange( 0, EMemoryOrder::Relaxed );
				
				if ( checkForAssigned )
					CHECK( old_low_level == 0 );	// some items is still assigned

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
	template <usize CS, usize MC, typename A>
	inline bool  LfPoolAllocator<CS,MC,A>::Assign (Index_t numBlocks, OUT Result &outBlock)
	{
		CHECK_ERR( numBlocks > 0 and numBlocks < 4 );

		const auto	mask = ToBitMask<LowLevelBits_t>( numBlocks );

		for (uint j = 0; j < TopWaitCount; ++j)
		{
			TopLevelBits_t	available	= _topLevel.load( EMemoryOrder::Relaxed );
			int				idx			= BitScanForward( ~available );

			if ( idx >= 0 )
			{
				ASSERT( usize(idx) < _chunkInfo.size() );

				if ( _AssignInChunk( OUT outBlock, mask, idx ))
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
	template <usize CS, usize MC, typename A>
	inline bool  LfPoolAllocator<CS,MC,A>::_AssignInChunk (OUT Result &outBlock, LowLevelBits_t bitMask, int chunkIndex)
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
			HiLevelBits_t	available	= info.hiLevel.load( EMemoryOrder::Relaxed );
			int				idx			= BitScanForward( ~available );

			if ( idx >= 0 )
			{
				ASSERT( usize(idx) < info.lowLevel.size() );

				if ( _AssignInLowLevel( OUT outIndex, bitMask, chunkIndex, idx ))
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
	template <usize CS, usize MC, typename A>
	inline bool  LfPoolAllocator<CS,MC,A>::_AssignInLowLevel (OUT Result &outBlock, LowLevelBits_t bitMask, int chunkIndex, int hiLevelIndex)
	{
		auto&			info		= _chunkInfo[ chunkIndex ];
		auto&			level		= info.lowLevel[ hiLevelIndex ];
		LowLevelBits_t	available	= level.load( EMemoryOrder::Relaxed );
		LowLevelBits_t	inv_bits	= ~available;
			
		for (int i = BitScanForward( inv_bits ); i >= 0;)
		{
			const LowLevelBits_t	mask = (bitMask << i);

			// have enought space?
			if ( (inv_bits & mask) != mask )
			{
				inv_bits |= (LowLevelBits_t(1) << i);
				i		  = BitScanForward( inv_bits );
				continue;
			}

			if ( level.compare_exchange_weak( INOUT available, available | mask, EMemoryOrder::Acquire, EMemoryOrder::Relaxed ))	// 0 -> 1
			{
				if_unlikely( available == ~mask )
					_UpdateHiLevel( chunkIndex, hiLevelIndex );

				outIndex = Index_t(i) | (Index_t(hiLevelIndex) * LowLevel_Count) | (Index_t(chunkIndex) * ChunkSize);

				return true;
			}

			inv_bits = ~available;
			i		 = BitScanForward( inv_bits );
		}
		return false;
	}
	
/*
=================================================
	_UpdateHiLevel
=================================================
*/
	template <usize CS, usize MC, typename A>
	inline void  LfPoolAllocator<CS,MC,A>::_UpdateHiLevel (int chunkIndex, int hiLevelIndex)
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
	Unassign
=================================================
*/
	template <usize CS, usize MC, typename A>
	inline bool  LfPoolAllocator<CS,MC,A>::Unassign (const Result &block)
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
	template <usize CS, usize MC, typename A>
	inline bool  LfPoolAllocator<CS,MC,A>::IsAssigned (const Result &block)
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

}	// AE::Threading
#endif
