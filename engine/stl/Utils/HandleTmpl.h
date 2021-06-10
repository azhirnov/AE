// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

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
		static constexpr Value_t	_GenMask	= (1 << sizeof(Index_t)*8) - 1;
		static constexpr Value_t	_GenOffset	= sizeof(Index_t)*8;


	// methods
	public:
		constexpr HandleTmpl () {}
		constexpr HandleTmpl (const Self &other) : _value{other._value} {}

		constexpr HandleTmpl (Value_t index, Value_t gen) :
			_value{Value_t(index) | (Value_t(gen) << _GenOffset)} {}

		ND_ constexpr bool					IsValid ()						const	{ return _value != UMax; }
		ND_ constexpr Index_t				Index ()						const	{ return _value & _IndexMask; }
		ND_ constexpr Generation_t			Generation ()					const	{ return _value >> _GenOffset; }
		ND_ HashVal							GetHash ()						const	{ return HashOf(_value); }
		ND_ constexpr Value_t				Data ()							const	{ return _value; }

		ND_ constexpr bool					operator == (const Self &rhs)	const	{ return _value == rhs._value; }
		ND_ constexpr bool					operator != (const Self &rhs)	const	{ return not (*this == rhs); }

		ND_ explicit constexpr				operator bool ()				const	{ return IsValid(); }

		ND_ static constexpr Index_t		MaxIndex ()								{ return _IndexMask; }
		ND_ static constexpr Generation_t	MaxGeneration ()						{ return _GenMask; }


	// debugging
	public:
		DEBUG_ONLY(
			class IDbgBaseClass;
			IDbgBaseClass*	_class = null;
		)
	};
	


	//
	// Unique ID
	//

	template <typename IDType>
	struct UniqueID
	{
	// types
	public:
		using ID_t	= IDType;
		using Self	= UniqueID< IDType >;


	// variables
	private:
		ID_t	_id;


	// methods
	public:
		UniqueID ()													{}
		UniqueID (Self &&other) : _id{other._id}					{ other._id = Default; }
		explicit UniqueID (const ID_t &id) : _id{id}				{}
		~UniqueID ()												{ ASSERT(not IsValid()); }	// handle must be released
		
		Self&				Set (ID_t id)							{ ASSERT(not IsValid());  _id = id;  return *this; }

		Self&				operator = (Self &&rhs)					{ ASSERT(not IsValid());  _id = rhs._id;  rhs._id = Default;  return *this; }
		Self&				operator = (const Self &rhs)			{ ASSERT(not IsValid());  _id = rhs._id;  rhs._id = Default;  return *this; }
		
		ND_ ID_t			Release ()								{ ID_t temp{_id};  _id = Default;  return temp; }
		ND_ bool			IsValid ()						const	{ return bool(_id); }

		ND_ ID_t const&		operator * ()					const	{ return _id; }
		ND_ ID_t const*		operator -> ()					const	{ return &_id; }

		ND_ bool			operator == (const Self &rhs)	const	{ return _id == rhs._id; }
		ND_ bool			operator != (const Self &rhs)	const	{ return _id != rhs._id; }

		ND_ explicit		operator bool ()				const	{ return IsValid(); }

		ND_					operator ID_t ()				const	{ return _id; }
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
