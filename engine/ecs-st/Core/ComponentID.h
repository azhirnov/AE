// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "ecs-st/Common.h"

namespace AE::ECS
{

	//
	// Component ID
	//

	struct ComponentID
	{
		uint16_t	value;

		constexpr explicit ComponentID (uint16_t id) : value{id} {}
		
		ND_ bool  operator <  (const ComponentID &rhs) const	{ return value <  rhs.value; }
		ND_ bool  operator >  (const ComponentID &rhs) const	{ return value >  rhs.value; }
		ND_ bool  operator == (const ComponentID &rhs) const	{ return value == rhs.value; }
	};
	


	//
	// Component Type Info
	//
	
	template <typename Comp>
	struct ComponentTypeInfo
	{
		STATIC_ASSERT( std::is_standard_layout_v<Comp> );
		STATIC_ASSERT( std::is_trivially_copyable_v<Comp> );
		STATIC_ASSERT( std::is_trivially_destructible_v<Comp> );
		STATIC_ASSERT( std::is_nothrow_destructible_v<Comp> );

		using type	= Comp;
		static inline const ComponentID		id {CheckCast<uint16_t>( _ae_stl_hidden_::StaticTypeIdOf< Comp, 0x1000 >::Get().Get() )};

		static void Ctor (OUT void *comp)
		{
			PlacementNew<Comp>( comp );
		}
	};


}	// AE::ECS

namespace std
{
	template <>
	struct hash< AE::ECS::ComponentID >
	{
		ND_ size_t  operator () (const AE::ECS::ComponentID &id) const
		{
			return id.value;
		}
	};

}	// std
