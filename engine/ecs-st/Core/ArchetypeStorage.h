// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "ecs-st/Core/Archetype.h"

namespace AE::ECS
{
	
	//
	// Archetype Storage
	//

	class ArchetypeStorage final
	{
	// types
	public:
		enum class Index_t : uint {};
		
	private:
		using Allocator_t			= UntypedAlignedAllocator;
		using ComponentOffsets_t	= FixedArray< BytesU, ECS_Config::MaxComponents >;


	// variables
	private:
		void *					_memory;
		size_t					_count;
		Atomic<int>				_locks;

		Archetype const&		_archetype;
		ComponentOffsets_t		_compOffsets;
		size_t					_capacity;
		Allocator_t				_allocator;


	// methods
	public:
		explicit ArchetypeStorage (const Archetype &archetype, size_t capacity);
		~ArchetypeStorage ();

			bool  Add (EntityID id, OUT Index_t &index);
			bool  Erase (Index_t index, OUT EntityID &movedEntity);
		ND_ bool  IsValid (EntityID id, Index_t index) const;
			void  Clear ();
			void  Reserve (size_t size);

			void  Lock ();
			void  Unlock ();
		ND_ bool  IsLocked () const;
		
		template <typename T>
		ND_ T*					GetComponent (Index_t idx)		const;
		template <typename T>
		ND_ T*					GetComponents ()				const;
		ND_ void*				GetComponents (ComponentID id)	const;
		ND_ EntityID const*		GetEntities ()					const;	// local index to EntityID
		ND_ size_t				Capacity ()						const	{ return _capacity; }
		ND_ size_t				Count ()						const	{ return _count; }
		ND_ bool				Empty ()						const	{ return _count == 0; }
		ND_ Archetype const&	GetArchetype ()					const	{ return _archetype; }

	private:
		ND_ EntityID *		_GetEntities ();
	};


	
	template <typename T>
	inline T*  ArchetypeStorage::GetComponents () const
	{
		STATIC_ASSERT( not IsEmpty<T> );
		return Cast<T>( GetComponents( ComponentTypeInfo<T>::id ));
	}
	
	inline void*  ArchetypeStorage::GetComponents (ComponentID id) const
	{
		ASSERT( _memory );
		size_t	pos = _archetype.IndexOf( id );
		return	pos < _compOffsets.size() ?
					_memory + _compOffsets[pos] :
					null;
	}
	
	template <typename T>
	inline T*  ArchetypeStorage::GetComponent (Index_t idx) const
	{
		STATIC_ASSERT( not IsEmpty<T> );
		ASSERT( size_t(idx) < Count() );
		ASSERT( _memory );

		size_t	pos = _archetype.IndexOf( ComponentTypeInfo<T>::id );
		return	pos < _compOffsets.size() ?
					Cast<T>( _memory + _compOffsets[pos] ) + size_t(idx) :
					null;

	}

	inline EntityID const*  ArchetypeStorage::GetEntities () const
	{
		ASSERT( _memory );
		return Cast<EntityID>( _memory );
	}
	
	inline EntityID*  ArchetypeStorage::_GetEntities ()
	{
		ASSERT( _memory );
		return Cast<EntityID>( _memory );
	}
	
	inline void  ArchetypeStorage::Lock ()
	{
		_locks.fetch_add( 1 );
	}

	inline void  ArchetypeStorage::Unlock ()
	{
		_locks.fetch_sub( 1 );
	}
	
	inline bool  ArchetypeStorage::IsLocked () const
	{
		return _locks.load() > 0;
	}

}	// AE::ECS
