// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "ecs-st/Common.h"

namespace AE::ECS
{

	//
	// Component ID
	//

	namespace _hidden_
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

	}	// _hidden_
	
	using ComponentID		= ECS::_hidden_::_ComponentID<0>;
	using TagComponentID	= ECS::_hidden_::_ComponentID<1>;
	using MsgTagID			= ECS::_hidden_::_ComponentID<2>;



	//
	// Component Type Info
	//
	
	template <typename Comp>
	struct ComponentTypeInfo
	{
		//STATIC_ASSERT( std::is_trivially_destructible_v<Comp> );
		//STATIC_ASSERT( std::is_trivially_copyable_v<Comp> );
		STATIC_ASSERT( std::is_nothrow_destructible_v<Comp> );

		using type	= Comp;
		static inline const ComponentID		id		{ CheckCast<uint16_t>( STL::_hidden_::StaticTypeIdOf< Comp, 0x1000 >::Get().Get() ) };
		static constexpr TBytes<uint16_t>	align	{ IsEmpty<Comp> ? 0 : alignof(Comp) };
		static constexpr TBytes<uint16_t>	size	{ IsEmpty<Comp> ? 0 : sizeof(Comp) };

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
		static inline const MsgTagID	id {CheckCast<uint16_t>( STL::_hidden_::StaticTypeIdOf< Comp, 0x1002 >::Get().Get() )};
	};

}	// AE::ECS

namespace std
{
	template <uint32_t UID>
	struct hash< AE::ECS::_hidden_::_ComponentID<UID> >
	{
		ND_ size_t  operator () (const AE::ECS::_hidden_::_ComponentID<UID> &id) const
		{
			return AE::Math::BitRotateLeft( size_t(id.value), UID*8 );
		}
	};

}	// std
