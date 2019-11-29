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
		}

		// components must be sorted by id
		DEBUG_ONLY(
			for (size_t i = 1; i < _desc.components.size(); ++i) {
				ASSERT( _desc.components[i-1].id < _desc.components[i].id );
			}
		)

		_hash		= HashOf( _desc.components.size() );
		_maxAlign	= Bytes<uint16_t>{ AlignOf<EntityID> };

		for (auto& comp : _desc.components)
		{
			_bits.Add( comp.id );

			_hash << HashOf( comp.id );
			_maxAlign = Max( _maxAlign, comp.align );
			
			if ( not comp.IsTag() )
			{
				CHECK( comp.ctor );
				CHECK( comp.size == AlignToLarger( comp.size, comp.align ));
			}
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
	operator ==
=================================================
*
	bool  Archetype::operator == (const Archetype &rhs) const
	{
		if ( this == &rhs )
			return true;

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
	
/*
=================================================
	Contains
=================================================
*
	bool  Archetype::Contains (const Archetype &rhs) const
	{
		if ( this == &rhs )
			return true;

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

		return true;
	}
	*/

}	// AE::ECS
