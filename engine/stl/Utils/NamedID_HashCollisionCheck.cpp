// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Utils/NamedID_HashCollisionCheck.h"

#include <random>

namespace AE::STL
{
	
/*
=================================================
	destructor
=================================================
*/
	NamedID_HashCollisionCheck::~NamedID_HashCollisionCheck ()
	{
		for (auto& id : _idMap)
		{
			if ( id.second.hasCollisions )
			{
				uint	seed = _RecalculateSeed( id.second );
				CHECK( seed );
			}
		}
	}
	
/*
=================================================
	_RecalculateSeed
=================================================
*/
	uint  NamedID_HashCollisionCheck::_RecalculateSeed (Info &info) const
	{
		std::mt19937	gen{ std::random_device{}() };
		for (;;)
		{
			// generate new seed
			uint	new_seed		= gen();
			bool	has_collision	= false;

			HashMap< usize, StString_t >	map;
			for (auto& item : info.data)
			{
				usize	key = usize(CT_Hash( item.second.data(), item.second.length(), new_seed ));

				if ( not map.insert({ key, StringView{item.second} }).second )
				{
					has_collision = true;
					break;
				}
			}

			if ( not has_collision )
			{
				info.seed = new_seed;
				return new_seed;
			}
		}
	}
	
/*
=================================================
	HasCollisions
=================================================
*/
	bool  NamedID_HashCollisionCheck::HasCollisions () const
	{
		for (auto& item : _idMap)
		{
			if ( item.second.hasCollisions )
				return true;
		}
		return false;
	}


}	// AE::STL
