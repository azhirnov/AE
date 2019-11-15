// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "ecs-st/Core/ComponentID.h"

namespace AE::ECS
{

	//
	// Archetype Description
	//

	struct ArchetypeDesc
	{
	// types
		using Ctor_t	= void (*)(void *);

		struct ComponentInfo
		{
			ComponentID			id;
			Bytes<uint16_t>		align;
			Bytes<uint16_t>		size;
			Ctor_t				ctor;

			template <typename T>
			ComponentInfo (InPlaceType<T>);
		};

		using Components_t	= FixedArray< ComponentInfo, ECS_Config::MaxComponents >;
		using Tags_t		= FixedArray< ComponentID, ECS_Config::MaxTagComponents >;

	// variables
		Components_t	components;
		Tags_t			tags;

	// methods
		ArchetypeDesc () {}

		template <typename Comp>	ArchetypeDesc&  Add ();
	};



	//
	// Archetype
	//

	class Archetype final
	{
	// variables
	private:
		HashVal				_hash;
		ArchetypeDesc		_desc;
		Bytes<uint16_t>		_maxAlign;


	// methods
	public:
		explicit Archetype (const ArchetypeDesc &desc, bool sorted = false);

		ND_ bool  operator == (const Archetype &rhs) const;

		ND_ ArchetypeDesc const&	Desc ()		const	{ return _desc; }
		ND_ HashVal					Hash ()		const	{ return _hash; }
		ND_ BytesU					MaxAlign ()	const	{ return BytesU{_maxAlign}; }
		ND_ size_t					Count ()	const	{ return _desc.components.size(); }

		ND_ size_t	IndexOf (ComponentID id) const;
		ND_ bool	HasTag (ComponentID id) const;

		template <typename T>
		ND_ bool	HasComponent () const;

		template <typename T>
		ND_ bool	HasTag () const;

		template <typename T>
		ND_ bool	Has () const;
	};



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
		bool					_locked;
		Archetype const&		_archetype;
		ComponentOffsets_t		_compOffsets;
		const size_t			_capacity;
		Allocator_t				_allocator;


	// methods
	public:
		explicit ArchetypeStorage (const Archetype &archetype, size_t count);
		~ArchetypeStorage ();

			bool  Add (EntityID id, OUT Index_t &index);
			bool  Erase (Index_t index, OUT EntityID &movedEntity);
		ND_ bool  IsValid (EntityID id, Index_t index) const;
			void  Clear ();

			void  Lock ();
			void  Unlock ();

		template <typename T>
		ND_ T*					GetComponents ()				const;
		ND_ void*				GetComponents (ComponentID id)	const;
		ND_ EntityID const*		GetEntities ()					const;	// local index to EntityID
		ND_ size_t				Count ()						const	{ return _count; }
		ND_ bool				Empty ()						const	{ return _count == 0; }
		ND_ Archetype const&	GetArchetype ()					const	{ return _archetype; }

	private:
		ND_ EntityID *		_GetEntities ();
	};
//-----------------------------------------------------------------------------
	

	
	template <typename Comp>
	inline ArchetypeDesc::ComponentInfo::ComponentInfo (InPlaceType<Comp>) :
		id{ ComponentTypeInfo<Comp>::id },
		align{ Bytes<uint16_t>{ AlignOf<Comp> }},
		size{ Bytes<uint16_t>{ SizeOf<Comp> }},
		ctor{ &ComponentTypeInfo<Comp>::Ctor }
	{
		STATIC_ASSERT( not IsEmpty<Comp> );
	}

	template <typename Comp>
	inline ArchetypeDesc&  ArchetypeDesc::Add ()
	{
		if constexpr( IsEmpty<Comp> )
			tags.push_back( ComponentTypeInfo<Comp>::id );
		else
			components.push_back(ComponentInfo{ InPlaceObj<Comp> });

		return *this;
	}
//-----------------------------------------------------------------------------
	

	template <typename T>
	inline bool  Archetype::Has () const
	{
		if constexpr( IsEmpty<T> )
			return HasTag<T>();
		else
			return HasComponent<T>();
	}
	
	template <typename T>
	inline bool  Archetype::HasComponent () const
	{
		STATIC_ASSERT( not IsEmpty<T> );
		return IndexOf( ComponentTypeInfo<T>::id ) < Count();
	}

	template <typename T>
	inline bool  Archetype::HasTag () const
	{
		STATIC_ASSERT( IsEmpty<T> );
		return HasTag( ComponentTypeInfo<T>::id );
	}

//-----------------------------------------------------------------------------


	template <typename T>
	inline T*  ArchetypeStorage::GetComponents () const
	{
		STATIC_ASSERT( not IsEmpty<T> );
		return Cast<T>( GetComponents( ComponentTypeInfo<T>::id ));
	}
	
	inline void*  ArchetypeStorage::GetComponents (ComponentID id) const
	{
		size_t	pos = _archetype.IndexOf( id );
		return	pos < _compOffsets.size() ?
					_memory + _compOffsets[pos] :
					null;
	}

	inline EntityID const*  ArchetypeStorage::GetEntities () const
	{
		return Cast<EntityID>( _memory );
	}
	
	inline EntityID*  ArchetypeStorage::_GetEntities ()
	{
		return Cast<EntityID>( _memory );
	}
	
	inline void  ArchetypeStorage::Lock ()
	{
		CHECK( not _locked );
		_locked = true;
	}

	inline void  ArchetypeStorage::Unlock ()
	{
		CHECK( _locked );
		_locked = false;
	}

}	// AE::ECS

namespace std
{
	template <>
	struct hash< AE::ECS::Archetype >
	{
		ND_ size_t  operator () (const AE::ECS::Archetype &value) const
		{
			return size_t(value.Hash());
		}
	};

}	// std
