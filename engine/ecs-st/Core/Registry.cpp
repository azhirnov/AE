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
	Ptr<Archetype const>  Registry::GetArchetype (EntityID id)
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

		for (auto& [arch, storage] : _archetypes)
		{
			// TODO: add message with MsgTag_RemovedComponent ???

			storage->Clear();
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
			
			_DecreaseStorageSize( storage );
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
		auto					[iter, inserted] = _archetypes.insert({ arch, ArchetypeStoragePtr{} });
		Archetype const&		key				 = iter->first;
		ArchetypeStoragePtr&	storage			 = iter->second;

		if ( inserted )
		{
			storage.reset( new ArchetypeStorage{ key, ECS_Config::InitialtStorageSize });

			// TODO: new archetype event
		}

		if ( storage->Add( id, OUT index ))
		{
			outStorage = storage.get();
			_entities.SetArchetype( id, storage.get(), index );
			return;
		}

		storage->Reserve( storage->Capacity()*2 );
		
		CHECK( storage->Add( id, OUT index ));
		_entities.SetArchetype( id, storage.get(), index );
		
		outStorage = storage.get();
	}
	
	void  Registry::_AddEntity (const Archetype &arch, EntityID id)
	{
		ArchetypeStorage*	storage = null;
		Index_t				index;
		return _AddEntity( arch, id, OUT storage, OUT index );
	}
	
/*
=================================================
	_MoveEntity
=================================================
*/
	void  Registry::_MoveEntity (const Archetype &arch, EntityID id, ArchetypeStorage* srcStorage, Index_t srcIndex,
								 OUT ArchetypeStorage* &dstStorage, OUT Index_t &dstIndex)
	{
		_AddEntity( arch, id, OUT dstStorage, OUT dstIndex );

		if ( srcStorage )
		{
			// copy components
			for (auto& comp : arch.Desc().components)
			{
				void*	src = srcStorage->GetComponents( comp.id );
				void*	dst = dstStorage->GetComponents( comp.id );

				if ( (src != null) & (dst != null) )
				{
					std::memcpy( OUT dst + BytesU{comp.size} * size_t(dstIndex),
								 src + BytesU{comp.size} * size_t(srcIndex),
								 size_t(comp.size) );
				}
			}

			EntityID	moved;
			srcStorage->Erase( srcIndex, OUT moved );

			// update reference to entity that was moved to new index
			if ( moved )
				_entities.SetArchetype( moved, srcStorage, srcIndex );
				
			_DecreaseStorageSize( srcStorage );
		}
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
