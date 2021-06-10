// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Math/Math.h"
#include "stl/Math/Vec.h"
#include "stl/Math/Bytes.h"
#include "stl/Math/Color.h"
#include "stl/Math/BitMath.h"
#include "stl/Algorithms/ArrayUtils.h"
#include "stl/Memory/MemUtils.h"
#include "stl/Containers/NtStringView.h"
#include <sstream>
#include <charconv>

namespace AE::STL
{
	using namespace std::string_literals;

/*
=================================================
	operator << (String, String)
	operator << (String, StringView)
	operator << (String, CStyleString)
	operator << (String, char)
=================================================
*/
	template <typename T, typename A1, typename A2>
	forceinline std::basic_string<T,A1>&&  operator << (std::basic_string<T,A1> &&lhs, const std::basic_string<T,A2> &rhs)
	{
		return RVRef( RVRef(lhs).append( rhs.data(), rhs.size() ));
	}

	template <typename T, typename A1, typename A2>
	forceinline std::basic_string<T,A1>&  operator << (std::basic_string<T,A1> &lhs, const std::basic_string<T,A2> &rhs)
	{
		return lhs.append( rhs.data(), rhs.size() );
	}

	template <typename T, typename A>
	forceinline std::basic_string<T,A>&&  operator << (std::basic_string<T,A> &&lhs, const BasicStringView<T> &rhs)
	{
		return RVRef( RVRef(lhs).append( rhs.data(), rhs.size() ));
	}

	template <typename T, typename A>
	forceinline std::basic_string<T,A>&  operator << (std::basic_string<T,A> &lhs, const BasicStringView<T> &rhs)
	{
		return lhs.append( rhs.data(), rhs.size() );
	}

	template <typename T, typename A>
	forceinline std::basic_string<T,A>&&  operator << (std::basic_string<T,A> &&lhs, const NtBasicStringView<T> &rhs)
	{
		return RVRef( RVRef(lhs).append( rhs.c_str(), rhs.size() ));
	}

	template <typename T, typename A>
	forceinline std::basic_string<T,A>&  operator << (std::basic_string<T,A> &lhs, const NtBasicStringView<T> &rhs)
	{
		return lhs.append( rhs.c_str(), rhs.size() );
	}

	template <typename T, typename A>
	forceinline std::basic_string<T,A>&&  operator << (std::basic_string<T,A> &&lhs, T const * const rhs)
	{
		return RVRef( RVRef(lhs).append( rhs ));
	}

	template <typename T, typename A>
	forceinline std::basic_string<T,A>&  operator << (std::basic_string<T,A> &lhs, T const * const rhs)
	{
		return lhs.append( rhs );
	}

	template <typename T, typename A>
	forceinline std::basic_string<T,A>&&  operator << (std::basic_string<T,A> &&lhs, const T rhs)
	{
		return RVRef( RVRef(lhs) += rhs );
	}

	template <typename T, typename A>
	forceinline std::basic_string<T,A>&  operator << (std::basic_string<T,A> &lhs, const T rhs)
	{
		return (lhs += rhs);
	}
//-----------------------------------------------------------------------------



/*
=================================================
	IsUpperCase
=================================================
*/
	ND_ forceinline const char  IsUpperCase (const char c)
	{
		return (c >= 'A' and c <= 'Z');
	}

/*
=================================================
	IsLowerCase
=================================================
*/
	ND_ forceinline const char  IsLowerCase (const char c)
	{
		return (c >= 'a' and c <= 'z');
	}

/*
=================================================
	ToLowerCase
=================================================
*/
	ND_ forceinline const char  ToLowerCase (const char c)
	{
		return IsUpperCase( c ) ? (c - 'A' + 'a') : c;
	}

/*
=================================================
	ToUpperCase
=================================================
*/
	ND_ forceinline const char  ToUpperCase (const char c)
	{
		return IsLowerCase( c ) ? (c - 'a' + 'A') : c;
	}

/*
=================================================
	HasSubString
----
	returns 'true' if 'str' has substring 'substr',
	comparison is case sansitive.
=================================================
*/
	ND_ inline bool  HasSubString (StringView str, StringView substr)
	{
		return (str.find( substr ) != StringView::npos);
	}

/*
=================================================
	HasSubStringIC
----
	returns 'true' if 'str' has substring 'substr',
	comparison is case insansitive.
=================================================
*/
	ND_ inline bool  HasSubStringIC (StringView str, StringView substr)
	{
		if ( str.empty() or substr.empty() )
			return false;

		for (usize i = 0, j = 0; i < str.length(); ++i)
		{
			while ( i+j < str.length() and j < substr.length() and
					ToLowerCase( substr[j] ) == ToLowerCase( str[i+j] ))
			{
				++j;
				if ( j >= substr.length() )
					return true;
			}
			j = 0;
		}
		return false;
	}
	
/*
=================================================
	StartsWith
----
	returns 'true' if 'str' starts with substring 'substr',
	comparison is case sansitive.
=================================================
*/
	ND_ inline bool  StartsWith (StringView str, StringView substr)
	{
		if ( str.length() < substr.length() )
			return false;

		for (usize i = 0; i < substr.length(); ++i)
		{
			if ( str[i] != substr[i] )
				return false;
		}
		return true;
	}
	
/*
=================================================
	StartsWithIC
----
	returns 'true' if 'str' starts with substring 'substr',
	comparison is case insansitive.
=================================================
*/
	ND_ inline bool  StartsWithIC (StringView str, StringView substr)
	{
		if ( str.length() < substr.length() )
			return false;

		for (usize i = 0; i < substr.length(); ++i)
		{
			if ( ToLowerCase(str[i]) != ToLowerCase(substr[i]) )
				return false;
		}
		return true;
	}
	
/*
=================================================
	EndsWith
----
	returns 'true' if 'str' ends with substring 'substr',
	comparison is case sansitive.
=================================================
*/
	ND_ inline bool  EndsWith (StringView str, StringView substr)
	{
		if ( str.length() < substr.length() )
			return false;

		for (usize i = 1; i <= substr.length(); ++i)
		{
			if ( str[str.length() - i] != substr[substr.length() - i] )
				return false;
		}
		return true;
	}
	
/*
=================================================
	EndsWithIC
----
	returns 'true' if 'str' ends with substring 'substr',
	comparison is case insansitive.
=================================================
*/
	ND_ inline bool  EndsWithIC (StringView str, StringView substr)
	{
		if ( str.length() < substr.length() )
			return false;

		for (usize i = 1; i <= substr.length(); ++i)
		{
			if ( ToLowerCase(str[str.length() - i]) != ToLowerCase(substr[substr.length() - i]) )
				return false;
		}
		return true;
	}
	
/*
=================================================
	FindAndReplace
=================================================
*/
	inline uint  FindAndReplace (INOUT String& str, StringView oldStr, StringView newStr)
	{
		String::size_type	pos		= 0;
		uint				count	= 0;

		while ( (pos = StringView{str}.find( oldStr, pos )) != StringView::npos )
		{
			str.replace( pos, oldStr.length(), newStr.data() );
			pos += newStr.length();
			++count;
		}
		return count;
	}

/*
=================================================
	ToAnsiString
=================================================
*/
	template <typename R, typename T>
	inline BasicString<R>  ToAnsiString (BasicStringView<T> str)
	{
		BasicString<R>	result;
		result.resize( str.size() );

		for (usize i = 0; i < str.size(); ++i)
		{
			ASSERT( str[i] <= 0x7F );
			result[i] = R(str[i] & 0x7F);
		}
		return result;
	}

	template <typename R, typename T>
	inline BasicString<R>  ToAnsiString (const T* str)
	{
		return ToAnsiString<R>( BasicStringView<T>{ str });
	}
	
	template <typename R, typename T, typename A>
	inline BasicString<R>  ToAnsiString (const std::basic_string<T,A> &str)
	{
		return ToAnsiString<R>( BasicStringView<T>{ str });
	}
//-----------------------------------------------------------------------------



/*
=================================================
	ToString
=================================================
*/
	template <typename T>
	ND_ forceinline EnableIf<not IsEnum<T>, String>  ToString (const T &value)
	{
		return std::to_string( value );
	}

	template <typename E>
	ND_ forceinline EnableIf<IsEnum<E>, String>  ToString (const E &value)
	{
		using T = Conditional< (sizeof(E) > sizeof(uint)), uint, ulong >;

		return std::to_string( T(value) );
	}

	ND_ forceinline String  ToString (const bool &value)
	{
		return value ? "true" : "false";
	}
	
/*
=================================================
	ToString
=================================================
*/
	template <int Radix, typename T>
	ND_ forceinline EnableIf< IsEnum<T> or IsInteger<T>, String>  ToString (const T &value)
	{
		if constexpr ( Radix == 10 )
		{
			return std::to_string( value );
		}
		else
		if constexpr ( Radix == 16 and sizeof(T) > sizeof(uint) )
		{
			std::stringstream	str;
			str << std::hex << ulong(value);
			return str.str();
		}
		else
		if constexpr ( Radix == 16 )
		{
			std::stringstream	str;
			str << std::hex << uint(BitCast<NearUInt<T>>(value));
			return str.str();
		}else{
			//STATIC_ASSERT( false, "not supported, yet" );
		}
	}

/*
=================================================
	ToString (double)
=================================================
*/
	ND_ inline String  ToString (const double &value, uint fractParts)
	{
		ASSERT( (fractParts > 0) and (fractParts < 100) );
		fractParts = Clamp( fractParts, 1u, 99u );

		const char	fmt[8]  = {'%', '0', '.', char('0' + fractParts / 10), char('0' + fractParts % 10), 'f', '\0' };
		char		buf[32] = {};

		const int	len = std::snprintf( buf, CountOf(buf), fmt, value );
		ASSERT( len > 0 );
		return buf;
	}

/*
=================================================
	ToString (Vec)
=================================================
*/
	template <typename T, int I>
	ND_ inline String  ToString (const Vec<T,I> &value)
	{
		String	str = "( ";

		for (int i = 0; i < I; ++i)
		{
			if ( i > 0 )	str << ", ";
			str << ToString( value[i] );
		}
		str << " )";
		return str;
	}
	
/*
=================================================
	ToString (RGBAColor)
=================================================
*/
	template <typename T>
	ND_ inline String  ToString (const RGBAColor<T> &value)
	{
		String	str = "( "s;

		for (uint i = 0; i < 4; ++i)
		{
			if ( i > 0 )	str << ", ";
			str << ToString( value[i] );
		}
		str << " )";
		return str;
	}

/*
=================================================
	ToString (Bytes)
=================================================
*/
	template <typename T>
	ND_ inline String  ToString (const TBytes<T> &value)
	{
		const T	kb	= SafeLeftBitShift( T(1), 12 );
		const T mb	= SafeLeftBitShift( T(1), 22 );
		const T	gb	= SafeLeftBitShift( T(1), 32 );
		const T	tb	= SafeLeftBitShift( T(1), 42 );
		const T	val	= T(value);

		String	str;

		if ( val < kb )	str << ToString( val ) << " b";								else
		if ( val < mb )	str << ToString( SafeRightBitShift( val, 10 )) << " Kb";	else
		if ( val < gb )	str << ToString( SafeRightBitShift( val, 20 )) << " Mb";	else
		if ( val < tb )	str << ToString( SafeRightBitShift( val, 30 )) << " Gb";	else
						str << ToString( SafeRightBitShift( val, 40 )) << " Tb";
		
		return str;
	}

/*
=================================================
	ToString (chrono::duration)
=================================================
*/
	template <typename T, typename Duration>
	ND_ inline String  ToString (const std::chrono::duration<T,Duration> &value, uint precission = 2)
	{
		using SecondsD_t  = std::chrono::duration<double>;
		using MicroSecD_t = std::chrono::duration<double, std::micro>;
		using NanoSecD_t  = std::chrono::duration<double, std::nano>;

		const auto	time = std::chrono::duration_cast<SecondsD_t>( value ).count();
		String		str;

		if ( time > 59.0 * 60.0 )
			str << ToString( time * (1.0/3600.0), precission ) << " h";
		else
		if ( time > 59.0 )
			str << ToString( time * (1.0/60.0), precission ) << " m";
		else
		if ( time > 1.0e-1 )
			str << ToString( time, precission ) << " s";
		else
		if ( time > 1.0e-4 )
			str << ToString( time * 1.0e+3, precission ) << " ms";
		else
		if ( time > 1.0e-7 )
			str << ToString( std::chrono::duration_cast<MicroSecD_t>( value ).count(), precission ) << " us";
		else
			str << ToString( std::chrono::duration_cast<NanoSecD_t>( value ).count(), precission ) << " ns";

		return str;
	}
//-----------------------------------------------------------------------------

	

/*
=================================================
	StringTo***
=================================================
*/
	ND_ inline int  StringToInt (StringView str, int base = 10)
	{
		ASSERT( base == 10 or base == 16 );
		int		val = 0;
		std::from_chars( str.data(), str.data() + str.size(), OUT val, base );
		return val;
	}
	
	ND_ inline uint  StringToUInt (StringView str, int base = 10)
	{
		ASSERT( base == 10 or base == 16 );
		uint	val = 0;
		std::from_chars( str.data(), str.data() + str.size(), OUT val, base );
		return val;
	}
	
	ND_ inline ulong  StringToUInt64 (StringView str, int base = 10)
	{
		ASSERT( base == 10 or base == 16 );
		ulong	val = 0;
		std::from_chars( str.data(), str.data() + str.size(), OUT val, base );
		return val;
	}
	/*
	ND_ inline float  StringToFloat (StringView str)
	{
		float	val = 0.0f;
		std::from_chars( str.data(), str.data() + str.size(), OUT val, std::chars_format::general );
		return val;
	}
	
	ND_ inline double  StringToDouble (StringView str)
	{
		double	val = 0.0;
		std::from_chars( str.data(), str.data() + str.size(), OUT val, std::chars_format::general );
		return val;
	}
	*/

}	// AE::STL
