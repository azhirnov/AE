// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "graphics/Public/Common.h"

namespace AE::Graphics
{

	//
	// Multi Samples
	//
	
	struct MultiSamples
	{
	// variables
	private:
		uint	_value;


	// methods
	public:
		constexpr MultiSamples () : _value(0) {}

		explicit MultiSamples (uint samples) : _value( IntLog2( samples )) {}

		ND_ constexpr uint	Get ()								const		{ return 1u << _value; }
		ND_ constexpr uint	GetPowerOf2 ()						const		{ return _value; }

		ND_ constexpr bool	IsEnabled ()						const		{ return _value > 0; }

		ND_ constexpr bool	operator == (const MultiSamples &rhs) const		{ return _value == rhs._value; }
		ND_ constexpr bool	operator != (const MultiSamples &rhs) const		{ return _value != rhs._value; }
		ND_ constexpr bool	operator >  (const MultiSamples &rhs) const		{ return _value >  rhs._value; }
		ND_ constexpr bool	operator <  (const MultiSamples &rhs) const		{ return _value <  rhs._value; }
		ND_ constexpr bool	operator >= (const MultiSamples &rhs) const		{ return _value >= rhs._value; }
		ND_ constexpr bool	operator <= (const MultiSamples &rhs) const		{ return _value <= rhs._value; }
	};
	

	ND_ inline MultiSamples operator "" _samples (unsigned long long value)	{ return MultiSamples( uint(value) ); }


}	// AE::Graphics


namespace std
{
	template <>
	struct hash< AE::Graphics::MultiSamples >
	{
		ND_ size_t  operator () (const AE::Graphics::MultiSamples &value) const
		{
			return size_t(AE::STL::HashOf( value.Get() ));
		}
	};

}	// std
