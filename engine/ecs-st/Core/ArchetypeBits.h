// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "ecs-st/Core/ComponentID.h"

namespace AE::ECS
{

	//
	// Archetype Bits
	//

	class ArchetypeBits
	{
	// types
	private:
		using					Chunk_t			= uint64_t;
		static constexpr uint	BitsPerChunk	= sizeof(Chunk_t) * 8;
		static constexpr uint	ChunkCount		= ECS_Config::MaxComponents / BitsPerChunk;
		using					CompBits_t		= StaticArray< Chunk_t, ChunkCount >;


	// variables
	private:
		CompBits_t		_bits	= {};


	// methods
	public:
		ArchetypeBits () {}

		template <typename Comp>	ArchetypeBits&  Add ()				{ return Add( ComponentTypeInfo<Comp>::id ); }
		template <typename Comp>	ArchetypeBits&  Remove ()			{ return Remove( ComponentTypeInfo<Comp>::id ); }
		template <typename Comp>	ND_ bool		Exists ()	const	{ return Exists( ComponentTypeInfo<Comp>::id ); }

		ArchetypeBits&  Add (ComponentID id);
		ArchetypeBits&  Remove (ComponentID id);

		ND_ bool		Exists (ComponentID id) const;
		ND_ bool		All (const ArchetypeBits &) const;
		ND_ bool		Any (const ArchetypeBits &) const;
		ND_ bool		AnyOrEmpty (const ArchetypeBits &) const;
		ND_ bool		Equals (const ArchetypeBits &) const;
		ND_ bool		Empty () const;
	};
	

	
	//
	// Archetype Query description
	//

	struct ArchetypeQueryDesc
	{
		ArchetypeBits		required;
		ArchetypeBits		subtractive;
		ArchetypeBits		requireAny;

		ArchetypeQueryDesc () {}

		ND_ bool  Compatible (const ArchetypeBits &) const;

		ND_ bool  operator == (const ArchetypeQueryDesc &rhs) const;
	};
//-----------------------------------------------------------------------------


	
/*
=================================================
	Add
=================================================
*/
	inline ArchetypeBits&  ArchetypeBits::Add (ComponentID id)
	{
		ASSERT( id.value < ECS_Config::MaxComponents );
		_bits[id.value / BitsPerChunk] |= (Chunk_t(1) << (id.value % BitsPerChunk));
		return *this;
	}
	
/*
=================================================
	Remove
=================================================
*/
	inline ArchetypeBits&  ArchetypeBits::Remove (ComponentID id)
	{
		ASSERT( id.value < ECS_Config::MaxComponents );
		_bits[id.value / BitsPerChunk] &= ~(Chunk_t(1) << (id.value % BitsPerChunk));
		return *this;
	}
		
/*
=================================================
	Exists
=================================================
*/
	inline bool  ArchetypeBits::Exists (ComponentID id) const
	{
		ASSERT( id.value < ECS_Config::MaxComponents );
		return _bits[id.value / BitsPerChunk] & (Chunk_t(1) << (id.value % BitsPerChunk));
	}
	
/*
=================================================
	All
=================================================
*/
	inline bool  ArchetypeBits::All (const ArchetypeBits &rhs) const
	{
		bool	result = true;
		for (size_t i = 0; i < _bits.size(); ++i) {
			result &= ((_bits[i] & rhs._bits[i]) == rhs._bits[i]);
		}
		return result;
	}
	
/*
=================================================
	Any
=================================================
*/
	inline bool  ArchetypeBits::Any (const ArchetypeBits &rhs) const
	{
		bool	result	= false;
		for (size_t i = 0; i < _bits.size(); ++i) {
			result |= !!(_bits[i] & rhs._bits[i]);
		}
		return result;
	}
	
/*
=================================================
	AnyOrEmpty
=================================================
*/
	inline bool  ArchetypeBits::AnyOrEmpty (const ArchetypeBits &rhs) const
	{
		bool	result	= false;
		bool	empty	= true;

		for (size_t i = 0; i < _bits.size(); ++i)
		{
			result |= !!(_bits[i] & rhs._bits[i]);
			empty  &= !_bits[i];
		}
		return result | empty;
	}
	
/*
=================================================
	Equals
=================================================
*/
	inline bool  ArchetypeBits::Equals (const ArchetypeBits &rhs) const
	{
		bool	result = true;
		for (size_t i = 0; i < _bits.size(); ++i) {
			result &= (_bits[i] == rhs._bits[i]);
		}
		return result;
	}
	
/*
=================================================
	Empty
=================================================
*/
	inline bool  ArchetypeBits::Empty () const
	{
		bool	result = true;
		for (size_t i = 0; i < _bits.size(); ++i) {
			result  &= !_bits[i];
		}
		return result;
	}
//-----------------------------------------------------------------------------
	

	
/*
=================================================
	Compatible
=================================================
*/
	inline bool  ArchetypeQueryDesc::Compatible (const ArchetypeBits &bits) const
	{
		return	bits.All( required )			&
				(not subtractive.Any( bits ))	&
				requireAny.AnyOrEmpty( bits );
	}
	
/*
=================================================
	operator ==
=================================================
*/
	inline bool  ArchetypeQueryDesc::operator == (const ArchetypeQueryDesc &rhs) const
	{
		return	required.Equals( rhs.required )			&
				subtractive.Equals( rhs.subtractive )	&
				requireAny.Equals( rhs.requireAny );
	}

}	// AE::ECS
