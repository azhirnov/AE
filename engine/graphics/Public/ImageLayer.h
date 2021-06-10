// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "graphics/Public/Common.h"

namespace AE::Graphics
{

	//
	// Image Array Layer
	//
	
	struct ImageLayer
	{
	// variables
	private:
		uint		_value	= 0;


	// methods
	public:
		constexpr ImageLayer () {}
		explicit constexpr ImageLayer (uint value) : _value(value) {}

		ND_ constexpr uint	Get ()								const	{ return _value; }
		
		ND_ ImageLayer		operator + (const ImageLayer &rhs)	const	{ return ImageLayer{ Get() + rhs.Get() }; }

		ND_ constexpr bool	operator == (const ImageLayer &rhs) const	{ return _value == rhs._value; }
		ND_ constexpr bool	operator != (const ImageLayer &rhs) const	{ return _value != rhs._value; }
		ND_ constexpr bool	operator >  (const ImageLayer &rhs) const	{ return _value >  rhs._value; }
		ND_ constexpr bool	operator <  (const ImageLayer &rhs) const	{ return _value <  rhs._value; }
		ND_ constexpr bool	operator >= (const ImageLayer &rhs) const	{ return _value >= rhs._value; }
		ND_ constexpr bool	operator <= (const ImageLayer &rhs) const	{ return _value <= rhs._value; }
	};
	

	ND_ inline constexpr ImageLayer operator "" _layer (unsigned long long value)	{ return ImageLayer( uint(value) ); }


}	// AE::Graphics


namespace std
{
	template <>
	struct hash< AE::Graphics::ImageLayer >
	{
		ND_ size_t  operator () (const AE::Graphics::ImageLayer &value) const
		{
			return size_t(AE::STL::HashOf( value.Get() ));
		}
	};

}	// std
