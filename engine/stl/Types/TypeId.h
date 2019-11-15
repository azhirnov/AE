// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Common.h"

namespace AE::STL::_ae_stl_hidden_
{
	//
	// Static Type ID
	//
	template <uint UID>
	struct StaticTypeID final
	{
	private:
		size_t	_value;

	public:
		constexpr StaticTypeID () : _value{~size_t(0)} {}

		ND_ constexpr bool  operator == (StaticTypeID<UID> rhs) const	{ return _value == rhs._value; }
		ND_ constexpr bool  operator != (StaticTypeID<UID> rhs) const	{ return _value != rhs._value; }
		ND_ constexpr bool  operator >  (StaticTypeID<UID> rhs) const	{ return _value >  rhs._value; }
		ND_ constexpr bool  operator <  (StaticTypeID<UID> rhs) const	{ return _value <  rhs._value; }
		ND_ constexpr bool  operator >= (StaticTypeID<UID> rhs) const	{ return _value >= rhs._value; }
		ND_ constexpr bool  operator <= (StaticTypeID<UID> rhs) const	{ return _value <= rhs._value; }

		ND_ constexpr size_t		Get ()	const						{ return _value; }
		ND_ constexpr const char *	Name ()	const						{ return ""; }
	};
	
	template <uint UID>
	struct BaseStaticTypeIdOf
	{
	protected:
		static inline size_t	_counter = 0;
	};

	template <typename T, uint UID>
	struct StaticTypeIdOf : BaseStaticTypeIdOf<UID>
	{
		ND_ static constexpr StaticTypeID<UID>  Get ()
		{
			static size_t id = _counter++;
			return BitCast< StaticTypeID<UID> >( id );
		}
	};

	template <typename T, uint UID>	struct StaticTypeIdOf< const T,          UID > final : StaticTypeIdOf<T, UID> {};
	template <typename T, uint UID>	struct StaticTypeIdOf< volatile T,       UID > final : StaticTypeIdOf<T, UID> {};
	template <typename T, uint UID>	struct StaticTypeIdOf< const volatile T, UID > final : StaticTypeIdOf<T, UID> {};

}	// AE::STL::_ae_stl_hidden_

namespace std
{
	template <uint32_t UID>
	struct hash< AE::STL::_ae_stl_hidden_::StaticTypeID<UID> >
	{
		ND_ size_t  operator () (const AE::STL::_ae_stl_hidden_::StaticTypeID<UID> &value) const
		{
			return value.Get();
		}
	};

}	// std
//-----------------------------------------------------------------------------

	
namespace AE::STL::_ae_stl_hidden_
{
	//
	// STD Type ID
	//
	struct StdTypeID final
	{
	private:
		enum UnknownType {};

		std::type_index		_value;

	public:
		StdTypeID () : _value{ typeid(UnknownType) } {}
		StdTypeID (const std::type_index &value) : _value{ value } {}
			
		ND_ bool  operator == (StdTypeID rhs) const		{ return _value == rhs._value; }
		ND_ bool  operator != (StdTypeID rhs) const		{ return _value != rhs._value; }
		ND_ bool  operator >  (StdTypeID rhs) const		{ return _value >  rhs._value; }
		ND_ bool  operator <  (StdTypeID rhs) const		{ return _value <  rhs._value; }
		ND_ bool  operator >= (StdTypeID rhs) const		{ return _value >= rhs._value; }
		ND_ bool  operator <= (StdTypeID rhs) const		{ return _value <= rhs._value; }

		ND_ std::type_index		Get ()	const			{ return _value; }
		ND_ const char *		Name ()	const			{ return _value.name(); }
	};

		
	template <typename T>
	struct StdTypeIdOf final
	{
		ND_ static StdTypeID  Get ()
		{
			return StdTypeID( typeid(T) );
		}
	};

}	// AE::STL::_ae_stl_hidden_

namespace std
{
	template <>
	struct hash< AE::STL::_ae_stl_hidden_::StdTypeID >
	{
		ND_ size_t  operator () (const AE::STL::_ae_stl_hidden_::StdTypeID &value) const
		{
			return std::hash< std::type_index >{}( value.Get() );
		}
	};

}	// std


#if 0
namespace AE::STL
{
	using TypeId = _ae_stl_hidden_::StaticTypeID<0>;
	
/*
=================================================
	TypeIdOf
=================================================
*/
	template <typename T>
	ND_ forceinline static TypeId  TypeIdOf ()
	{
		return _ae_stl_hidden_::StaticTypeIdOf<T,0>::Get();
	}

	template <typename T>
	ND_ forceinline static TypeId  TypeIdOf (const T&)
	{
		return TypeIdOf<T>();
	}

}	// AE::STL

#else
namespace AE::STL
{
	using TypeId = _ae_stl_hidden_::StdTypeID;
/*
=================================================
	TypeIdOf
=================================================
*/
	template <typename T>
	ND_ forceinline static TypeId  TypeIdOf ()
	{
		return _ae_stl_hidden_::StdTypeIdOf<T>::Get();
	}

	template <typename T>
	ND_ forceinline static TypeId  TypeIdOf (const T&)
	{
		return TypeIdOf<T>();
	}

}	// AE::STL

#endif
