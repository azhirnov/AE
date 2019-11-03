// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "ecs/Common.h"

namespace AE::ECS
{
	
	//
	// Entity ID
	//

	struct EntityID
	{
	// types
	public:
		using Self			= EntityID;
		using Index_t		= uint16_t;
		using InstanceID_t	= uint16_t;
		using Value_t		= uint32_t;


	// variables
	private:
		Value_t		_value	= UMax;

		STATIC_ASSERT( sizeof(_value) == (sizeof(Index_t) + sizeof(InstanceID_t)) );

		static constexpr Index_t	_IndexMask	= (1 << sizeof(Index_t)*8) - 1;
		static constexpr Value_t	_InstOffset	= sizeof(Index_t)*8;


	// methods
	public:
		constexpr EntityID () {}
		constexpr EntityID (const EntityID &other) : _value{other._value} {}
		explicit constexpr EntityID (Value_t data) : _value{ data } {}
		explicit constexpr EntityID (Index_t val, InstanceID_t inst) : _value{Value_t(val) | (Value_t(inst) << _InstOffset)} {}

		ND_ constexpr bool			IsValid ()						const	{ return _value != UMax; }
		ND_ constexpr Index_t		Index ()						const	{ return _value & _IndexMask; }
		ND_ constexpr InstanceID_t	InstanceID ()					const	{ return _value >> _InstOffset; }
		ND_ HashVal					GetHash ()						const	{ return HashOf(_value); }
		ND_ constexpr Value_t		Data ()							const	{ return _value; }

		ND_ constexpr bool			operator == (const Self &rhs)	const	{ return _value == rhs._value; }
		ND_ constexpr bool			operator != (const Self &rhs)	const	{ return not (*this == rhs); }

		ND_ explicit constexpr		operator bool ()				const	{ return IsValid(); }
	};


	//
	// Component ID
	//

	struct ComponentID
	{
		uint	value;

		constexpr explicit ComponentID (uint id) : value{id} {}
	};


	//
	// Component Type Info
	//

	template <typename Comp, typename Ppln>
	struct ComponentTypeInfo
	{
		//using type	= T;
		static constexpr ComponentID	id {Ppln::Counter_t::Next()};
	};


	//
	// Archetype
	//

	struct ArchetypeID
	{
		uint	id;
	};


	//
	// Archetype Description
	//

	struct AchetypeDesc
	{
	// types
		struct ComponentInfo
		{
			ComponentID			id;
			Bytes<uint16_t>		align;
			Bytes<uint16_t>		size;
		};

	// variables
		Array<ComponentInfo> components;
	};


	//
	// Archetype Storage
	//

	class ArchetypeStorage
	{
	// types
	public:
		enum class Index_t : uint {};
		

	// variables
	private:
		Array<uint8_t>		_componentsMem;
		Array<EntityID>		_entities;		// local index to EntityID
		AchetypeDesc		_desc;


	// methods
	public:

	};

}	// AE::ECS
