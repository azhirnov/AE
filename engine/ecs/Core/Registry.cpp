// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "ecs/Core/Registry.h"

namespace AE::ECS
{
	
/*
=================================================
	CreateEntity
=================================================
*/
	EntityID  Registry::CreateEntity ()
	{
		EntityID	id = _entities.Assign();
		//_entities.SetArchetype( id, ... );
		return id;
	}
	
/*
=================================================
	DestroyEntity
=================================================
*/
	void  Registry::DestroyEntity (EntityID id)
	{
		_entities.Unassign( id );
	}

}	// AE::ECS
