// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'
/*
	This class uses lock-free algorithm to put and exteract values without any order (but not really in random order).
	You can use 'Put', 'Extract' methods without any syncs.
	'Clear' method must be synchronized with 'Put' and 'Extract' methods.
*/

#pragma once

#ifndef AE_LFAS_ENABLED
# include "threading/Common.h"
# include "stl/Math/BitMath.h"
# include "stl/Memory/MemUtils.h"
#endif

namespace AE::Threading
{

	//
	// Lock-free Static Pool
	//

	template <typename T, uint Count>
	struct LfStaticPool
	{
		STATIC_ASSERT( Count > 0 );

	// types
	public:
		using Self		= LfStaticPool< T, Count >;
		using Index_t	= uint;
		using Value_t	= T;

	private:
		static constexpr uint	Level2_Count	= (Count <= 32 ? 32 : 64);
		static constexpr uint	Level1_Count	= ((Count + Level2_Count - 1) / Level2_Count);
		static constexpr uint	WaitCount		= 10;
		
		using Bitfield_t	= Conditional< (Level2_Count <= 32), uint, ulong >;
		using AssignBits_t	= StaticArray< Atomic< Bitfield_t >, Level1_Count >;
		
		STATIC_ASSERT( AssignBits_t::value_type::is_always_lock_free );


	// variables
	private:
		AssignBits_t	_assignedBits;		// 1 - is unassigned bit, 0 - assigned bit
		AssignBits_t	_availableBits;		// 1 - unavailable bit, 0 - available

		union {
			T			_values  [ Count ];
			ubyte		_arrData [ Count * sizeof(T) ];
		};


	// methods
	public:
		LfStaticPool ()
		{
			for (uint i = 0; i < Level1_Count; ++i) {
				_assignedBits[i].store( UMax, EMemoryOrder::Relaxed );
				_availableBits[i].store( UMax, EMemoryOrder::Relaxed );
			}
			DEBUG_ONLY( std::memset( _values, 0xCD, sizeof(_values) ));
		}

		~LfStaticPool ()
		{
			Clear();
		}

		LfStaticPool (const Self &) = delete;
		LfStaticPool (Self &&) = delete;

		Self&  operator = (const Self &) = delete;
		Self&  operator = (Self &&) = delete;


		ND_ static constexpr usize  capacity ()		{ return Count; }

		
		void Clear ()
		{
			return Clear( [](T &value) { value.~T(); });
		}

		template <typename FN>
		void Clear (FN &&fn)
		{
			// invalidate cache before calling value destructor
			ThreadFence( EMemoryOrder::Acquire );

			for (uint i = 0; i < Level1_Count; ++i)
			{
				const Bitfield_t	available	= _availableBits[i].exchange( UMax, EMemoryOrder::Relaxed );
				Bitfield_t			bits		= ~available;

				for (int j = BitScanForward( bits ); j >= 0;)
				{
					fn( _values[ Index_t(j) | (Index_t(i) * Level2_Count) ] );

					bits ^= (Bitfield_t(1) << j);
					j	  = BitScanForward( bits );
				}

				const Bitfield_t	assigned = _assignedBits[i].exchange( UMax, EMemoryOrder::Relaxed );
				ASSERT( available == assigned );
			}

			// fuzz value data and flush cache
			DEBUG_ONLY(
				std::memset( _values, 0xCD, sizeof(_values) );
				ThreadFence( EMemoryOrder::Release );
			)
		}


		template <typename V>
		ND_ bool  Put (V&& value)
		{
			for (uint k = 0; k < WaitCount; ++k)
			{
				for (uint i = 0; i < Level1_Count; ++i)
				{
					Bitfield_t	bits = _assignedBits[i].load( EMemoryOrder::Relaxed );

					for (int j = BitScanForward( bits ); j >= 0;)
					{
						const Bitfield_t	mask = Bitfield_t(1) << j;

						// acquire index and invalidate cache before writing
						if ( _assignedBits[i].compare_exchange_weak( INOUT bits, bits ^ mask, EMemoryOrder::Acquire, EMemoryOrder::Relaxed ))	// 1 -> 0
						{
							uint	idx = Index_t(j) | (Index_t(i) * Level2_Count);

							ASSERT( idx < Count );
							PlacementNew<T>( &_values[idx], FwdArg<V>(value) );
							
							// flush cache after writing and set availability bit
							Bitfield_t	old_available = _availableBits[i].fetch_xor( mask, EMemoryOrder::Release );	// 1 -> 0
							Unused( old_available );
							ASSERT( !!(old_available & mask) );

							return true;
						}

						j = BitScanForward( bits );
					}
				}
				std::this_thread::yield();
			}
			return false;
		}


		bool  Extract (OUT Value_t &outValue)
		{
			for (uint k = 0; k < WaitCount; ++k)
			{
				for (uint i = 0; i < Level1_Count; ++i)
				{
					Bitfield_t	bits = _availableBits[i].load( EMemoryOrder::Relaxed );

					for (int j = BitScanForward( ~bits ); j >= 0;)
					{
						const Bitfield_t	mask = Bitfield_t(1) << j;
						
						// acquire index and invalidate cache before reading / writing
						if ( _availableBits[i].compare_exchange_weak( INOUT bits, bits | mask, EMemoryOrder::Acquire, EMemoryOrder::Relaxed ))	// 0 -> 1
						{
							Index_t	idx = Index_t(j) | (Index_t(i) * Level2_Count);

							ASSERT( idx < Count );
							outValue = RVRef( _values[idx] );
							
							_values[idx].~T();
							DEBUG_ONLY( std::memset( &_values[idx], 0xCD, sizeof(T) ));
						
							// flush cache after writing
							Bitfield_t	old_assign	= _assignedBits[i].fetch_or( mask, EMemoryOrder::Release );	// 0 -> 1
							Unused( old_assign );
							ASSERT( not (old_assign & mask) );

							return true;
						}

						j = BitScanForward( ~bits );
					}
				}
				std::this_thread::yield();
			}
			return false;
		}
	};


}	// AE::Threading
