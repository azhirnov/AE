// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "ecs-st/Core/Registry.h"

namespace AE::ECS
{

/*
=================================================
	constructor
=================================================
*/
	Registry::Registry () :
		_componentInfo{ new ComponentMap_t::element_type{} }
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
	bool  Registry::_RemoveEntity (EntityID entId)
	{
		ArchetypeStorage*	storage = null;
		Index_t				index;

		if ( _entities.GetArchetype( entId, OUT storage, OUT index ) and storage )
		{
			ASSERT( storage->IsValid( entId, index ));

			// add messages
			{
				auto	comp_ids	= storage->GetComponentIDs();
				auto	comp_sizes	= storage->GetComponentSizes();
				auto	comp_aligns	= storage->GetComponentAligns();
				auto	comp_data	= storage->GetComponentData();

				for (size_t i = 0; i < comp_ids.size(); ++i)
				{
					size_t	comp_size = size_t(comp_sizes[i]);

					if ( comp_size > 0 )
					{
						uint8_t*	comp_ptr = Cast<uint8_t>( comp_data[i] + BytesU{comp_size} * size_t(index) );
						_messages.Add<MsgTag_RemovedComponent>( entId, comp_ids[i], ArrayView<uint8_t>{ comp_ptr, comp_size });
					}
					else
						_messages.Add<MsgTag_RemovedComponent>( entId, comp_ids[i] );
				}
			}

			EntityID	moved;
			if ( not storage->Erase( index, OUT moved ))
				return false;
			
			// update reference to entity that was moved to new index
			if ( moved )
				_entities.SetArchetype( moved, storage, index );

			_entities.SetArchetype( entId, null, Index_t(-1) );
			
			_DecreaseStorageSize( storage );
		}
		return true;
	}

/*
=================================================
	_AddEntity
=================================================
*/
	void  Registry::_AddEntity (const Archetype &arch, EntityID entId, OUT ArchetypeStorage* &outStorage, OUT Index_t &index)
	{
		auto					[iter, inserted] = _archetypes.insert({ arch, ArchetypeStoragePtr{} });
		Archetype const&		key				 = iter->first;
		ArchetypeStoragePtr&	storage			 = iter->second;

		if ( inserted )
		{
			storage.reset( new ArchetypeStorage{ *this, key, ECS_Config::InitialtStorageSize });

			_OnNewArchetype( &*iter );
		}

		if ( storage->Add( entId, OUT index ))
		{
			outStorage = storage.get();
			_entities.SetArchetype( entId, storage.get(), index );
			return;
		}

		storage->Reserve( storage->Capacity()*2 );
		
		CHECK( storage->Add( entId, OUT index ));
		_entities.SetArchetype( entId, storage.get(), index );
		
		outStorage = storage.get();
	}
	
	void  Registry::_AddEntity (const Archetype &arch, EntityID entId)
	{
		ArchetypeStorage*	storage = null;
		Index_t				index;
		return _AddEntity( arch, entId, OUT storage, OUT index );
	}
	
/*
=================================================
	_MoveEntity
=================================================
*/
	void  Registry::_MoveEntity (const Archetype &arch, EntityID entId, ArchetypeStorage* srcStorage, Index_t srcIndex,
								 OUT ArchetypeStorage* &dstStorage, OUT Index_t &dstIndex)
	{
		_AddEntity( arch, entId, OUT dstStorage, OUT dstIndex );

		if ( srcStorage )
		{
			auto	comp_ids = dstStorage->GetComponentIDs();

			// copy components
			for (auto& comp_id : comp_ids)
			{
				auto	src = srcStorage->GetComponent( srcIndex, comp_id );
				auto	dst = dstStorage->GetComponent( dstIndex, comp_id );

				if ( (src.first != null) & (dst.first != null) )
				{
					ASSERT( src.second == dst.second );
					std::memcpy( OUT dst.first, src.first, size_t(src.second) );
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
	
/*
=================================================
	CreateQuery
=================================================
*/
	QueryID  Registry::CreateQuery (const ArchetypeQueryDesc &desc)
	{
		EXLOCK( _drCheck );

		for (size_t i = 0; i < _queries.size(); ++i)
		{
			if ( desc == _queries[i].desc )
				return QueryID{ CheckCast<uint16_t>(i), 0 };
		}

		auto&	q = _queries.emplace_back();
		q.desc	= desc;

		for (auto& arch : _archetypes)
		{
			if ( desc.Compatible( arch.first.Bits() ))
				q.archetypes.push_back( &arch );
		}

		q.locked = false;
		return QueryID{ CheckCast<uint16_t>(_queries.size()-1), 0 };
	}
	
/*
=================================================
	_OnNewArchetype
=================================================
*/
	void  Registry::_OnNewArchetype (ArchetypePair_t *arch)
	{
		for (size_t i = 0; i < _queries.size(); ++i)
		{
			auto&	q = _queries[i];

			if ( q.desc.Compatible( arch->first.Bits() ))
			{
				CHECK( not q.locked );

				q.archetypes.push_back( arch );
			}
		}
	}


}	// AE::ECS
