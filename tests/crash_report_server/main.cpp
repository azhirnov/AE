// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Utils/FileSystem.h"
using namespace AE::STL;

#define TEST	CHECK_FATAL

#ifdef PLATFORM_WINDOWS
extern void  WinTest ();
#endif


int main ()
{
	TEST( FileSystem::FindAndSetCurrent( "tests/crash_report_server/input", 5 ));

	FileSystem::RemoveAll( "../output" );
	FileSystem::CreateDirectory( "../output" );

# ifdef PLATFORM_WINDOWS
	WinTest();
# endif

	CHECK_FATAL( AE_DUMP_MEMLEAKS() );

	AE_LOGI( "Tests.CrashReportServer finished" );
	return 0;
}
