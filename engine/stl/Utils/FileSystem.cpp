// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Utils/FileSystem.h"

namespace AE::STL
{
	
/*
=================================================
	FindAndSetCurrent
=================================================
*/
	bool  FileSystem::FindAndSetCurrent (const Path &ref, uint depth)
	{
		return FindAndSetCurrent( CurrentPath(), ref, depth );
	}

	bool  FileSystem::FindAndSetCurrent (const Path &base, const Path &ref, uint depth)
	{
		Path	dir;

		if ( Search( base, ref, depth, depth, OUT dir ))
			return SetCurrentPath( dir );
		
		return false;
	}
	
/*
=================================================
	SearchBackward
=================================================
*/
	bool  FileSystem::SearchBackward (const Path &ref, uint depth, OUT Path &result)
	{
		return SearchBackward( CurrentPath(), ref, depth, OUT result );
	}
	
	bool  FileSystem::SearchBackward (const Path &base, const Path &ref, uint depth, OUT Path &result)
	{
		CHECK_ERR( Exists( base ));

		Path	curr = ToAbsolute( base );

		for (; not curr.empty(); --depth)
		{
			result = (Path{ curr } /= ref);

			if ( Exists( result ))
				return true;
			
			if ( depth == 0 )
				break;

			curr = curr.parent_path();
		}

		result.clear();
		return false;
	}

/*
=================================================
	RecursiveSearchForward
=================================================
*/
namespace {
	static bool  RecursiveSearchForward (const Path &curr, const Path &ref, uint depth, OUT Path &result)
	{
		for (auto& dir : FileSystem::Enum( curr ))
		{
			if ( not dir.is_directory() )
				continue;
			
			result = (Path{ dir.path() } /= ref);

			if ( FileSystem::Exists( result ))
				return true;

			if ( depth > 0 )
			{
				if ( RecursiveSearchForward( dir.path(), ref, depth-1, OUT result ))
					return true;
			}
		}

		result.clear();
		return false;
	}
}	// namespace

/*
=================================================
	SearchForward
=================================================
*/
	bool  FileSystem::SearchForward (const Path &ref, uint depth, OUT Path &result)
	{
		return SearchForward( CurrentPath(), ref, depth, OUT result );
	}
	
	bool  FileSystem::SearchForward (const Path &base, const Path &ref, uint depth, OUT Path &result)
	{
		CHECK_ERR( Exists( base ));

		const Path	curr = ToAbsolute( base );

		result = (Path{ curr } /= ref);

		if ( FileSystem::Exists( result ))
			return true;

		return RecursiveSearchForward( curr, ref, depth, OUT result );
	}

/*
=================================================
	Search
=================================================
*/
	bool  FileSystem::Search (const Path &ref, uint backwardDepth, uint forwardDepth, OUT Path &result)
	{
		return Search( CurrentPath(), ref, backwardDepth, forwardDepth, OUT result );
	}

	bool  FileSystem::Search (const Path &base, const Path &ref, const uint backwardDepth, const uint forwardDepth, OUT Path &result)
	{
		CHECK_ERR( Exists( base ));

		Path	curr	= ToAbsolute( base );
		uint	depth	= backwardDepth;

		for (; not curr.empty(); --depth)
		{
			result = (Path{ curr } /= ref);

			if ( Exists( result ))
				return true;
			
			if ( depth == 0 )
				break;
			
			if ( SearchForward( curr, ref, forwardDepth, OUT result ))
				return true;

			curr = curr.parent_path();
		}

		result.clear();
		return false;
	}


}	// AE::STL
