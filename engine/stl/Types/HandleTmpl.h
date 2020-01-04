// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Common.h"

namespace AE::STL
{

	//
	// Handle template
	//

	template <typename IndexType, typename GenerationType, uint UID>
	struct HandleTmpl
	{
	// types
	public:
		using Self			= HandleTmpl< IndexType, GenerationType, UID >;
		using Index_t		= IndexType;
		using Generation_t	= GenerationType;
		using Value_t		= BitSizeToUInt< sizeof(Index_t) + sizeof(Generation_t) >;


	// variables
	private:
		Value_t		_value	= UMax;
		
		STATIC_ASSERT( sizeof(_value) >= (sizeof(Index_t) + sizeof(Generation_t)) );

		static constexpr Value_t	_IndexMask	= (1 << sizeof(Index_t)*8) - 1;
		static constexpr Value_t	_GenOffset	= sizeof(Index_t)*8;


	// methods
	public:
		constexpr HandleTmpl () {}
		constexpr HandleTmpl (const Self &other) : _value{other._value} {}

		constexpr HandleTmpl (Value_t index, Value_t gen) :
			_value{Value_t(index) | (Value_t(gen) << _GenOffset)}
		{
			ASSERT( index == Index() );
			ASSERT( gen == Generation() );
		}

		ND_ constexpr bool			IsValid ()						const	{ return _value != UMax; }
		ND_ constexpr Index_t		Index ()						const	{ return _value & _IndexMask; }
		ND_ constexpr Generation_t	Generation ()					const	{ return _value >> _GenOffset; }
		ND_ HashVal					GetHash ()						const	{ return HashOf(_value); }
		ND_ constexpr Value_t		Data ()							const	{ return _value; }

		ND_ constexpr bool			operator == (const Self &rhs)	const	{ return _value == rhs._value; }
		ND_ constexpr bool			operator != (const Self &rhs)	const	{ return not (*this == rhs); }

		ND_ explicit constexpr		operator bool ()				const	{ return IsValid(); }


	// debugging
	public:
		DEBUG_ONLY(
			class IDbgBaseClass;
			IDbgBaseClass*	_class = null;
		)
	};

}	// AE::STL

namespace std
{
	template <typename IndexType, typename GenerationType, uint32_t UID>
	struct hash< AE::STL::HandleTmpl<IndexType, GenerationType, UID> >
	{
		ND_ size_t  operator () (const AE::STL::HandleTmpl<IndexType, GenerationType, UID> &value) const
		{
			return size_t(value.GetHash());
		}
	};

}	// std
