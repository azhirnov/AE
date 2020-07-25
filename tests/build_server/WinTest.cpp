// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#ifdef PLATFORM_WINDOWS

#define SERVER_IP	"127.0.0.1:8080"

#include "stl/Algorithms/StringUtils.h"
#include "stl/Utils/Helpers.h"
#include "stl/Utils/FileSystem.h"
#include "stl/Platforms/WindowsProcess.h"

#include "networking/http/HttpClient.h"

using namespace AE::STL;
using namespace AE::Threading;
using namespace AE::Networking;


/*
=================================================
	StartServer
=================================================
*/
static bool  StartServer (OUT WindowsProcess &proc)
{
	WString		command_line;
	command_line
		<< L"\"" AE_BUILD_SERVER_EXE "\""
		<< L" \"" SERVER_IP "\""		// server ip
		<< L" \"output/temp\""			// working directory
		<< L" \"output/deploy\"";		// deploy directory

	return proc.ExecuteAsync( command_line, WindowsProcess::EFlags::None );
}

/*
=================================================
	StopServer
=================================================
*/
static bool  StopServer (WindowsProcess &proc)
{
	return proc.WaitAndClose();
}

/*
=================================================
	SendScript
=================================================
*/
static bool  SendScript ()
{
	const char	script[] = R"#(
string			version = "1";
array<string>	defines;
defines.push_back( "BS_VERSION=\"" + version + "\"" );
defines.push_back( "BS_FAIL_TEST=OFF" );

api.GitClone( "https://github.com/azhirnov/AE-BuildServerTest.git", "master", "." );
api.MakeDir( "build" );
api.CurDir( "build" );
api.CMakeGen( ECompiler_VisualStudio2019_v141, EArch_x64, /*source dir*/"..", /*build dir*/"", defines );
api.CMakeBuild( /*build dir*/"", /*config*/"Release", /*target*/"" );
if ( api.CTest( "", "Release" ))
{
	api.CMakeInstall( /*build dir*/"", /*install dir*/"../install", /*config*/"Release", /*target*/"" );
	api.Deploy( version, /*dist dir*/"../install/dist", /*dbg sym dir*/"../install/dbgsym" );
}
)#";
	
	using EMethod	= HttpRequestDesc::EMethod;
	using ECode		= HttpRequest::ECode;
	using EStatus	= IAsyncTask::EStatus;

	auto&	network = HttpClient::Instance();
	String	build_id;

	// build
	{
		auto	task = network.Send( HttpRequestDesc{ SERVER_IP "/build" }.Method( EMethod::Put ).Content( script ));
		CHECK_ERR( Scheduler().Wait( {task} ));
		CHECK_ERR( task->Status() == EStatus::Completed );
		
		auto	resp = task->Response();
		CHECK_ERR( resp->code == ECode::OK );
		
		StringView	str{ (char*)resp->content.data(), resp->content.size() };
		StringView	key = "BuildID: ";

		size_t		pos1 = str.find( key );
		CHECK_ERR( pos1 != StringView::npos );
		pos1 += key.length();

		size_t		pos2 = pos1;
		for (; pos2 < str.length(); ++pos2)
		{
			char	c = str[pos2];
			if ( not ((c >= '0' and c <= '9') or (c >= 'a' and c <= 'f') or (c >= 'A' and c <= 'F') ))
				break;
		}

		build_id = str.substr( pos1, pos2 - pos1 );
	}

	// get status
	for (;;)
	{
		auto	task = network.Send( HttpRequestDesc{ SERVER_IP "/build_status/" + build_id }.Method( EMethod::Get ));
		CHECK_ERR( Scheduler().Wait( {task} ));
		CHECK_ERR( task->Status() == EStatus::Completed );
		
		auto	resp = task->Response();
		CHECK_ERR( resp->code == ECode::OK );

		StringView	str{ (char*)resp->content.data(), resp->content.size() };
		if ( str == "=== eof ===" )
			break;

		if ( str.size() )
			AE_LOGI( "response:\n"s << str );

		std::this_thread::sleep_for( milliseconds{50} );
	}
	
	// stop server
	{
		auto	task = network.Send( HttpRequestDesc{ SERVER_IP "/stop" }.Method( EMethod::Put ));
		CHECK_ERR( Scheduler().Wait( {task} ));
		CHECK_ERR( task->Status() == EStatus::Completed );
	}
	return true;
}

/*
=================================================
	WinTest
=================================================
*/
extern void  WinTest ()
{
	CHECK_FATAL( FileSystem::Exists( AE_BUILD_SERVER_EXE ));
	
	TaskScheduler::CreateInstance();
	Scheduler().Setup( 1 );
	HttpClient::Instance().Setup( Default );

	WindowsProcess	proc;

	CHECK_FATAL( StartServer( OUT proc ));
	CHECK_FATAL( SendScript() );
	CHECK_FATAL( StopServer( proc ));
	
	Scheduler().Release();
	TaskScheduler::DestroyInstance();
	HttpClient::Instance().Release();
}

#endif	// PLATFORM_WINDOWS
