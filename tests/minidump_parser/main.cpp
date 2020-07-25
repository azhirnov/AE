// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Utils/FileSystem.h"
#include "stl/Stream/FileStream.h"
#include "MinidumpParser.h"
using namespace AE::STL;

#define TEST	CHECK_FATAL


static void  Minidump_Test1 ()
{
	AE::MinidumpParser::CrashInfo	info;
	TEST( AE::MinidumpParser::ParseMinidump( "18a244ca-111b-412e-a728-bd95a41924ef.dmp", "Tests.Breakpad.exe.sym", 1, OUT info ));

	TEST( info.reason == "EXCEPTION_ACCESS_VIOLATION_WRITE" );
	TEST( info.address == 0 );
	TEST( info.callstack == "Tests.Breakpad.exe + 0x15ed" );
	TEST( info.userInfo == L"" );
	TEST( info.log == "" );
	TEST( info.system.os == "Windows NT | 10.0.18363 " );
	TEST( info.system.cpu == "amd64 | family 6 model 158 stepping 10 threads 12" );
	TEST( info.system.gpu == "" );
}


static void CrashReport_Test1 ()
{
	SharedPtr<RStream>	stream = MakeShared<FileRStream>( "crash_report.bin" );
	TEST( stream->IsOpen() );

	AE::MinidumpParser::CrashInfo	info;
	TEST( AE::MinidumpParser::ParseCrashReport( stream, "", 1, OUT info ));
	
	TEST( info.reason == "EXCEPTION_ACCESS_VIOLATION_WRITE" );
	TEST( info.address == 0 );
	TEST( info.callstack == "Tests.Breakpad.exe + 0x15ed" );
	TEST( info.userInfo == L"ID: 10442745955219762557" );
	TEST( info.log == "log test" );
	TEST( info.system.os == "Windows NT | 10.0.18363 " );
	TEST( info.system.cpu == "amd64 | family 6 model 158 stepping 10 threads 12" );
	TEST( info.system.gpu == "" );
}


int main ()
{
	TEST( FileSystem::FindAndSetCurrent( "minidump_parser/data", 5 ));

	Minidump_Test1();
	CrashReport_Test1();

	CHECK_FATAL( AE_DUMP_MEMLEAKS() );

	AE_LOGI( "Tests.MinidumpParser finished" );
	return 0;
}
