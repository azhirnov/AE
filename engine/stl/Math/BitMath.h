#pragma once

#include "stl/Algorithms/Cast.h"
#include <cstdlib>

#ifdef COMPILER_MSVC
# include <intrin.h>
# pragma intrinsic( _BitScanForward, _BitScanReverse )
# pragma intrinsic( __popcnt16, __popcnt )
# if PLATFORM_BITS == 64
#	pragma intrinsic( _BitScanForward64, _BitScanReverse64 )
#	pragma intrinsic( __popcnt64 )
# endif
#endif
	
namespace AE::Math
{
	
	template <typename T>
	using NearInt = Conditional< (sizeof(T) > sizeof(int32_t)), int64_t,
						Conditional< (sizeof(T) > sizeof(int16_t)), int32_t,
							Conditional< (sizeof(T) > sizeof(int8_t)), int16_t, int8_t >>>;

	template <typename T>
	using NearUInt = Conditional< (sizeof(T) > sizeof(uint32_t)), uint64_t,
						Conditional< (sizeof(T) > sizeof(uint16_t)), uint32_t,
							Conditional< (sizeof(T) > sizeof(uint8_t)), uint16_t, uint8_t >>>;
	
/*
=================================================
	ToNearUInt
=================================================
*/
	template <typename T>
	ND_ forceinline constexpr NearUInt<T>  ToNearUInt (const T &value)
	{
		STATIC_ASSERT( IsScalarOrEnum<T> );
		STATIC_ASSERT( sizeof(value) <= sizeof(NearUInt<T>) );

		return NearUInt<T>( value );
	}

/*
=================================================
	ToNearInt
=================================================
*/
	template <typename T>
	ND_ forceinline constexpr NearInt<T>  ToNearInt (const T &value)
	{
		STATIC_ASSERT( IsScalarOrEnum<T> );
		STATIC_ASSERT( sizeof(value) <= sizeof(NearInt<T>) );

		return NearInt<T>( value );
	}

/*
=================================================
	AllBits
----
	returns 'true' if 'lhs' has ALL bits that presented in 'rhs'
=================================================
*/
	template <typename T1, typename T2>
	ND_ forceinline constexpr bool  AllBits (const T1& lhs, const T2& rhs)
	{
		STATIC_ASSERT( IsScalarOrEnum< T1 > );
		STATIC_ASSERT( IsScalarOrEnum< T2 > );

		return ( ToNearUInt(lhs) & ToNearUInt(rhs) ) == ToNearUInt(rhs);
	}
	
/*
=================================================
	AllBits
=================================================
*/
	template <typename T1, typename T2, typename T3>
	ND_ forceinline constexpr bool  AllBits (const T1& lhs, const T2& rhs, const T3& mask)
	{
		STATIC_ASSERT( IsScalarOrEnum< T1 > );
		STATIC_ASSERT( IsScalarOrEnum< T2 > );
		STATIC_ASSERT( IsScalarOrEnum< T3 > );
		ASSERT( rhs != T2(0) );

		return ( ToNearUInt(lhs) & ToNearUInt(mask) ) == ( ToNearUInt(rhs) & ToNearUInt(mask) );
	}

/*
=================================================
	AnyBits
----
	returns 'true' if 'lhs' has ANY bit that presented in 'rhs'
=================================================
*/
	template <typename T1, typename T2>
	ND_ forceinline constexpr bool  AnyBits (const T1& lhs, const T2& rhs)
	{
		STATIC_ASSERT( IsScalarOrEnum< T1 > );
		STATIC_ASSERT( IsScalarOrEnum< T2 > );
		ASSERT( rhs != T2(0) );

		return !!( ToNearUInt(lhs) & ToNearUInt(rhs) );
	}
	
/*
=================================================
	ExtractBit
=================================================
*/
	template <typename T>
	ND_ forceinline constexpr T  ExtractBit (INOUT T& value)
	{
		STATIC_ASSERT( IsEnum<T> or IsInteger<T> );
		
		using U = NearUInt<T>;
		ASSERT( U(value) > 0 );

		const U	result = U(value) & ~(U(value) - U(1));
		value = T(U(value) & ~result);

		return T(result);
	}

/*
=================================================
	IsPowerOfTwo
=================================================
*/
	template <typename T>
	ND_ forceinline constexpr bool  IsPowerOfTwo (const T &x)
	{
		STATIC_ASSERT( IsEnum<T> or IsInteger<T> );

		using U = NearUInt<T>;

		U	val = U( x );
		return (val != U(0)) & ((val & (val - U(1))) == U(0));
	}
	
/*
=================================================
	IntLog2 / GetPowerOfTwo / BitScanReverse
=================================================
*/
	template <typename T>
	ND_ forceinline int  IntLog2 (const T& x)
	{
		STATIC_ASSERT( IsInteger<T> or IsEnum<T> );

		constexpr int	INVALID_INDEX = std::numeric_limits<int>::min();

	#ifdef COMPILER_MSVC
		unsigned long	index;
		
		if constexpr( sizeof(x) == 8 )
			return _BitScanReverse64( OUT &index, ulong(x) ) ? index : INVALID_INDEX;
		else
		if constexpr( sizeof(x) <= 4 )
			return _BitScanReverse( OUT &index, uint(x) ) ? index : INVALID_INDEX;
		
	#elif defined(COMPILER_GCC) or defined(COMPILER_CLANG)
		if constexpr( sizeof(x) == 8 )
			return x ? (sizeof(x)*8)-1 - __builtin_clzll( ulong(x) ) : INVALID_INDEX;
		else
		if constexpr( sizeof(x) <= 4 )
			return x ? (sizeof(x)*8)-1 - __builtin_clz( uint(x) ) : INVALID_INDEX;

	#else
		#error add BitScanReverse implementation
	#endif
	}

	template <typename T>
	ND_ forceinline int  BitScanReverse (const T& x)
	{
		return IntLog2( x );
	}
	
/*
=================================================
	BitScanForward
=================================================
*/
	template <typename T>
	ND_ forceinline int  BitScanForward (const T& x)
	{
		STATIC_ASSERT( IsInteger<T> or IsEnum<T> );

	#ifdef COMPILER_MSVC
		constexpr int	INVALID_INDEX = std::numeric_limits<int>::min();
		unsigned long	index;
		
		if constexpr( sizeof(x) == 8 )
			return _BitScanForward64( OUT &index, ulong(x) ) ? index : INVALID_INDEX;
		else
		if constexpr( sizeof(x) <= 4 )
			return _BitScanForward( OUT &index, uint(x) ) ? index : INVALID_INDEX;
		
	#elif defined(COMPILER_GCC) or defined(COMPILER_CLANG)
		if constexpr( sizeof(x) == 8 )
			return __builtin_ffsll( ulong(x) ) - 1;
		else
		if constexpr( sizeof(x) <= 4 )
			return __builtin_ffs( uint(x) ) - 1;

	#else
		#error add BitScanForward implementation
	#endif
	}
	
/*
=================================================
	BitCount
=================================================
*/
	template <typename T>
	ND_ forceinline usize  BitCount (const T& x)
	{
		STATIC_ASSERT( IsEnum<T> or IsInteger<T> );
		
	#ifdef COMPILER_MSVC
		// requires CPUInfo::POPCNT
	  #if PLATFORM_BITS == 64
		if constexpr( sizeof(x) == 8 )
			return T( __popcnt64( ulong(x) ));
		else
	  #endif
		if constexpr( sizeof(x) == 4 )
			return T( __popcnt( uint(x) ));
		else
		if constexpr( sizeof(x) <= 2 )
			return T( __popcnt16( ushort(x) ));

	#else
		if constexpr( sizeof(x) == 8 )
			return T( std::bitset<64>{ ulong(x) }.count() );
		else
		if constexpr( sizeof(x) <= 4 )
			return T( std::bitset<32>{ uint(x) }.count() );
	#endif
	}
	
/*
=================================================
	SafeLeftBitShift
=================================================
*/
	template <typename T>
	ND_ forceinline constexpr T  SafeLeftBitShift (const T& x, usize shift)
	{
		STATIC_ASSERT( IsEnum<T> or IsInteger<T> );
		
		return T( ToNearUInt(x) << (shift & (sizeof(x)*8 - 1)) );
	}
	
/*
=================================================
	SafeRightBitShift
=================================================
*/
	template <typename T>
	ND_ forceinline constexpr T  SafeRightBitShift (const T& x, usize shift)
	{
		STATIC_ASSERT( IsEnum<T> or IsInteger<T> );

		return T( ToNearUInt(x) >> (shift & (sizeof(x)*8 - 1)) );
	}
	
/*
=================================================
	BitRotateLeft
----
	from https://en.wikipedia.org/wiki/Circular_shift#Implementing_circular_shifts
=================================================
*/
	namespace _hidden_
	{
		template <typename T>
		forceinline constexpr T _BitRotateLeft (T value, usize shift)
		{
			const usize	mask = (sizeof(value)*8 - 1);

			shift &= mask;
			return (value << shift) | (value >> ( ~(shift-1) & mask ));
		}
	}	// _hidden_
	
	template <typename T>
	ND_ forceinline constexpr T  BitRotateLeft (const T& x, usize shift)
	{
		STATIC_ASSERT( IsEnum<T> or IsInteger<T> );
		
	#ifdef COMPILER_MSVC
		if constexpr( sizeof(x) > sizeof(uint) )
			return T( _rotl64( ulong(x), int(shift) ));
		else
			return T( _rotl( uint(x), int(shift) ));
	#else
		return T( Math::_hidden_::_BitRotateLeft( ToNearUInt(x), shift ));
	#endif
	}
	
/*
=================================================
	BitRotateRight
----
	from https://en.wikipedia.org/wiki/Circular_shift#Implementing_circular_shifts
=================================================
*/
	namespace _hidden_
	{
		template <typename T>
		forceinline constexpr T _BitRotateRight (T value, usize shift)
		{
			const usize	mask = (sizeof(value)*8 - 1);

			shift &= mask;
			return (value >> shift) | (value << ( ~(shift-1) & mask ));
		}
	}	// _hidden_

	template <typename T>
	ND_ forceinline constexpr T  BitRotateRight (const T& x, usize shift)
	{
		STATIC_ASSERT( IsEnum<T> or IsInteger<T> );
		
	#ifdef COMPILER_MSVC
		if constexpr( sizeof(x) > sizeof(uint) )
			return T( _rotr64( ulong(x), int(shift) ));
		else
			return T( _rotr( uint(x), int(shift) ));
	#else
		return T( Math::_hidden_::_BitRotateRight( ToNearUInt(x), shift ));
	#endif
	}
	
/*
=================================================
	ToBitMask
=================================================
*/
	template <typename T>
	ND_ inline constexpr EnableIf< IsUnsignedInteger<T>, T >  ToBitMask (usize count)
	{
		return	count >= sizeof(T)*8 ?
					~T(0) :
					(T(1) << count) - 1;
	}
	
	template <typename T>
	ND_ inline constexpr EnableIf< IsUnsignedInteger<T>, T >  ToBitMask (usize firstBit, usize count)
	{
		return SafeLeftBitShift( ToBitMask<T>( count ), firstBit );
	}
	
/*
=================================================
	ByteSwap
----
	Little Endian <-> Big Endian
=================================================
*/
	template <typename T>
	ND_ forceinline T  ByteSwap (const T &x)
	{
		STATIC_ASSERT( IsEnum<T> or IsInteger<T> );

		if constexpr( sizeof(x) == 1 )
			return x;
		else

	#ifdef COMPILER_MSVC
		if constexpr( sizeof(x) == 2 )
			return T( _byteswap_ushort( ToNearUInt(x) ));
		else
		if constexpr( sizeof(x) == 4 )
			return T( _byteswap_ulong( ToNearUInt(x) ));
		else
		if constexpr( sizeof(x) == 8 )
			return T( _byteswap_uint64( ToNearUInt(x) ));

	#elif defined(COMPILER_GCC) or defined(COMPILER_CLANG)
		if constexpr( sizeof(x) == 2 )
			return T(((ushort(x) & 0x00FF) << 8) | ((ushort(x) & 0xFF00) >> 8));
		else
		if constexpr( sizeof(x) == 4 )
			return T( __builtin_bswap32( ToNearUInt(x) ));
		else
		if constexpr( sizeof(x) == 8 )
			return T( __builtin_bswap64( ToNearUInt(x) ));
	#else
		// will fail to compile
		return;
	#endif
	}


}	// AE::Math
