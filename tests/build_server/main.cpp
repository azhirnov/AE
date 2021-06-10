// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Utils/FileSystem.h"
using namespace AE::STL;

#ifdef PLATFORM_WINDOWS
extern void  WinTest ();
#endif

int main ()
{
	{
		Path	path;
		CHECK_FATAL( FileSystem::Search( "tests/build_server/main.cpp", 3, 3, OUT path ));
		CHECK_FATAL( FileSystem::SetCurrentPath( path.remove_filename() ));

		AE_LOGI( "Current path: '" + FileSystem::CurrentPath().string() + "'" );

		CHECK_FATAL( FileSystem::RemoveAll( "output" ));
		CHECK_FATAL( FileSystem::CreateDirectory( "output" ));
		CHECK_FATAL( FileSystem::CreateDirectory( "output/temp" ));
		CHECK_FATAL( FileSystem::CreateDirectory( "output/deploy" ));
	}

# ifdef PLATFORM_WINDOWS
	WinTest();
# endif

	CHECK_FATAL( AE_DUMP_MEMLEAKS() );

	AE_LOGI( "Tests.BuildServer finished" );
	return 0;
}
