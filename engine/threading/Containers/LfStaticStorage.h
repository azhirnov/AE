// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "threading/Common.h"
#include "stl/Math/BitMath.h"

namespace AE::Threading
{

	//
	// Lock-free Static Storage
	//

	template <typename T, uint Count>
	struct LfStaticStorage
	{
		STATIC_ASSERT( Count > 0 );
		STATIC_ASSERT( std::is_trivially_constructible_v<T> );
		STATIC_ASSERT( std::is_trivially_destructible_v<T> );

	// types
	public:
		using Self		= LfStaticStorage< T, Count >;
		using Index_t	= uint;
		using Value_t	= T;

	private:
		static constexpr uint	Level2_Count	= (Count <= 32 ? 32 : 64);
		static constexpr uint	Level1_Count	= ((Count + Level2_Count - 1) / Level2_Count);
		static constexpr uint	WaitCount		= 10;
		
		using Bitfield_t	= Conditional< (Level2_Count <= 32), uint, uint64_t >;
		using AssignBits_t	= StaticArray< Atomic< Bitfield_t >, Level1_Count >;
		
		STATIC_ASSERT( AssignBits_t::value_type::is_always_lock_free );


	// variables
	private:
		AssignBits_t	_assignedBits;	// 1 - is unassigned bit, 0 - assigned bit
		AssignBits_t	_initBits;		// 0 - uninitialized bit, 0 - initialized and can be readn

		union {
			T			_values  [ Count ];
			uint8_t		_arrData [ Count * sizeof(T) ];
		};


	// methods
	public:
		LfStaticStorage ()
		{
			Clear();
		}

		~LfStaticStorage ()
		{
			for (uint i = 0; i < Level1_Count; ++i) {
				ASSERT( _assignedBits[i].load( EMemoryOrder::Relaxed ) == UMax );
				ASSERT( _initBits[i].load( EMemoryOrder::Relaxed ) == UMax );
			}
		}

		LfStaticStorage (const Self &) = delete;
		LfStaticStorage (Self &&) = delete;

		Self&  operator = (const Self &) = delete;
		Self&  operator = (Self &&) = delete;


		void Clear ()
		{
			for (uint i = 0; i < Level1_Count; ++i) {
				_assignedBits[i].store( UMax, EMemoryOrder::Relaxed );
				_initBits[i].store( UMax, EMemoryOrder::Relaxed );
			}
			DEBUG_ONLY( std::memset( _values, 0xCD, sizeof(_values) ));
		}


		ND_ bool  Append (const Value_t &value, OUT Index_t &outIndex)
		{
			for (uint k = 0; k < WaitCount; ++k)
			{
				for (uint i = 0; i < Level1_Count; ++i)
				{
					Bitfield_t	bits = _assignedBits[i].load( EMemoryOrder::Relaxed );

					for (int j = BitScanForward( bits ); j >= 0;)
					{
						const Bitfield_t	mask = Bitfield_t(1) << j;

						if ( _assignedBits[i].compare_exchange_weak( INOUT bits, bits ^ mask, EMemoryOrder::Acquire, EMemoryOrder::Relaxed ))
						{
							outIndex = Index_t(j) | (Index_t(i) * Level2_Count);

							ASSERT( outIndex < Count );
							PlacementNew<T>( &_values[outIndex], value );

							Bitfield_t	old_init = _initBits[i].fetch_xor( mask, EMemoryOrder::Release );
							ASSERT( !!(old_init & mask) );

							return true;
						}

						j = BitScanForward( bits );
					}
				}
				std::this_thread::yield();
			}

			outIndex = UMax;
			return false;
		}
		

		ND_ bool  Append (const Value_t &value)
		{
			Index_t	idx;
			return Append( value, OUT idx );
		}


		bool  Remove (Index_t index)
		{
			if ( index < Count )
			{
				const uint	chunk_idx	= index / Level2_Count;
				const uint	bit_idx		= index % Level2_Count;
				Bitfield_t	mask		= Bitfield_t(1) << bit_idx;
				Bitfield_t	old_init	= _initBits[chunk_idx].fetch_or( mask, EMemoryOrder::Relaxed );		// 0 -> 1
				Bitfield_t	old_assign	= _assignedBits[chunk_idx].fetch_or( mask, EMemoryOrder::Relaxed );	// 0 -> 1
				ASSERT( !(old_init & mask) );
				ASSERT( !(old_assign & mask) );
				return true;
			}
			return false;
		}
		

		bool Extract (OUT Value_t &outValue)
		{
			for (uint k = 0; k < WaitCount; ++k)
			{
				for (uint i = 0; i < Level1_Count; ++i)
				{
					Bitfield_t	bits = _initBits[i].load( EMemoryOrder::Relaxed );

					for (int j = BitScanForward( ~bits ); j >= 0;)
					{
						const Bitfield_t	mask = Bitfield_t(1) << j;

						if ( _initBits[i].compare_exchange_weak( INOUT bits, bits | mask, EMemoryOrder::Acquire, EMemoryOrder::Relaxed ))
						{
							Index_t	idx = Index_t(j) | (Index_t(i) * Level2_Count);
						
							ASSERT( idx < Count );
							outValue = std::move( _values[idx] );
							
							_values[idx].~T();
							DEBUG_ONLY( std::memset( &_values[idx], 0xCD, sizeof(T) ));
						
							Bitfield_t	old_assign	= _assignedBits[i].fetch_or( mask, EMemoryOrder::Relaxed );	// 0 -> 1
							ASSERT( !(old_assign & mask) );

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
