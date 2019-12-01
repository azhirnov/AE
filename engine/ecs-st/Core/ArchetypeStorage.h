// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "ecs-st/Core/Archetype.h"
#include "stl/Containers/FixedTupleArray.h"

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
		using Allocator_t	= UntypedAlignedAllocator;
		using Components_t	= FixedTupleArray< ECS_Config::MaxComponentsPerArchetype,
									/*0 - id    */ ComponentID,
									/*1 - size  */ Bytes<uint16_t>,
									/*2 - align */ Bytes<uint16_t>,
									/*3 - ptr   */ void*,
									/*4 - ctor  */ void (*)(void*) >;


	// variables
	private:
		void *				_memory;
		size_t				_count;
		Atomic<int>			_locks;

		const Archetype		_archetype;
		Components_t		_components;
		size_t				_capacity;
		BytesU				_maxAlign;
		Allocator_t			_allocator;


	// methods
	public:
		explicit ArchetypeStorage (const Registry &reg, const Archetype &archetype, size_t capacity);
		~ArchetypeStorage ();

			bool  AddEntities (ArrayView<EntityID> ids, OUT Index_t &startIndex);
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
		ND_ Pair<void*, BytesU>	GetComponent (Index_t idx, ComponentID id) const;

		template <typename T>
		ND_ T*					GetComponents ()				const;
		ND_ void*				GetComponents (ComponentID id)	const;

		ND_ EntityID const*		GetEntities ()					const;	// local index to EntityID
		ND_ size_t				Capacity ()						const	{ return _capacity; }
		ND_ size_t				Count ()						const	{ return _count; }
		ND_ bool				Empty ()						const	{ return _count == 0; }
		ND_ Archetype const&	GetArchetype ()					const	{ return _archetype; }

		ND_ ArrayView<ComponentID>		GetComponentIDs ()		const	{ return _components.get<0>(); }
		ND_ ArrayView<Bytes<uint16_t>>	GetComponentSizes ()	const	{ return _components.get<1>(); }
		ND_ ArrayView<Bytes<uint16_t>>	GetComponentAligns ()	const	{ return _components.get<2>(); }
		ND_ ArrayView<void*>			GetComponentData ()				{ return _components.get<3>(); }


	private:
		ND_ EntityID *	_GetEntities ();

		ND_ size_t		_IndexOf (ComponentID id) const;

		bool _InitComponents (const Registry &reg);
	};


	
/*
=================================================
	GetComponents
=================================================
*/
	template <typename T>
	inline T*  ArchetypeStorage::GetComponents () const
	{
		STATIC_ASSERT( not IsEmpty<T> );
		return Cast<T>( GetComponents( ComponentTypeInfo<T>::id ));
	}
	
/*
=================================================
	GetComponents
=================================================
*/
	inline void*  ArchetypeStorage::GetComponents (ComponentID id) const
	{
		ASSERT( _memory );
		size_t	pos = _IndexOf( id );
		return	pos < _components.size() ?
					_components.at<3>(pos) :
					null;
	}
	
/*
=================================================
	GetComponent
=================================================
*/
	template <typename T>
	inline T*  ArchetypeStorage::GetComponent (Index_t idx) const
	{
		STATIC_ASSERT( not IsEmpty<T> );
		ASSERT( size_t(idx) < Count() );
		ASSERT( _memory );

		size_t	pos = _IndexOf( ComponentTypeInfo<T>::id );
		return	pos < _components.size() ?
					Cast<T>( _components.at<3>(pos) ) + size_t(idx) :
					null;

	}
	
/*
=================================================
	GetComponent
=================================================
*/
	inline Pair<void*, BytesU>  ArchetypeStorage::GetComponent (Index_t idx, ComponentID id) const
	{
		ASSERT( size_t(idx) < Count() );
		ASSERT( _memory );

		size_t	pos = _IndexOf( id );
		return	pos < _components.size() ?
					Pair<void*, BytesU>{ _components.at<3>(pos) + BytesU{_components.at<1>(pos)} * size_t(idx), BytesU{_components.at<1>(pos)} } :
					Pair<void*, BytesU>{ null, 0_b };
	}
	
/*
=================================================
	_IndexOf
=================================================
*/
	inline size_t  ArchetypeStorage::_IndexOf (ComponentID id) const
	{
		return BinarySearch( _components.get<0>(), id );
	}
	
/*
=================================================
	GetEntities
=================================================
*/
	inline EntityID const*  ArchetypeStorage::GetEntities () const
	{
		ASSERT( _memory );
		return Cast<EntityID>( _memory );
	}
	
/*
=================================================
	_GetEntities
=================================================
*/
	inline EntityID*  ArchetypeStorage::_GetEntities ()
	{
		ASSERT( _memory );
		return Cast<EntityID>( _memory );
	}
	
/*
=================================================
	Lock
=================================================
*/
	inline void  ArchetypeStorage::Lock ()
	{
		_locks.fetch_add( 1 );
	}
	
/*
=================================================
	Unlock
=================================================
*/
	inline void  ArchetypeStorage::Unlock ()
	{
		_locks.fetch_sub( 1 );
	}
	
/*
=================================================
	IsLocked
=================================================
*/
	inline bool  ArchetypeStorage::IsLocked () const
	{
		return _locks.load() > 0;
	}

}	// AE::ECS
