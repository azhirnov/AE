// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "ecs-st/Core/Archetype.h"

namespace AE::ECS
{
	
/*
=================================================
	constructor
=================================================
*/
	Archetype::Archetype (const ArchetypeDesc &desc, bool sorted) :
		_desc{ desc }
	{
		ASSERT( _desc.components.size() );

		if ( not sorted )
		{
			std::stable_sort( _desc.components.begin(), _desc.components.end(),
							  [] (auto& lhs, auto& rhs) { return lhs.id < rhs.id; });

			std::stable_sort( _desc.tags.begin(), _desc.tags.end() );
		}

		// components must be sorted by id
		DEBUG_ONLY(
			for (size_t i = 1; i < _desc.components.size(); ++i) {
				ASSERT( _desc.components[i-1].id < _desc.components[i].id );
			}
			for (size_t i = 1; i < _desc.tags.size(); ++i) {
				ASSERT( _desc.tags[i-1] < _desc.tags[i] );
			}
		)

		_hash		= HashOf( _desc.components.size() );
		_maxAlign	= Bytes<uint16_t>{ AlignOf<EntityID> };

		for (auto& comp : _desc.components)
		{
			_hash << HashOf( comp.id );
			_maxAlign = Max( _maxAlign, comp.align );
			
			CHECK( comp.ctor );
			CHECK( comp.size == AlignToLarger( comp.size, comp.align ));
		}

		for (auto& tag : _desc.tags)
		{
			_hash << HashOf( tag );
		}
	}
	
/*
=================================================
	IndexOf
=================================================
*/
	size_t  Archetype::IndexOf (ComponentID id) const
	{
		using ComponentInfo = ArchetypeDesc::ComponentInfo;

		struct Key {
			ComponentID id;

			Key (ComponentID id) : id{id} {}
			bool operator >  (const ComponentInfo &rhs) const	{ return id >  rhs.id; }
			bool operator == (const ComponentInfo &rhs) const	{ return id == rhs.id; }
		};

		return BinarySearch( ArrayView<ComponentInfo>{ _desc.components }, Key{id} );
	}
	
/*
=================================================
	HasTag
=================================================
*/
	bool  Archetype::HasTag (ComponentID id) const
	{
		return BinarySearch( ArrayView<ComponentID>{ _desc.tags }, id ) < _desc.tags.size();
	}

/*
=================================================
	operator ==
=================================================
*/
	bool  Archetype::operator == (const Archetype &rhs) const
	{
		if ( _hash != rhs._hash )
			return false;

		if ( _desc.components.size() != rhs._desc.components.size() )
			return false;

		for (size_t i = 0; i < _desc.components.size(); ++i)
		{
			if ( not (_desc.components[i].id == rhs._desc.components[i].id) )
				return false;
		}
		return true;
	}
//-----------------------------------------------------------------------------



/*
=================================================
	constructor
=================================================
*/
	ArchetypeStorage::ArchetypeStorage (const Archetype &archetype, size_t count) :
		_count{ 0 },
		_locked{ false },
		_archetype{ archetype },
		_capacity{ count }
	{
		_compOffsets.resize( _archetype.Count() );

		BytesU	offset	= SizeOf<EntityID> * _capacity;

		for (size_t i = 0; i < _compOffsets.size(); ++i)
		{
			auto&	comp = _archetype.Desc().components[i];
			
			offset			= AlignToLarger( offset, BytesU{comp.align} );
			_compOffsets[i] = offset;
			offset			+= BytesU{comp.size} * _capacity;
		}

		_memory = _allocator.Allocate( offset, _archetype.MaxAlign() );
	}
	
/*
=================================================
	destructor
=================================================
*/
	ArchetypeStorage::~ArchetypeStorage ()
	{
		ASSERT( _count == 0 );
		_allocator.Deallocate( _memory, _archetype.MaxAlign() );
	}
	
/*
=================================================
	Add
=================================================
*/
	bool  ArchetypeStorage::Add (EntityID id, OUT Index_t &index)
	{
		CHECK_ERR( not _locked );

		if ( _count < _capacity )
		{
			_GetEntities()[_count] = id;

			for (size_t i = 0; i < _compOffsets.size(); ++i)
			{
				auto&	comp = _archetype.Desc().components[i];
				void*	data = _memory + _compOffsets[i] + BytesU{comp.size} * _count;

				DEBUG_ONLY( std::memset( OUT data, 0xCD, size_t(comp.size) ));
				comp.ctor( OUT data );
			}
			
			index = Index_t(_count);
			++_count;

			return true;
		}
		return false;
	}
	
/*
=================================================
	Erase
=================================================
*/
	bool  ArchetypeStorage::Erase (Index_t index, OUT EntityID &movedEntity)
	{
		CHECK_ERR( not _locked );

		const size_t	idx = size_t(index);
		CHECK_ERR( idx < _count );
		
		--_count;

		if ( idx != _count )
		{
			for (size_t i = 0; i < _compOffsets.size(); ++i)
			{
				auto&			comp			= _archetype.Desc().components[i];
				void*			comp_storage	= _memory + _compOffsets[i];
				const BytesU	comp_size		{comp.size};

				// TODO: call component destructor ???

				std::memcpy( comp_storage + comp_size * idx, comp_storage + comp_size * _count, size_t(comp_size) );
			}

			auto*	ent = _GetEntities();
			movedEntity = ent[idx] = ent[_count];
		}
		else
		{
			ASSERT( not movedEntity.IsValid() );
		}
		return true;
	}
	
/*
=================================================
	IsValid
=================================================
*/
	bool  ArchetypeStorage::IsValid (EntityID id, Index_t index) const
	{
		return	size_t(index) < _count and
				GetEntities()[ size_t(index) ] == id;
	}
	
/*
=================================================
	Clear
=================================================
*/
	void  ArchetypeStorage::Clear ()
	{
		CHECK_ERR( not _locked, void() );

		_count = 0;
	}


}	// AE::ECS
