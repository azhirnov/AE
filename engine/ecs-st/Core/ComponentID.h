// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "ecs-st/Common.h"

namespace AE::ECS
{

	//
	// Component ID
	//

	namespace _ae_ecs_hidden_
	{
		template <uint UID>
		struct _ComponentID
		{
			uint16_t	value;

			constexpr _ComponentID () : value{UMax} {}
			constexpr explicit _ComponentID (uint16_t id) : value{id} {}

			ND_ constexpr bool  operator <  (const _ComponentID &rhs) const	{ return value <  rhs.value; }
			ND_ constexpr bool  operator >  (const _ComponentID &rhs) const	{ return value >  rhs.value; }
			ND_ constexpr bool  operator == (const _ComponentID &rhs) const	{ return value == rhs.value; }
		};

	}	// _ae_ecs_hidden_
	
	using ComponentID		= _ae_ecs_hidden_::_ComponentID<0>;
	using TagComponentID	= _ae_ecs_hidden_::_ComponentID<1>;
	using MsgTagID			= _ae_ecs_hidden_::_ComponentID<2>;



	//
	// Component Type Info
	//
	
	template <typename Comp>
	struct ComponentTypeInfo
	{
		//STATIC_ASSERT( not IsEmpty<Comp> );
		//STATIC_ASSERT( std::is_standard_layout_v<Comp> );
		//STATIC_ASSERT( std::is_trivially_copyable_v<Comp> );
		STATIC_ASSERT( std::is_trivially_destructible_v<Comp> );
		STATIC_ASSERT( std::is_nothrow_destructible_v<Comp> );

		using type	= Comp;
		static inline const ComponentID		id		{ CheckCast<uint16_t>( _ae_stl_hidden_::StaticTypeIdOf< Comp, 0x1000 >::Get().Get() ) };
		static constexpr Bytes<uint16_t>	align	{ IsEmpty<Comp> ? 0 : alignof(Comp) };
		static constexpr Bytes<uint16_t>	size	{ IsEmpty<Comp> ? 0 : sizeof(Comp) };

		static void Ctor (OUT void *comp)
		{
			PlacementNew<Comp>( comp );
		}
	};
	
	template <typename Comp>	struct ComponentTypeInfo< Comp& >		: ComponentTypeInfo<Comp> {};
	template <typename Comp>	struct ComponentTypeInfo< const Comp& >	: ComponentTypeInfo<Comp> {};
	


	//
	// Message Tag Type Info
	//
	
	template <typename Comp>
	struct MsgTagTypeInfo
	{
		STATIC_ASSERT( IsEmpty<Comp> );

		using type	= Comp;
		static inline const MsgTagID	id {CheckCast<uint16_t>( _ae_stl_hidden_::StaticTypeIdOf< Comp, 0x1002 >::Get().Get() )};
	};

}	// AE::ECS

namespace std
{
	template <uint32_t UID>
	struct hash< AE::ECS::_ae_ecs_hidden_::_ComponentID<UID> >
	{
		ND_ size_t  operator () (const AE::ECS::_ae_ecs_hidden_::_ComponentID<UID> &id) const
		{
			return AE::Math::BitRotateLeft( size_t(id.value), UID*8 );
		}
	};

}	// std
