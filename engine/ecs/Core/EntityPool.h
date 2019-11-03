// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "ecs/Core/Archetype.h"

namespace AE::ECS
{

	//
	// Entity Pool
	//

	struct EntityPool
	{
	// types
	public:
		using Self		= EntityPool;
		using Index_t	= EntityID::Index_t;

	private:
		using InstanceID_t	= EntityID::InstanceID_t;
		using LocalIndex_t	= ArchetypeStorage::Index_t;

		struct EntityRef
		{
			ArchetypeStorage*	storage		= null;
			LocalIndex_t		index		= LocalIndex_t(~0u);
			InstanceID_t		instance	= 0;
		};


	// variables
	private:
		Array<EntityRef>	_entities;
		Array<Index_t>		_available;


	// methods
	public:
		EntityPool () {}
		~EntityPool ();

		ND_ EntityID	Assign ();
			bool		Unassign (EntityID id);
			bool		SetArchetype (EntityID id, ArchetypeStorage* group, LocalIndex_t index);
	};


	
	inline EntityID  EntityPool::Assign ()
	{
		Index_t	idx;

		if ( _available.size() ) {
			idx = _available.back();
			_available.pop_back();
		} else {
			idx = Index_t(_entities.size());
			//_entities.push_back( EntityRef{} );
		}

		return EntityID{ idx, _entities[idx].instance };
	}
	
	inline bool  EntityPool::Unassign (EntityID id)
	{
		/*CHECK_ERR( id.Index() < _entities.size() );
		CHECK_ERR( _entities[idx].instance == id.InstanceID() );
			
		auto& item = _entities[ id.Index() ];

		item.storage = null;
		item.index   = ~LocalIndex_t(0);

		++item.instance;
		_available.push_back( id.Index() );*/
		return true;
	}
	
	inline bool  EntityPool::SetArchetype (EntityID id, ArchetypeStorage* storage, LocalIndex_t index)
	{
		/*CHECK_ERR( id.Index() < _entities.size() );
		CHECK_ERR( _entities[ id.Index() ].instance == id.InstanceID() );

		auto& item   = _entities[ id.Index() ];
		item.storage = storage;
		item.index   = index;*/
		return true;
	}

}	// AE::ECS
