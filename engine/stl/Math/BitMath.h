#pragma once

#include "stl/Algorithms/EnumUtils.h"
#include "stl/Algorithms/Cast.h"

#ifdef COMPILER_MSVC
# include <intrin.h>
# pragma intrinsic( _BitScanForward, _BitScanReverse )
# if PLATFORM_BITS == 64
#	pragma intrinsic( _BitScanForward64, _BitScanReverse64 )
# endif
#endif
	
namespace AE::Math
{

/*
=================================================
	IsPowerOfTwo
=================================================
*/
	template <typename T>
	ND_ forceinline constexpr bool  IsPowerOfTwo (const T &x)
	{
		return (x != 0) & ((x & (x - T(1))) == T(0));
	}
	
/*
=================================================
	IntLog2 / GetPowerOfTwo / BitScanReverse
=================================================
*/
	template <typename T>
	ND_ forceinline int  IntLog2 (const T& x)
	{
		constexpr int	INVALID_INDEX = std::numeric_limits<int>::min();

	#ifdef COMPILER_MSVC
		unsigned long	index;

		if constexpr ( sizeof(x) > sizeof(uint) )
			return _BitScanReverse64( OUT &index, x ) ? index : INVALID_INDEX;
		else
			return _BitScanReverse( OUT &index, x ) ? index : INVALID_INDEX;
		
	#elif defined(COMPILER_GCC) or defined(COMPILER_CLANG)
		if constexpr ( sizeof(x) > sizeof(uint) )
			return x ? (sizeof(x)*8)-1 - __builtin_clzll( x ) : INVALID_INDEX;
		else
			return x ? (sizeof(x)*8)-1 - __builtin_clz( x ) : INVALID_INDEX;

	#else
		//return std::ilogb( x );
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
	#ifdef COMPILER_MSVC
		constexpr int	INVALID_INDEX = std::numeric_limits<int>::min();
		unsigned long	index;

		if constexpr ( sizeof(x) > sizeof(uint) )
			return _BitScanForward64( OUT &index, x ) ? index : INVALID_INDEX;
		else
			return _BitScanForward( OUT &index, x ) ? index : INVALID_INDEX;
		
	#elif defined(COMPILER_GCC) or defined(COMPILER_CLANG)
		if constexpr ( sizeof(x) > sizeof(uint) )
			return __builtin_ffsll( x ) - 1;
		else
			return __builtin_ffs( x ) - 1;

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
	ND_ forceinline size_t  BitCount (const T& x)
	{
		STATIC_ASSERT( IsEnum<T> or IsUnsignedInteger<T> );

		if constexpr( sizeof(x) == 8 )
			return std::bitset<64>{ uint64_t(x) }.count();
		else
		if constexpr( sizeof(x) <= 4 )
			return std::bitset<32>{ uint32_t(x) }.count();
	}
	
/*
=================================================
	SafeLeftBitShift
=================================================
*/
	template <typename T>
	ND_ forceinline constexpr T  SafeLeftBitShift (const T& x, size_t shift)
	{
		STATIC_ASSERT( IsScalarOrEnum<T> );
		STATIC_ASSERT( IsInteger<T> );
		
		return x << (shift & (sizeof(shift)*8 - 1));
	}
	
/*
=================================================
	SafeRightBitShift
=================================================
*/
	template <typename T>
	ND_ forceinline constexpr T  SafeRightBitShift (const T& x, size_t shift)
	{
		STATIC_ASSERT( IsScalarOrEnum<T> );
		STATIC_ASSERT( IsInteger<T> );

		return x >> (shift & (sizeof(shift)*8 - 1));
	}
	
/*
=================================================
	BitRotateLeft
----
	from https://en.wikipedia.org/wiki/Circular_shift#Implementing_circular_shifts
=================================================
*/
	namespace _ae_math_hidden_
	{
		template <typename T>
		forceinline constexpr T _BitRotateLeft (T value, size_t shift)
		{
			const size_t	mask = (sizeof(value)*8 - 1);

			shift &= mask;
			return (value << shift) | (value >> ( ~(shift-1) & mask ));
		}
	}	// _ae_math_hidden_
	
	template <typename T>
	ND_ forceinline constexpr T  BitRotateLeft (const T& x, size_t shift)
	{
		STATIC_ASSERT( IsScalarOrEnum<T> );
		STATIC_ASSERT( IsInteger<T> );

		using Unsigned_t = ToUnsignedInteger<T>;
		return BitCast<T>( _ae_math_hidden_::_BitRotateLeft( BitCast<Unsigned_t>(x), shift ));
	}
	
/*
=================================================
	BitRotateRight
----
	from https://en.wikipedia.org/wiki/Circular_shift#Implementing_circular_shifts
=================================================
*/
	namespace _ae_math_hidden_
	{
		template <typename T>
		forceinline constexpr T _BitRotateRight (T value, size_t shift)
		{
			const size_t	mask = (sizeof(value)*8 - 1);

			shift &= mask;
			return (value >> shift) | (value << ( ~(shift-1) & mask ));
		}
	}	// _ae_math_hidden_

	template <typename T>
	ND_ forceinline constexpr T  BitRotateRight (const T& x, size_t shift)
	{
		STATIC_ASSERT( IsScalarOrEnum<T> );
		STATIC_ASSERT( IsInteger<T> );
		
		using Unsigned_t = ToUnsignedInteger<T>;
		return BitCast<T>( _ae_math_hidden_::_BitRotateRight( BitCast<Unsigned_t>(x), shift ));
	}


}	// AE::Math
