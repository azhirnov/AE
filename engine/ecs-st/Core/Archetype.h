// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "ecs-st/Core/ArchetypeBits.h"

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

			ND_ bool	IsTag ()	const	{ return size == 0; }
			ND_ bool	HasData ()	const	{ return size > 0; }
		};

		using Components_t	= FixedArray< ComponentInfo, ECS_Config::MaxComponentsPerArchetype >;


	// variables
		Components_t	components;


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
		ArchetypeDesc		_desc;			// TODO: optimize search by id
		Bytes<uint16_t>		_maxAlign;
		ArchetypeBits		_bits;


	// methods
	public:
		explicit Archetype (const ArchetypeDesc &desc, bool sorted = false);

		ND_ ArchetypeDesc const&	Desc ()			const	{ return _desc; }
		ND_ HashVal					Hash ()			const	{ return _hash; }
		ND_ BytesU					MaxAlign ()		const	{ return BytesU{_maxAlign}; }
		ND_ size_t					Count ()		const	{ return _desc.components.size(); }
		ND_ ArchetypeBits const&	Bits ()			const	{ return _bits; }

		ND_ bool operator == (const Archetype &rhs)	const	{ return Equals( rhs ); }

		ND_ bool	Equals (const Archetype &rhs)	const	{ return _bits.Equals( rhs._bits ); }
		ND_ bool	Contains (const Archetype &rhs)	const	{ return _bits.All( rhs._bits ); }
		ND_ bool	Exists (ComponentID id)			const	{ return _bits.Exists( id ); }

		template <typename T>
		ND_ bool	Exists () const							{ return Exists( ComponentTypeInfo<T>::id ); }

		ND_ size_t	IndexOf (ComponentID id) const;
	};

	

	
	template <typename Comp>
	inline ArchetypeDesc::ComponentInfo::ComponentInfo (InPlaceType<Comp>) :
		id{ ComponentTypeInfo<Comp>::id },
		align{ ComponentTypeInfo<Comp>::align },
		size{ ComponentTypeInfo<Comp>::size },
		ctor{ &ComponentTypeInfo<Comp>::Ctor }
	{
		CHECK( id.value < ECS_Config::MaxComponents );
	}

	template <typename Comp>
	inline ArchetypeDesc&  ArchetypeDesc::Add ()
	{
		components.push_back(ComponentInfo{ InPlaceObj<Comp> });
		return *this;
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
