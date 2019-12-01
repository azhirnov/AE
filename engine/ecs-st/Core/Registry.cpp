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

		_IncreaseStorageSize( storage.get(), 1 );
		
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
	RemoveComponent
=================================================
*/
	bool  Registry::RemoveComponent (EntityID entId, ComponentID compId)
	{
		EXLOCK( _drCheck );

		ArchetypeStorage*	src_storage		= null;
		Index_t				src_index;
		ArchetypeDesc		desc;

		_entities.GetArchetype( entId, OUT src_storage, OUT src_index );
		
		if ( not src_storage )
			return false;

		// get new archetype
		{
			desc = src_storage->GetArchetype().Bits();
			
			if ( not desc.Exists( compId ))
				return false;

			desc.Remove( compId );
		}

		auto	comp_data = src_storage->GetComponent( src_index, compId );

		if ( comp_data.first != null )
			_messages.Add<MsgTag_RemovedComponent>( entId, compId, comp_data );
		else
			_messages.Add<MsgTag_RemovedComponent>( entId, compId );

		// add entity to new archetype
		ArchetypeStorage*	dst_storage		= null;
		Index_t				dst_index;

		_MoveEntity( Archetype{desc}, entId, src_storage, src_index, OUT dst_storage, OUT dst_index );
		return true;
	}
	
/*
=================================================
	RemoveComponents
=================================================
*/
	void  Registry::RemoveComponents (QueryID query, const ArchetypeDesc &removeComps)
	{
		EXLOCK( _drCheck );

		const auto&	q = _queries[ query.Index() ];

		CHECK( not q.locked );
		q.locked = true;

		const auto	remove_comp_ids = removeComps.GetIDs();

		for (auto* arch : q.archetypes)
		{
			ArchetypeDesc		desc		= arch->first.Bits();
			ArchetypeStorage*	src_storage	= arch->second.get();

			desc.Remove( removeComps );
			
			auto					[iter, inserted] = _archetypes.insert({ Archetype{desc}, ArchetypeStoragePtr{} });
			Archetype const&		key				 = iter->first;
			ArchetypeStoragePtr&	dst_storage		 = iter->second;
			
			if ( inserted )
			{
				dst_storage.reset( new ArchetypeStorage{ *this, key, ECS_Config::InitialtStorageSize });

				_OnNewArchetype( &*iter );
			}

			// copy components
			{
				_IncreaseStorageSize( dst_storage.get(), src_storage->Count() );

				auto		comp_ids	= dst_storage->GetComponentIDs();
				auto		comp_sizes	= dst_storage->GetComponentSizes();
				auto		comp_data	= dst_storage->GetComponentData();
				size_t		count		= src_storage->Count();

				Index_t		start;
				CHECK( dst_storage->AddEntities( ArrayView<EntityID>{ src_storage->GetEntities(), count }, OUT start ));

				for (size_t i = 0; i < comp_ids.size(); ++i)
				{
					ComponentID	comp_id		= comp_ids[i];
					size_t		comp_size	= size_t(comp_sizes[i]);
					const auto*	src			= src_storage->GetComponents( comp_id );
					auto*		dst			= comp_data[i];

					if ( (src != null) & (dst != null) )
					{
						dst = dst + BytesU{comp_size * size_t(start)};

						std::memcpy( OUT dst, src, comp_size * count );
					}
				}

				for (size_t i = 0; i < count; ++i)
				{
					_entities.SetArchetype( src_storage->GetEntities()[i], dst_storage.get(), Index_t(size_t(start) + i) );
				}
			}

			// add messages
			{
				auto	comp_ids	= src_storage->GetComponentIDs();
				auto	comp_sizes	= src_storage->GetComponentSizes();
				auto	comp_data	= src_storage->GetComponentData();
				auto*	ent			= src_storage->GetEntities();
				size_t	count		= src_storage->Count();
				
				for (size_t i = 0; i < comp_ids.size(); ++i)
				{
					ComponentID	comp_id		= comp_ids[i];
					size_t		comp_size	= count * size_t(comp_sizes[i]);
				
					if ( removeComps.Exists( comp_id ) and
						 _messages.HasListener<MsgTag_RemovedComponent>( comp_id ))
					{
						if ( comp_size > 0 )
							_messages.AddMulti<MsgTag_RemovedComponent>( comp_id, ArrayView<EntityID>{ ent, count },
																		 ArrayView<uint8_t>{ Cast<uint8_t>(comp_data[i]), comp_size });
						else
							_messages.AddMulti<MsgTag_RemovedComponent>( comp_id, ArrayView<EntityID>{ ent, count });
					}
				}
			}

			src_storage->Clear();
			_DecreaseStorageSize( src_storage );
		}

		q.locked = false;
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
