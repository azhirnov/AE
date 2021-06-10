// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "ecs-st/Core/ArchetypeStorage.h"
#include "ecs-st/Core/Registry.h"

namespace AE::ECS
{

/*
=================================================
	constructor
=================================================
*/
	ArchetypeStorage::ArchetypeStorage (const Registry &reg, const Archetype &archetype, usize capacity) :
		_memory{ null },
		_count{ 0 },
		_locks{ 0 },
		_archetype{ archetype },
		_capacity{ 0 },
		_owner{ reg }
	{
		CHECK( _InitComponents() );
		Reserve( capacity );
	}
	
/*
=================================================
	_InitComponents
=================================================
*/
	bool  ArchetypeStorage::_InitComponents ()
	{
		_maxAlign = Bytes{AE_CACHE_LINE};

		auto&	desc = _archetype.Desc().Raw();

		for (usize i = 0; i < desc.size(); ++i)
		{
			auto	u = desc[i];
			int		j = BitScanForward( u );

			for (; j >= 0; j = BitScanForward( u ))
			{
				u &= ~(1ull << j);

				ComponentID	id{ CheckCast<uint16_t>( j + i*sizeof(u)*8 )};

				auto	info = _owner.GetComponentInfo( id );
				CHECK_ERR( info );

				usize	idx = _components.size();
				_components.emplace_back();

				_components.at<0>( idx ) = id;
				_components.at<1>( idx ) = info->size;
				_components.at<2>( idx ) = info->align;
				_components.at<3>( idx ) = null;
				_components.at<4>( idx ) = info->ctor;

				_maxAlign = Max( _maxAlign, Bytes{ info->align });

				DEBUG_ONLY( _dbgView.emplace_back() );
			}
		}

		return true;
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
		_allocator.Deallocate( _memory, _maxAlign );
	}
	
/*
=================================================
	Add
----
	add entity ID and initialize components
=================================================
*/
	bool  ArchetypeStorage::Add (EntityID id, OUT Index_t &index)
	{
		CHECK_ERR( not IsLocked() );

		if ( _count < _capacity )
		{
			_GetEntities()[_count] = id;

			for (usize i = 0; i < _components.size(); ++i)
			{
				auto	comp_size	= _components.at<1>(i);
				auto	comp_data	= _components.at<3>(i);
				auto	comp_ctor	= _components.at<4>(i);

				if ( comp_size > 0 )
				{
					void*	data = comp_data + Bytes{comp_size} * _count;

					DEBUG_ONLY( std::memset( OUT data, 0xCD, usize(comp_size) ));
					comp_ctor( OUT data );
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
	AddEntities
----
	add entity IDs without initializing components
=================================================
*/
	bool  ArchetypeStorage::AddEntities (ArrayView<EntityID> ids, OUT Index_t &startIndex)
	{
		CHECK_ERR( not IsLocked() );

		if ( _count + ids.size() < _capacity )
		{
			MemCopy( OUT _GetEntities() + _count, ids.data(), ArraySizeOf(ids) );
			startIndex = Index_t(_count);

			_count += ids.size();
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

		const usize	idx = usize(index);
		CHECK_ERR( idx < _count );
		
		--_count;

		if ( idx != _count )
		{
			for (usize i = 0; i < _components.size(); ++i)
			{
				const Bytes	comp_size{ _components.at<1>(i) };

				if ( comp_size > 0 )
				{
					void*	comp_storage = _components.at<3>(i);
					MemCopy( OUT comp_storage + comp_size * idx,
							 comp_storage + comp_size * _count,
							 comp_size );
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
		for (usize i = 0; i < _components.size(); ++i)
		{
			const usize	comp_size = usize(_components.at<1>(i));

			if ( comp_size > 0 )
			{
				void*	comp_storage = _components.at<3>(i);

				std::memset( OUT comp_storage + Bytes{comp_size} * _count, 0xCD, comp_size );
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
		return	usize(index) < _count and
				GetEntities()[ usize(index) ] == id;
	}
	
/*
=================================================
	Clear
=================================================
*/
	void  ArchetypeStorage::Clear ()
	{
		CHECK_ERRV( not IsLocked() );

		_count = 0;
	}
	
/*
=================================================
	Reserve
=================================================
*/
	void  ArchetypeStorage::Reserve (usize size)
	{
		using ComponentData_t	= FixedArray< void*, ECS_Config::MaxComponentsPerArchetype >;

		CHECK_ERRV( not IsLocked() );
		CHECK_ERRV( size >= _count );

		if ( size == _capacity )
			return;

		_capacity = size;

		const ComponentData_t	old_data	{ _components.get<3>() };
		void *					old_mem		= _memory;

		// allocate
		{
			Bytes	offset	= SizeOf<EntityID> * _capacity;

			for (usize i = 0; i < _components.size(); ++i)
			{
				auto	comp_size	= _components.at<1>(i);
				auto	comp_align	= _components.at<2>(i);
				auto&	comp_data	= _components.at<3>(i);
			
				if ( comp_size > 0 )
				{
					offset		= AlignToLarger( offset, Bytes{comp_align} );
					comp_data	= (void*)(offset);
					offset		+= Bytes{comp_size} * _capacity;
				}
				else
				{
					comp_data	= null;	// TODO: ???
				}
			}

			_memory = _allocator.Allocate( offset, _maxAlign );

			if ( not _memory )
			{
				CHECK( !"failed to allocate memory" );
				_count		= 0;
				_capacity	= 0;
				_allocator.Deallocate( old_mem, _maxAlign );
				return;
			}
			
			for (usize i = 0; i < _components.size(); ++i)
			{
				auto&	comp_data = _components.at<3>(i);
				comp_data = comp_data ? BitCast<void*>( usize(comp_data) + usize(_memory) ) : null;

				DEBUG_ONLY(
				if ( comp_data ) {
					_dbgView[i] = _owner.GetComponentInfo( _components.at<0>(i) )->dbgView( comp_data, _capacity );
				})
			}

			DEBUG_ONLY( std::memset( OUT _memory, 0xCD, usize(offset) ));
			DEBUG_ONLY( _memoryEnd = _memory + offset; )
		}

		// copy
		if ( _count and old_mem )
		{
			MemCopy( OUT _GetEntities(), old_mem, SizeOf<EntityID> * _count );

			for (usize i = 0; i < _components.size(); ++i)
			{
				auto&	src			= old_data[i];
				auto&	dst			= _components.at<3>(i);
				auto	comp_size	= Bytes{_components.at<1>(i)};

				if ( comp_size > 0 )
					MemCopy( OUT dst, src, comp_size * _count );
			}
		}

		if ( old_mem )
			_allocator.Deallocate( old_mem, _maxAlign );
	}


}	// AE::ECS
