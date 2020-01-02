// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Common.h"

namespace AE::STL
{

	//
	// Handle template
	//

	template <typename IndexType, typename InstanceType, uint UID>
	struct HandleTmpl
	{
	// types
	public:
		using Self			= HandleTmpl< IndexType, InstanceType, UID >;
		using Index_t		= IndexType;
		using InstanceID_t	= InstanceType;
		using Value_t		= BitSizeToUInt< sizeof(Index_t) + sizeof(InstanceID_t) >;


	// variables
	private:
		Value_t		_value	= UMax;

		STATIC_ASSERT( sizeof(_value) >= (sizeof(Index_t) + sizeof(InstanceID_t)) );

		static constexpr Index_t	_IndexMask	= (1 << sizeof(Index_t)*8) - 1;
		static constexpr Value_t	_InstOffset	= sizeof(Index_t)*8;


	// methods
	public:
		constexpr HandleTmpl () {}
		constexpr HandleTmpl (const Self &other) : _value{other._value} {}
		explicit constexpr HandleTmpl (Value_t data) : _value{ data } {}
		explicit constexpr HandleTmpl (Index_t val, InstanceID_t inst) : _value{Value_t(val) | (Value_t(inst) << _InstOffset)} {}

		ND_ constexpr bool			IsValid ()						const	{ return _value != UMax; }
		ND_ constexpr Index_t		Index ()						const	{ return _value & _IndexMask; }
		ND_ constexpr InstanceID_t	InstanceID ()					const	{ return _value >> _InstOffset; }
		ND_ HashVal					GetHash ()						const	{ return HashOf(_value); }
		ND_ constexpr Value_t		Data ()							const	{ return _value; }

		ND_ constexpr bool			operator == (const Self &rhs)	const	{ return _value == rhs._value; }
		ND_ constexpr bool			operator != (const Self &rhs)	const	{ return not (*this == rhs); }

		ND_ explicit constexpr		operator bool ()				const	{ return IsValid(); }
	};

}	// AE::STL

namespace std
{
	template <typename IndexType, typename InstanceType, uint32_t UID>
	struct hash< AE::STL::HandleTmpl<IndexType, InstanceType, UID> >
	{
		ND_ size_t  operator () (const AE::STL::HandleTmpl<IndexType, InstanceType, UID> &value) const
		{
			return size_t(value.GetHash());
		}
	};

}	// std
