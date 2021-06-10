// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'
/*
	Based on code from 'Breakpad', files:
		minidump_stackwalk.cc
		stackwalk_common.cc
*/

#include "MinidumpParser.h"
#include "stl/Stream/FileStream.h"
#include "stl/Stream/BrotliStream.h"
#include "stl/Stream/MemStream.h"
#include "stl/Stream/SubStream.h"
#include "stl/Stream/StdStream.h"
#include "stl/Algorithms/StringUtils.h"

#include "google_breakpad/processor/basic_source_line_resolver.h"
#include "google_breakpad/processor/minidump_processor.h"
#include "google_breakpad/processor/process_state.h"
#include "google_breakpad/processor/call_stack.h"
#include "google_breakpad/processor/stack_frame.h"

#include "processor/simple_symbol_supplier.h"
#include "processor/stackwalk_common.h"
#include "processor/pathname_stripper.h"

namespace AE::MinidumpParser
{
namespace
{
#	include "CrashFileHeader.h"
	
	using google_breakpad::SimpleSymbolSupplier;
	using google_breakpad::BasicSourceLineResolver;
	using google_breakpad::MinidumpProcessor;
	using google_breakpad::MinidumpThreadList;
	using google_breakpad::MinidumpMemoryList;
	using google_breakpad::Minidump;
	using google_breakpad::ProcessState;
	using google_breakpad::PathnameStripper;

/*
=================================================
	ProcessMinidump
=================================================
*/
	static bool  ProcessMinidump (ProcessState &process_state, uint callStackDepth, OUT CrashInfo &info)
	{
		if ( not process_state.crashed() )
			return false;

		const String	separator	= " | ";
		const auto		AddPart		= [separator] (const String &part) { return String{part.empty() ? ""s : separator} + part; };

		info.system.os	= process_state.system_info()->os + separator +
						  process_state.system_info()->os_version;
		info.system.cpu	= process_state.system_info()->cpu + separator +
						  process_state.system_info()->cpu_info +
						  " threads " + ToString( process_state.system_info()->cpu_count );
		info.system.gpu	= process_state.system_info()->gl_vendor +
						  AddPart( process_state.system_info()->gl_renderer ) +
						  AddPart( process_state.system_info()->gl_version );

		info.reason		= process_state.crash_reason();
		info.address	= process_state.crash_address();
		
		int		requesting_thread = process_state.requesting_thread();
		CHECK_ERR( requesting_thread != -1 );

		auto*	callstack	= process_state.threads()->at( requesting_thread );
		//auto*	mem_region	= process_state.thread_memory_regions()->at( requesting_thread );
		
		usize	frame_count = callstack->frames()->size();
		CHECK_ERR( frame_count > 0 );
		
		frame_count = Min( callStackDepth, frame_count );

		for (usize frame_index = 0; frame_index < frame_count; ++frame_index)
		{
			const auto*	frame				= callstack->frames()->at( frame_index );
			ulong	instruction_address = frame->ReturnAddress();

			if ( frame_index > 0 )
				info.callstack << "\n";

			if ( frame->module )
			{
				info.callstack << PathnameStripper::File( frame->module->code_file() );

				if ( not frame->function_name.empty() )
				{
					info.callstack << ' ' << frame->function_name;
					
					if ( not frame->source_file_name.empty() )
					{
						String	source_file = PathnameStripper::File( frame->source_file_name );
						info.callstack << ' ' << source_file << ": " << ToString( frame->source_line ) << " + 0x" << ToString( instruction_address - frame->source_line_base );
					}
					else
					{
						info.callstack << " + 0x" << ToString<16>( instruction_address - frame->function_base );
					}
				}
				else
				{
					info.callstack << " + 0x" << ToString<16>( instruction_address - frame->module->base_address() );
				}
			}
			else
			{
				info.callstack << " + 0x" << ToString<16>( instruction_address );
			}
		}
		return true;
	}
	
/*
=================================================
	ParseCrashReport_v1
=================================================
*/
	static bool  ParseCrashReport_v1 (const RC<RStream> &crashReport, const CrashFileHeader &header, Path symbolsPath, uint callStackDepth, OUT CrashInfo &info)
	{
		CHECK_ERR( header.version == 1 );

		// decompress
		RC<RStream>	rstream;
		{
			auto			mem_stream	= MakeRC<MemRStream>();
			BrotliRStream	brotli		{ crashReport };

			CHECK_ERR( brotli.IsOpen() );
			CHECK_ERR( mem_stream->Decompress( brotli ));
			rstream = mem_stream;
		}
		
		if ( header.symbolsId.size )
		{
			WString		fname;
			CHECK_ERR( rstream->SeekSet( Bytes{header.symbolsId.offset} ));
			CHECK_ERR( rstream->Read( header.symbolsId.size / sizeof(wchar_t), OUT fname ));

			symbolsPath.append( fname + L".sym" );
			CHECK_ERR( FileSystem::IsFile( symbolsPath ));
		}

		UniquePtr<SimpleSymbolSupplier>		symbol_supplier;
		symbol_supplier.reset( new SimpleSymbolSupplier{ symbolsPath.string() });
		
		BasicSourceLineResolver	resolver;
		MinidumpProcessor		minidump_processor{ symbol_supplier.get(), &resolver };

		MinidumpThreadList::set_max_threads( std::numeric_limits<uint32_t>::max() );
		MinidumpMemoryList::set_max_regions( std::numeric_limits<uint32_t>::max() );
		
		RC<RStream>	dump_file = MakeRC<RSubStream>( rstream, Bytes{header.dump.offset}, Bytes{header.dump.size} );
		CHECK_ERR( dump_file->IsOpen() );

		StreambufWrap<char>	dump_streambuf{ dump_file };
		std::istream		dump_stream{ &dump_streambuf };
		Minidump			dump{ dump_stream };
		CHECK_ERR( dump.Read() );

		ProcessState	process_state;

		if ( minidump_processor.Process( &dump, &process_state ) != google_breakpad::PROCESS_OK )
			return false;

		CHECK_ERR( ProcessMinidump( process_state, callStackDepth, OUT info ));

		if ( header.userInfo.size )
		{
			CHECK_ERR( rstream->SeekSet( Bytes{header.userInfo.offset} ));
			CHECK_ERR( rstream->Read( header.userInfo.size / sizeof(wchar_t), OUT info.userInfo ));
		}

		if ( header.log.size )
		{
			CHECK_ERR( rstream->SeekSet( Bytes{header.log.offset} ));
			CHECK_ERR( rstream->Read( header.log.size, OUT info.log ));
		}
		return true;
	}

}	// namespace
//-----------------------------------------------------------------------------


/*
=================================================
	ParseMinidump
=================================================
*/
	bool  ParseMinidump (const Path &minidumpPath, const Path &symbolsPath, uint callStackDepth, OUT CrashInfo &info)
	{
		info = Default;

		UniquePtr<SimpleSymbolSupplier>		symbol_supplier;
		symbol_supplier.reset( new SimpleSymbolSupplier{ symbolsPath.string() });
		
		BasicSourceLineResolver	resolver;
		MinidumpProcessor		minidump_processor{ symbol_supplier.get(), &resolver };

		MinidumpThreadList::set_max_threads( std::numeric_limits<uint32_t>::max() );
		MinidumpMemoryList::set_max_regions( std::numeric_limits<uint32_t>::max() );
		
		Minidump	dump{ minidumpPath.string() };
		CHECK_ERR( dump.Read() );

		ProcessState	process_state;

		if ( minidump_processor.Process( &dump, &process_state ) != google_breakpad::PROCESS_OK )
			return false;
		
		return ProcessMinidump( process_state, callStackDepth, OUT info );
	}
	
/*
=================================================
	ParseCrashReport
=================================================
*/
	bool  ParseCrashReport (const RC<RStream> &crashReport, Path symbolsFolder, uint callStackDepth, OUT CrashInfo &info)
	{
		info = Default;

		CHECK_ERR( crashReport and crashReport->IsOpen() );

		CrashFileHeader		header;
		CHECK_ERR( crashReport->Read( OUT header ));

		CHECK_ERR( header.magic == CrashFileHeader::MAGIC );
		
		switch ( header.version )
		{
			case CrashFileHeader::VERSION :
				return ParseCrashReport_v1( crashReport, header, RVRef(symbolsFolder), callStackDepth, OUT info );
		}
		
		RETURN_ERR( "unsupported version" );
	}

}	// AE::MinidumpParser
