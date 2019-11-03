// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "scripting/Impl/ClassBinder.h"
#include "stl/Algorithms/StringUtils.h"

namespace AE::Scripting
{

	//
	// Enum Binder
	//
	template <typename T>
	class EnumBinder final
	{
	// types
	public:
		using Self		= EnumBinder<T>;
		using Enum_t	= T;


	// variables
	private:
		ScriptEnginePtr		_engine;
		String				_name;


	// methods
	public:
		explicit EnumBinder (const ScriptEnginePtr &eng);

		bool Create ();

		bool AddValue (StringView name, T value);
	};



	//
	// Enum Bitfield Binder
	//
	template <typename T>
	class EnumBitfieldBinder
	{
	// types
	public:
		using Self			= EnumBitfieldBinder<T>;
		using Bitfield_t	= T;
		using Enum_t		= typename Bitfield_t::Index_t;


	// variables
	private:
		ClassBinder<T>	_binder;


	// methods
	public:
		explicit EnumBitfieldBinder (const ScriptEnginePtr &eng);

		// operators
		Self&	Op_And ();
		Self&	Op_Or ();
		Self&	Op_Xor ();

		// methods
		Self&	Op_Set ();
		Self&	Op_Reset ();
		Self&	Op_Get ();

		Self&	BindDefaults ();

	private:
		static Bitfield_t _EnumAndBitfield (Enum_t lhs, const Bitfield_t &rhs)		{ return lhs & rhs; }
		static Bitfield_t _EnumOrBitfield (Enum_t lhs, const Bitfield_t &rhs)		{ return lhs | rhs; }
		static Bitfield_t _EnumXorBitfield (Enum_t lhs, const Bitfield_t &rhs)		{ return lhs ^ rhs; }
	};
//-----------------------------------------------------------------------------


	
/*
=================================================
	constructor
=================================================
*/
	template <typename T>
	inline EnumBinder<T>::EnumBinder (const ScriptEnginePtr &eng) :
		_engine{ eng }
	{
		ScriptTypeInfo< T >::Name( OUT _name );
	}

/*
=================================================
	Create
=================================================
*/
	template <typename T>
	inline bool EnumBinder<T>::Create ()
	{
		int	res = _engine->Get()->RegisterEnum( _name.c_str() );
		
		if ( res < 0 and res != AngelScript::asALREADY_REGISTERED )
		{
			AS_CALL( res );
			RETURN_ERR( String("enum '") << _name << "' already registerd" );
		}
		return true;
	}
	
/*
=================================================
	AddValue
=================================================
*/
	template <typename T>
	inline bool EnumBinder<T>::AddValue (StringView valueName, T value)
	{
		ASSERT( int64_t(value) >= std::numeric_limits<int>::min() and
				int64_t(value) <= std::numeric_limits<int>::max() );

		AS_CALL_R( _engine->Get()->RegisterEnumValue( _name.c_str(), (String(_name) << '_' << valueName).c_str(), int(value) ));
		return true;
	}
//-----------------------------------------------------------------------------

	

/*
=================================================
	constructor
=================================================
*/
	template <typename T>
	inline EnumBitfieldBinder<T>::EnumBitfieldBinder (const ScriptEnginePtr &eng) :
		_binder( eng )
	{
		_binder.CreateClassValue();

		struct Ctors {
			static void FromEnum (void *dst, Enum_t e) {
				new(dst) Bitfield_t{ e };
			}
		};

		_binder.AddConstructor( &Ctors::FromEnum );
	}

/*
=================================================
	operators
=================================================
*/
	template <typename T>
	inline EnumBitfieldBinder<T>&  EnumBitfieldBinder<T>::Op_And ()
	{
		_binder.Operators()
			.BinaryRH( EBinaryOperator::And, &_EnumAndBitfield )
			.template And<Enum_t>()		.template AndAssign<Enum_t>()
			.template And<Bitfield_t>()	.template AndAssign<Bitfield_t>();

		return *this;
	}

	template <typename T>
	inline EnumBitfieldBinder<T>&  EnumBitfieldBinder<T>::Op_Or ()
	{
		_binder.Operators()
			.BinaryRH( EBinaryOperator::Or, &_EnumOrBitfield )
			.template Or<Enum_t>()		.template OrAssign<Enum_t>()
			.template Or<Bitfield_t>()	.template OrAssign<Bitfield_t>();

		return *this;
	}

	template <typename T>
	inline EnumBitfieldBinder<T>&  EnumBitfieldBinder<T>::Op_Xor ()
	{
		_binder.Operators()
			.BinaryRH( EBinaryOperator::Xor, &_EnumXorBitfield )
			.template Xor<Enum_t>()		.template XorAssign<Enum_t>()
			.template Xor<Bitfield_t>()	.template XorAssign<Bitfield_t>();

		return *this;
	}
	
/*
=================================================
	methods
=================================================
*/
	template <typename T>
	inline EnumBitfieldBinder<T>&  EnumBitfieldBinder<T>::Op_Set ()
	{
		_binder.AddMethod( &Bitfield_t::Set, "Set" );
		return *this;
	}

	template <typename T>
	inline EnumBitfieldBinder<T>&  EnumBitfieldBinder<T>::Op_Reset ()
	{
		_binder.AddMethod( &Bitfield_t::Reset, "Reset" );
		return *this;
	}

	template <typename T>
	inline EnumBitfieldBinder<T>&  EnumBitfieldBinder<T>::Op_Get ()
	{
		_binder.AddMethod( &Bitfield_t::Get, "Get" );
		return *this;
	}
	
/*
=================================================
	BindDefaults
=================================================
*/
	template <typename T>
	inline EnumBitfieldBinder<T>&  EnumBitfieldBinder<T>::BindDefaults ()
	{
		return (*this).Op_Set().Op_Reset().Op_Get().Op_Or();
	}

}	// AE::Scripting
