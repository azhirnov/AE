// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Platforms/WindowsHeader.h"

#include "exception_handler.h"
using namespace google_breakpad;

#include "Handler.h"
#include "stl/Algorithms/StringUtils.h"
#include "stl/Platforms/PlatformUtils.h"
using namespace AE::STL;


namespace
{
#	include "../crash_report_sender/ECrashReportSenderFlags.h"

	static ExceptionHandler *	g_Handler;
	static Path					g_CrashReporterPath;
	static Path					g_LogFilePath;
	static WString				g_CrashServerUrl;	// for local server: "127.0.0.1:8080"
	static WString				g_UserInfo;
	static WString				g_SymbolFileID;
	static const uint			g_TimeoutMS		= 60'000;

/*
=================================================
	FilterCallback
=================================================
*/
	static bool  FilterCallback (void* context, EXCEPTION_POINTERS* exinfo, MDRawAssertionInfo* assertion)
	{
		Unused( context, exinfo, assertion );
		return true;
	}
	
/*
=================================================
	MinidumpCallback
=================================================
*/
	static bool  MinidumpCallback (const wchar_t* dump_path,
									const wchar_t* minidump_id,
									void* context,
									EXCEPTION_POINTERS* exinfo,
									MDRawAssertionInfo* assertion,
									bool succeeded)
	{
		Unused( dump_path, minidump_id, context, exinfo, assertion, succeeded );

	#ifdef PLATFORM_WINDOWS
		
		// ask to send crash report
		if ( true )
		{
			const wchar_t	caption[]	= L"Send crash report?";
			WString			str			= L"ID: " + WString{minidump_id};

			int	result = ::MessageBoxExW( null, str.c_str(), caption,
										  MB_YESNO | MB_ICONSTOP | MB_SETFOREGROUND | MB_TOPMOST | MB_DEFBUTTON1,
										  MAKELANGID( LANG_ENGLISH, SUBLANG_ENGLISH_US ));
			switch ( result )
			{
				case IDYES :	break;
				case IDNO :
				default :		return false;	// don't send minidump
			};
		}

		WString		command_line;
		command_line
			<< g_CrashReporterPath.wstring()
			<< L" \"" << g_UserInfo << L'"'															// user info
			<< L" \"" << Path{dump_path}.append( WString{minidump_id} + L".dmp" ).wstring() << L'"'	// minidump file path
			<< L" \"" << g_LogFilePath.wstring() << L'"'											// log file path
			<< L" \"" << g_SymbolFileID << L'"'														// symbol file name
			<< L" \"" << g_CrashServerUrl << L'"'													// server url
			<< L" " << ToAnsiString<wchar_t>( ToString( ECrashReportSenderFlags::RemoveDumpFile ));	// flags

		return WindowsProcess::Execute( command_line, WindowsProcess::EFlags::NoWindow, milliseconds{g_TimeoutMS} );

	#else
		return true;
	#endif
	}

}	// namespace
//-----------------------------------------------------------------------------


namespace AE
{
/*
=================================================
	InitCrashHandler
----
	minidumpPath		- path where minidump will be stored
	logPath				- (optional) path to log file
	crashReporterPath	- path to 'CrashReportSender.exe'
	userInfo			- (optional) any user information (id, ...)
	symbolFileID		- (optional) ID/name of symbols file generated for current application
	url					- server address
=================================================
*/
	bool  InitCrashHandler (const Path &minidumpPath, Path logPath, Path crashReporterPath,
							WString userInfo, WString symbolFileID, WString url)
	{
		ASSERT( FileSystem::IsDirectory( minidumpPath ));
		ASSERT( FileSystem::IsFile( crashReporterPath ));

		g_CrashReporterPath = RVRef( crashReporterPath );
		g_LogFilePath		= RVRef( logPath );
		g_CrashServerUrl	= RVRef( url );
		g_UserInfo			= RVRef( userInfo );
		g_SymbolFileID		= RVRef( symbolFileID );

		g_Handler = new ExceptionHandler(
			minidumpPath.wstring(),
			FilterCallback,
			MinidumpCallback,
			0,
			ExceptionHandler::HANDLER_ALL,
			MiniDumpNormal,
			L"",
			0 );

		return true;
	}

}	// AE
