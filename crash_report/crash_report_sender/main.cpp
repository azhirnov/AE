// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Platforms/WindowsHeader.h"
#include <curl/curl.h>

#include "stl/Stream/MemStream.h"
#include "stl/Stream/BrotliStream.h"
#include "stl/Stream/FileStream.h"
#include "stl/Utils/Helpers.h"
#include "stl/Algorithms/ArrayUtils.h"
#include "stl/Algorithms/StringUtils.h"
using namespace AE::STL;

namespace
{
#	include "ECrashReportSenderFlags.h"
#	include "../minidump_parser/CrashFileHeader.h"

/*
=================================================
	UploadCallback
=================================================
*/
	static usize  UploadCallback (char *buffer, usize size, usize nitems, void *userdata)
	{
		MemRStream*	stream = Cast<MemRStream>( userdata );

		if ( not stream or not stream->IsOpen() )
			return CURLE_ABORTED_BY_CALLBACK;

		usize	required	= size * nitems;
		usize	written		= usize( stream->Read2( OUT buffer, Bytes{required} ));
		return written;
	}

/*
=================================================
	UploadMinidump
=================================================
*/
	static bool  UploadMinidump (NtWStringView userInfo, const Path &minidumpPath, const Path &logPath,
								 NtWStringView symbolsId, NtStringView url, ECrashReportSenderFlags flags)
	{
		AE_LOGI( "Sending crash report to '"s << url << "' ..." );

		RC<MemRStream>			rstream;
		RC<MemWStream>			wstream		= MakeRC<MemWStream>();
		BrotliWStream::Config	brotli_cfg;	brotli_cfg.quality = 0.7f;
		CrashFileHeader			header		{};

		header.version	= CrashFileHeader::VERSION;
		header.magic	= CrashFileHeader::MAGIC;

		// placeholder for header
		CHECK_ERR( wstream->Write( &header, Bytes::SizeOf(header) ));

		// compress
		{
			BrotliWStream	compressed	{ wstream, brotli_cfg };
			ubyte			buf[ 1u << 12 ];

			// write user info
			{
				header.userInfo.offset	= uint(compressed.Position());
				header.userInfo.size	= uint(StringSizeOf( userInfo ));
				CHECK_ERR( compressed.Write( WStringView{userInfo} ));
			}

			// write symbol file name
			{
				header.symbolsId.offset	= uint(compressed.Position());
				header.symbolsId.size	= uint(StringSizeOf( symbolsId ));
				CHECK_ERR( compressed.Write( WStringView{symbolsId} ));
			}

			// write minidump
			{
				FileRStream		rfile{ minidumpPath };
				CHECK_ERR( rfile.IsOpen() );
			
				header.dump.offset	= uint(compressed.Position());
				header.dump.size	= uint(rfile.Size());

				for (;;)
				{
					Bytes	size = rfile.Read2( buf, Bytes::SizeOf( buf ));
					if ( size == 0_b )
						break;

					CHECK_ERR( compressed.Write( buf, size ));
				}
			}

			// write log
			{
				FileRStream		rfile{ logPath };
			
				if ( rfile.IsOpen() )
				{
					header.log.offset	= uint(compressed.Position());
					header.log.size		= uint(rfile.Size());

					for (;;)
					{
						Bytes	size = rfile.Read2( buf, Bytes::SizeOf( buf ));
						if ( size == 0_b )
							break;

						CHECK_ERR( compressed.Write( buf, size ));
					}
				}
				else
				{
					header.log.offset	= UMax;
					header.log.size		= 0;
				}
			}

			compressed.Flush();
		}

		// update header
		CHECK_ERR( wstream->SeekSet( 0_b ));
		CHECK_ERR( wstream->Write( &header, Bytes::SizeOf(header) ));

		rstream = MakeRC<MemRStream>( wstream->GetData() );

	
		CURL*	curl = curl_easy_init();
		CHECK_ERR( curl );

		OnDestroy	curl_cleanup{ [&curl] () { curl_easy_cleanup( curl ); curl = null; }};
	
		curl_easy_setopt( curl, CURLOPT_URL,				url.c_str() );
		curl_easy_setopt( curl, CURLOPT_UPLOAD,				1L );
		curl_easy_setopt( curl, CURLOPT_READDATA,			rstream.get() );
		curl_easy_setopt( curl, CURLOPT_READFUNCTION,		&UploadCallback );
		curl_easy_setopt( curl, CURLOPT_INFILESIZE_LARGE,	curl_off_t(rstream->Size()) );
		curl_easy_setopt( curl, CURLOPT_VERBOSE,			1L );
 
		CHECK_ERR( curl_easy_perform( curl ) == CURLE_OK );
	
		long	response_code = 0;
		curl_easy_getinfo( curl, CURLINFO_RESPONSE_CODE, OUT &response_code );

		// remove minidump and log files
		if ( response_code == 200 )
		{
			AE_LOGI( "Done" );

			if ( AllBits( flags, ECrashReportSenderFlags::RemoveDumpFile ))
				FileSystem::Remove( minidumpPath );
			
			if ( AllBits( flags, ECrashReportSenderFlags::RemoveLogFile ))
				FileSystem::Remove( logPath );
		}
		else
		{
			AE_LOGI( "Failed with code: "s << ToString( response_code ));
		}

		return true;
	}

}	// namespace
//-----------------------------------------------------------------------------


/*
=================================================
	wmain
----
	argv[0] - exe path
	argv[1] - user info (user id, date, ...)
	argv[2] - minidump file path
	argv[3] - log file path
	argv[4] - symbol file name (generated at build time)
	argv[5] - crash server url
	argv[6] - flags
=================================================
*/
int  wmain (int argc, const wchar_t *argv[])
{
	CHECK_ERR( argc == 7, 1 );
	CHECK_ERR( UploadMinidump( NtWStringView{argv[1]}, Path{argv[2]}, Path{argv[3]},
							   NtWStringView{argv[4]}, ToAnsiString<char>( argv[5] ), 
							   ECrashReportSenderFlags( StringToUInt( ToAnsiString<char>( argv[6] ))) ),
			   1 );
	return 0;
}
