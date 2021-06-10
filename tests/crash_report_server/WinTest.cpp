// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#ifdef PLATFORM_WINDOWS

#include "stl/Algorithms/StringUtils.h"
#include "stl/Utils/FileSystem.h"
#include "stl/Stream/FileStream.h"
#include "stl/Stream/BrotliStream.h"
#include "stl/Stream/MemStream.h"
#include "stl/Math/Random.h"
#include "stl/Platforms/WindowsProcess.h"

#include "stl/Platforms/WindowsHeader.h"
using namespace AE::STL;

#define SERVER_IP		"127.0.0.1:8080"
#define MINIDUMP_FILE	"18a244ca-111b-412e-a728-bd95a41924ef.dmp"
#define LOG_FILE		"log.txt"
#define SYMBOLS_ID		"Tests.Breakpad.exe"

#include "../crash_report_server/ECrashReportServerFalgs.h"
#include "../minidump_parser/CrashFileHeader.h"


/*
=================================================
	StartServer
=================================================
*/
static bool  StartServer (OUT WindowsProcess &proc)
{
	WString		command_line;
	command_line
		<< L"\"" AE_CRASH_REPORT_SERVER_EXE "\""
		<< L" \"" SERVER_IP "\""																	// server ip
		<< L" \"../output\""																		// working folder
		<< L" " << ToAnsiString<wchar_t>( ToString( ECrashReportServerFalgs::ReceiveAndClose ));	// flags

	return proc.ExecuteAsync( command_line, WindowsProcess::EFlags::Unknown );
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
	SendReport
=================================================
*/
static bool  SendReport (WStringView userId)
{
	WString		command_line;
	command_line
		<< L"\"" AE_CRASH_REPORT_SENDER_EXE "\""
		<< L" \"" << userId << L"\""				// user info
		<< L" \"" MINIDUMP_FILE "\""				// minidump file path
		<< L" \"" LOG_FILE "\""						// log file path
		<< L" \"" SYMBOLS_ID "\""					// symbol file name
		<< L" \"" SERVER_IP "/upload\""				// server ulr
		<< L" 0";									// flags

	return WindowsProcess::Execute( command_line, Default );
}

/*
=================================================
	CheckCrashReport
=================================================
*/
static bool  CheckCrashReport (WStringView userIdRef)
{
	for (auto& path : FileSystem::Enum( "../output" ))
	{
		if ( FileSystem::IsFile( path ))
		{
			RC<RStream>		file = MakeRC<FileRStream>( path );
			CHECK_ERR( file->IsOpen() );

			CrashFileHeader	header;
			CHECK_ERR( file->Read( OUT header ));

			CHECK_ERR( header.magic == CrashFileHeader::MAGIC );
			CHECK_ERR( header.version == CrashFileHeader::VERSION );
			CHECK_ERR( header.userInfo.size > 0 );
			
			BrotliRStream	brotli{ file };
			MemRStream		mem;
			CHECK_ERR( mem.Decompress( brotli ));
			
			{
				WString		user_id;
				CHECK_ERR( mem.SeekSet( Bytes{header.userInfo.offset} ));
				CHECK_ERR( mem.Read( header.userInfo.size / sizeof(wchar_t), OUT user_id ));
				CHECK_ERR( user_id == userIdRef );
			}{
				WString		symbols_id;
				CHECK_ERR( mem.SeekSet( Bytes{header.symbolsId.offset} ));
				CHECK_ERR( mem.Read( header.symbolsId.size / sizeof(wchar_t), OUT symbols_id ));
				CHECK_ERR( symbols_id == L"" SYMBOLS_ID );
			}{
				Array<uint8_t>	dump_ref;
				FileRStream		file2{ MINIDUMP_FILE };
				CHECK_ERR( file2.IsOpen() );
				CHECK_ERR( file2.Read( usize(file2.RemainingSize()), OUT dump_ref ));
				
				Array<uint8_t>	dump;
				CHECK_ERR( mem.SeekSet( Bytes{header.dump.offset} ));
				CHECK_ERR( mem.Read( header.dump.size, OUT dump ));
				CHECK_ERR( dump == dump_ref );
			}{
				String			log_ref;
				FileRStream		file2{ LOG_FILE };
				CHECK_ERR( file2.IsOpen() );
				CHECK_ERR( file2.Read( usize(file2.RemainingSize()), OUT log_ref ));
			
				String			log;
				CHECK_ERR( mem.SeekSet( Bytes{header.log.offset} ));
				CHECK_ERR( mem.Read( header.log.size, OUT log ));
				CHECK_ERR( log == log_ref );
			}
			return true;
		}
	}
	return false;
}

/*
=================================================
	WinTest
=================================================
*/
extern void  WinTest ()
{
	CHECK_FATAL( FileSystem::Exists( AE_CRASH_REPORT_SERVER_EXE ));
	CHECK_FATAL( FileSystem::Exists( AE_CRASH_REPORT_SENDER_EXE ));

	WindowsProcess	proc;
	WString			user_id	= L"ID: "s << ToAnsiString<wchar_t>( ToString( Random{}.Uniform( 0ull, ~0ull )));

	CHECK_FATAL( StartServer( OUT proc ));
	CHECK_FATAL( SendReport( user_id ));
	CHECK_FATAL( StopServer( proc ));
	CHECK_FATAL( CheckCrashReport( user_id ));
}

#endif	// PLATFORM_WINDOWS
