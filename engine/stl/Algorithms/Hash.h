// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include <functional>
#include "stl/Log/Log.h"
#include "stl/CompileTime/TypeTraits.h"

namespace AE::STL
{

	//
	// Hash Value
	//

	struct HashVal
	{
	// variables
	private:
		usize		_value	= 0;

	// methods
	public:
		constexpr HashVal () {}
		explicit constexpr HashVal (usize val) : _value{val} {}

		ND_ constexpr bool	operator == (const HashVal &rhs)	const	{ return _value == rhs._value; }
		ND_ constexpr bool	operator != (const HashVal &rhs)	const	{ return not (*this == rhs); }
		ND_ constexpr bool	operator >  (const HashVal &rhs)	const	{ return _value > rhs._value; }
		ND_ constexpr bool  operator <  (const HashVal &rhs)	const	{ return _value < rhs._value; }

		constexpr HashVal&	operator << (const HashVal &rhs)
		{
			const usize	mask	= (sizeof(_value)*8 - 1);
			usize			val		= rhs._value;
			usize			shift	= 8;

			shift &= mask;
			_value ^= (val << shift) | (val >> ( ~(shift-1) & mask ));	// TODO: add constant

			return *this;
		}

		ND_ constexpr const HashVal  operator + (const HashVal &rhs) const
		{
			return HashVal(*this) << rhs;
		}

		ND_ explicit constexpr operator usize () const	{ return _value; }
	};
//-----------------------------------------------------------------------------


	
/*
=================================================
	HashOf
=================================================
*/
	template <typename T>
	ND_ forceinline EnableIf<not IsFloatPoint<T>, HashVal>  HashOf (const T &value)
	{
		return HashVal( std::hash<T>()( value ));
	}

/*
=================================================
	HashOf (float)
=================================================
*/
	ND_ forceinline HashVal  HashOf (const float &value, uint ignoreMantissaBits = (23-10))
	{
		ASSERT( ignoreMantissaBits < 23 );
		uint	dst;
		std::memcpy( OUT &dst, &value, sizeof(dst) );
		dst &= ~((1 << ignoreMantissaBits)-1);
		return HashVal( std::hash<uint>()( dst ));
	}

/*
=================================================
	HashOf (double)
=================================================
*/
	ND_ forceinline HashVal  HashOf (const double &value, uint ignoreMantissaBits = (52-10))
	{
		ASSERT( ignoreMantissaBits < 52 );
		ulong	dst;
		std::memcpy( OUT &dst, &value, sizeof(dst) );
		dst &= ~((1 << ignoreMantissaBits)-1);
		return HashVal( std::hash<ulong>()( dst ));
	}

/*
=================================================
	HashOf (buffer)
----
	use private api to calculate hash of buffer
=================================================
*/
	ND_ forceinline HashVal  HashOf (const void *ptr, usize sizeInBytes)
	{
		ASSERT( ptr and sizeInBytes );

		# if defined(AE_HAS_HASHFN_HashArrayRepresentation)
			return HashVal{std::_Hash_array_representation( static_cast<const unsigned char*>(ptr), sizeInBytes )};

		#elif defined(AE_HAS_HASHFN_Murmur2OrCityhash)
			return HashVal{std::__murmur2_or_cityhash<usize>()( ptr, sizeInBytes )};

		#elif defined(AE_HAS_HASHFN_HashBytes)
			return HashVal{std::_Hash_bytes( ptr, sizeInBytes, 0 )};

		#else
			AE_COMPILATION_MESSAGE( "used fallback hash function" )
			const ubyte*	buf		= static_cast<const ubyte*>(ptr);
			HashVal			result;
			for (usize i = 0; i < sizeInBytes; ++i) {
				result << HashVal{buf[i]};
			}
			return result;
		#endif
	}

}	// AE::STL


namespace std
{
	template <>
	struct hash< AE::STL::HashVal >
	{
		ND_ size_t  operator () (const AE::STL::HashVal &value) const
		{
			return size_t(value);
		}
	};

	template <typename First, typename Second>
	struct hash< std::pair<First, Second> >
	{
		ND_ size_t  operator () (const std::pair<First, Second> &value) const
		{
			return size_t(AE::STL::HashOf( value.first ) + AE::STL::HashOf( value.second ));
		}
	};

}	// std
