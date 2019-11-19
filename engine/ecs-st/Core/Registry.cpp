// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "ecs-st/Core/Registry.h"

namespace AE::ECS
{

/*
=================================================
	constructor
=================================================
*/
	Registry::Registry ()
	{
		EXLOCK( _drCheck );
	}
	
/*
=================================================
	destructor
=================================================
*/
	Registry::~Registry ()
	{
		EXLOCK( _drCheck );
		ASSERT( _singleComponents.empty() );
	}
	
/*
=================================================
	GetArchetype
=================================================
*/
	Archetype const*  Registry::GetArchetype (EntityID id)
	{
		ArchetypeStorage*	storage	= null;
		Index_t				index;
		_entities.GetArchetype( id, OUT storage, OUT index );

		return storage ? &storage->GetArchetype() : null;
	}

/*
=================================================
	CreateEntity
=================================================
*/
	EntityID  Registry::CreateEntity ()
	{
		EXLOCK( _drCheck );

		EntityID	id;
		CHECK_ERR( _entities.Assign( OUT id ));
		return id;
	}
	
/*
=================================================
	DestroyEntity
=================================================
*/
	bool  Registry::DestroyEntity (EntityID id)
	{
		EXLOCK( _drCheck );

		CHECK( _RemoveEntity( id ));
		return _entities.Unassign( id );
	}
	
/*
=================================================
	DestroyAllEntities
=================================================
*/
	void  Registry::DestroyAllEntities ()
	{
		EXLOCK( _drCheck );

		for (auto& [arch, storages] : _archetypes)
		{
			for (auto& st : storages)
			{
				// TODO: add message with MsgTag_RemovedComponent ???

				st->Clear();
			}
		}

		_entities.Clear();
		_archetypes.clear();
	}
	
/*
=================================================
	DestroyAllSingleComponents
=================================================
*/
	void  Registry::DestroyAllSingleComponents ()
	{
		EXLOCK( _drCheck );

		_singleComponents.clear();
	}

/*
=================================================
	_RemoveEntity
=================================================
*/
	bool  Registry::_RemoveEntity (EntityID id)
	{
		ArchetypeStorage*	storage = null;
		Index_t				index;

		if ( _entities.GetArchetype( id, OUT storage, OUT index ) and storage )
		{
			ASSERT( storage->IsValid( id, index ));

			// add messages
			{
				auto&	arch = storage->GetArchetype();

				for (auto& comp : arch.Desc().components)
				{
					void*	comp_ptr = storage->GetComponents( comp.id );
					_messages.Add<MsgTag_RemovedComponent>( id, comp.id, ArrayView<uint8_t>{ Cast<uint8_t>(comp_ptr), size_t(comp.size) });
				}

				for (auto& tag : arch.Desc().tags) {
					_messages.Add<MsgTag_RemovedComponent>( id, tag );
				}
			}

			EntityID	moved;
			if ( not storage->Erase( index, OUT moved ))
				return false;
			
			// update reference to entity that was moved to new index
			if ( moved )
				_entities.SetArchetype( moved, storage, index );

			_entities.SetArchetype( id, null, Index_t(-1) );
			
			// remove empty storage
			/*if ( storage->Empty() )
			{
				auto&	storages = _archetypes[ storage->GetArchetype() ];

				for (auto iter = storages.begin(); iter != storages.end(); ++iter) {
					if ( iter->get() == storage ) {
						storages.erase( iter );
						break;
					}
				}
			}*/
		}
		return true;
	}

/*
=================================================
	_AddEntity
=================================================
*/
	void  Registry::_AddEntity (const Archetype &arch, EntityID id, OUT ArchetypeStorage* &outStorage, OUT Index_t &index)
	{
		auto					[iter, inserted] = _archetypes.insert({ arch, ArchetypeStorages_t{} });
		Archetype const&		key				 = iter->first;
		ArchetypeStorages_t&	storages		 = iter->second;

		if ( inserted )
		{
			// TODO: new archetype event
		}

		for (auto& storage : storages)
		{
			if ( storage->Add( id, OUT index ))
			{
				outStorage = storage.get();
				_entities.SetArchetype( id, storage.get(), index );
				return;
			}
		}

		auto	storage = MakeUnique<ArchetypeStorage>( key, ECS_Config::DefaultStorageSize );
		
		CHECK( storage->Add( id, OUT index ));
		_entities.SetArchetype( id, storage.get(), index );
		
		outStorage = storage.get();
		storages.push_back( std::move(storage) );
	}
	
	void  Registry::_AddEntity (const Archetype &arch, EntityID id)
	{
		ArchetypeStorage*	storage = null;
		Index_t				index;
		return _AddEntity( arch, id, OUT storage, OUT index );
	}

/*
=================================================
	Process
=================================================
*/
	void  Registry::Process ()
	{
		EXLOCK( _drCheck );

		const auto	FlushEvents = [this] ()
		{
			for (auto iter = _pendingEvents.rbegin(); iter != _pendingEvents.rend(); ++iter) {
				_eventQueue.push_back( std::move(*iter) );
			}
			_pendingEvents.clear();
		};

		//CHECK( _pendingEvents.empty() );
		CHECK( _eventQueue.empty() );
		
		_messages.Process();
		FlushEvents();

		for (; _eventQueue.size();)
		{
			auto	fn = std::move( _eventQueue.back() );
			_eventQueue.pop_back();

			fn();
			_messages.Process();
			
			FlushEvents();
		}
	}

}	// AE::ECS
