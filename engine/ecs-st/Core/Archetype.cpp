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
		ASSERT( _desc.components.size() or _desc.tags.size() );

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
	bool  Archetype::HasTag (TagComponentID id) const
	{
		return BinarySearch( ArrayView<TagComponentID>{ _desc.tags }, id ) < _desc.tags.size();
	}

/*
=================================================
	operator ==
=================================================
*/
	bool  Archetype::operator == (const Archetype &rhs) const
	{
		if ( this == &rhs )
			return true;

		if ( _hash != rhs._hash )
			return false;

		if ( _desc.components.size() != rhs._desc.components.size() )
			return false;
		
		if ( _desc.tags.size() != rhs._desc.tags.size() )
			return false;

		for (size_t i = 0; i < _desc.components.size(); ++i)
		{
			if ( not (_desc.components[i].id == rhs._desc.components[i].id) )
				return false;
		}

		for (size_t i = 0; i < _desc.tags.size(); ++i)
		{
			if ( not (_desc.tags[i] == rhs._desc.tags[i]) )
				return false;
		}
		return true;
	}
	
/*
=================================================
	Contains
=================================================
*/
	bool  Archetype::Contains (const Archetype &rhs) const
	{
		if ( this == &rhs )
			return true;

		// components
		{
			auto		li = this->_desc.components.begin();
			const auto	le = this->_desc.components.end();
			auto		ri = rhs._desc.components.begin();
			const auto	re = rhs._desc.components.end();

			for (; (li != le) & (ri != re);)
			{
				if ( li->id < ri->id )
					++li;
				else
				if ( li->id > ri->id )
					break;
				else
				if ( li->id == ri->id )
				{
					++li;
					++ri;
				}
			}

			if ( ri != re )
				return false;
		}

		// tags
		{
			auto		li = this->_desc.tags.begin();
			const auto	le = this->_desc.tags.end();
			auto		ri = rhs._desc.tags.begin();
			const auto	re = rhs._desc.tags.end();

			for (; (li != le) & (ri != re);)
			{
				if ( *li < *ri )
					++li;
				else
				if ( *li > *ri )
					break;
				else
				if ( *li == *ri )
				{
					++li;
					++ri;
				}
			}

			if ( ri != re )
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
	ArchetypeStorage::ArchetypeStorage (const Archetype &archetype, size_t capacity) :
		_memory{ null },
		_count{ 0 },
		_locks{ 0 },
		_archetype{ archetype },
		_capacity{ 0 }
	{
		_compOffsets.resize( _archetype.Count() );
		Reserve( capacity );
	}
	
/*
=================================================
	destructor
=================================================
*/
	ArchetypeStorage::~ArchetypeStorage ()
	{
		CHECK( not IsLocked() );

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
		CHECK_ERR( not IsLocked() );

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
		CHECK_ERR( not IsLocked() );

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

				std::memcpy( OUT comp_storage + comp_size * idx,
							 comp_storage + comp_size * _count,
							 size_t(comp_size) );
			}

			auto*	ent = _GetEntities();
			movedEntity = ent[idx] = ent[_count];
		}
		else
		{
			ASSERT( not movedEntity.IsValid() );
		}
		
		DEBUG_ONLY(
		for (size_t i = 0; i < _compOffsets.size(); ++i)
		{
			auto&			comp			= _archetype.Desc().components[i];
			void*			comp_storage	= _memory + _compOffsets[i];
			const BytesU	comp_size		{comp.size};

			std::memset( OUT comp_storage + comp_size * _count, 0xCD, size_t(comp_size) );
		})

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
		CHECK_ERR( not IsLocked(), void() );

		_count = 0;
	}
	
/*
=================================================
	Reserve
=================================================
*/
	void  ArchetypeStorage::Reserve (size_t size)
	{
		CHECK_ERR( not IsLocked(), void() );

		if ( size == _capacity or size < _count )
			return;

		_capacity = size;

		const ComponentOffsets_t	old_offsets	= _compOffsets;
		void *						old_mem		= _memory;

		// allocate
		{
			BytesU	offset	= SizeOf<EntityID> * _capacity;

			for (size_t i = 0; i < _compOffsets.size(); ++i)
			{
				auto&	comp = _archetype.Desc().components[i];
			
				offset			= AlignToLarger( offset, BytesU{comp.align} );
				_compOffsets[i] = offset;
				offset			+= BytesU{comp.size} * _capacity;
			}

			_memory = _allocator.Allocate( offset, _archetype.MaxAlign() );

			if ( not _memory )
			{
				CHECK( !"failed to allocate memory" );
				_count		= 0;
				_capacity	= 0;
				_allocator.Deallocate( old_mem, _archetype.MaxAlign() );
				return;
			}

			DEBUG_ONLY( std::memset( OUT _memory, 0xCD, size_t(offset) ));
		}

		// copy
		if ( _count and old_mem )
		{
			std::memcpy( OUT _memory, old_mem, size_t(SizeOf<EntityID> * _count) );

			for (size_t i = 0; i < _compOffsets.size(); ++i)
			{
				auto&	src		= old_offsets[i];
				auto&	dst		= _compOffsets[i];
				auto&	comp	= _archetype.Desc().components[i];

				std::memcpy( OUT _memory + dst, old_mem + src, size_t(comp.size) * _count );
			}
		}

		if ( old_mem )
			_allocator.Deallocate( old_mem, _archetype.MaxAlign() );
	}

}	// AE::ECS
