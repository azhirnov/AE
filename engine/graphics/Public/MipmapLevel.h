// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "graphics/Public/Common.h"

namespace AE::Graphics
{

	//
	// Mipmap Level
	//
	
	struct MipmapLevel
	{
	// variables
	private:
		uint		_value;


	// methods
	public:
		constexpr MipmapLevel () : _value(0) {}

		explicit constexpr MipmapLevel (uint value) : _value(value) {}

		ND_ constexpr uint	Get ()								 const		{ return _value; }
		
		ND_ constexpr bool	operator == (const MipmapLevel &rhs) const		{ return _value == rhs._value; }
		ND_ constexpr bool	operator != (const MipmapLevel &rhs) const		{ return _value != rhs._value; }
		ND_ constexpr bool	operator >  (const MipmapLevel &rhs) const		{ return _value >  rhs._value; }
		ND_ constexpr bool	operator <  (const MipmapLevel &rhs) const		{ return _value <  rhs._value; }
		ND_ constexpr bool	operator >= (const MipmapLevel &rhs) const		{ return _value >= rhs._value; }
		ND_ constexpr bool	operator <= (const MipmapLevel &rhs) const		{ return _value <= rhs._value; }
	};

	
	ND_ inline constexpr MipmapLevel operator "" _mipmap (unsigned long long value)		{ return MipmapLevel( uint(value) ); }


}	// AE::Graphics


namespace std
{
	template <>
	struct hash< AE::Graphics::MipmapLevel >
	{
		ND_ size_t  operator () (const AE::Graphics::MipmapLevel &value) const
		{
			return size_t(AE::STL::HashOf( value.Get() ));
		}
	};

}	// std
