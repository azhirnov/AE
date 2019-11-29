// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "ecs-st/Core/ArchetypeStorage.h"

namespace AE::ECS
{
	
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

				if ( comp.HasData() )
				{
					void*	data = _memory + _compOffsets[i] + BytesU{comp.size} * _count;

					DEBUG_ONLY( std::memset( OUT data, 0xCD, size_t(comp.size) ));
					comp.ctor( OUT data );
				}
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
				auto& comp = _archetype.Desc().components[i];

				if ( comp.HasData() )
				{
					void*			comp_storage	= _memory + _compOffsets[i];
					const BytesU	comp_size		{comp.size};

					// TODO: call component destructor ???

					std::memcpy( OUT comp_storage + comp_size * idx,
								 comp_storage + comp_size * _count,
								 size_t(comp_size) );
				}
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
			auto& comp = _archetype.Desc().components[i];

			if ( comp.HasData() )
			{
				void*			comp_storage	= _memory + _compOffsets[i];
				const BytesU	comp_size		{comp.size};

				std::memset( OUT comp_storage + comp_size * _count, 0xCD, size_t(comp_size) );
			}
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
			
				if ( comp.HasData() )
				{
					offset			= AlignToLarger( offset, BytesU{comp.align} );
					_compOffsets[i] = offset;
					offset			+= BytesU{comp.size} * _capacity;
				}
				else
				{
					_compOffsets[i]	= BytesU{0};	// TODO: ???
				}
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

				if ( comp.HasData() )
					std::memcpy( OUT _memory + dst, old_mem + src, size_t(comp.size) * _count );
			}
		}

		if ( old_mem )
			_allocator.Deallocate( old_mem, _archetype.MaxAlign() );
	}


}	// AE::ECS
