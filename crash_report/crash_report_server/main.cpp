// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Containers/NtStringView.h"
#include "stl/Stream/BrotliStream.h"
#include "stl/Stream/FileStream.h"
#include "stl/Algorithms/ArrayUtils.h"
#include "stl/Algorithms/StringUtils.h"
#include "stl/Utils/FileSystem.h"

#include "threading/Common.h"

#include "stl/Platforms/WindowsHeader.h"
#include "mongoose.h"

#define SERVER_NAME	"CrashReportServer"


namespace
{
	using namespace AE::STL;
	using namespace AE::Threading;

#	include "../minidump_parser/CrashFileHeader.h"
#	include "ECrashReportServerFalgs.h"


	//
	// Crash Report Server
	//

	class CrashReportServer
	{
	// variables
	private:
		mg_mgr						_mongoose;
		mg_connection *				_connection		= null;
		mg_serve_http_opts			_options;

		Atomic<bool>				_looping;

		ECrashReportServerFalgs		_flags	= Default;


	// methods
	public:
		CrashReportServer () {}
		~CrashReportServer () {}

		bool  Start (NtStringView url, const Path &folder, ECrashReportServerFalgs flags);

	private:
		void  _Stop ();
		
		static void  _EventHandler (mg_connection* nc, int ev, void* eventData, void* userData);
		static void  _HandleUpload (mg_connection* nc, int ev, void* eventData, void* userData);

		static String  _IPtoFileName (mg_connection* nc);
	};
	

/*
=================================================
	Start
=================================================
*/
	bool  CrashReportServer::Start (NtStringView url, const Path &folder, ECrashReportServerFalgs flags)
	{
		CHECK_ERR( FileSystem::IsDirectory( folder ));
		CHECK_ERR( FileSystem::SetCurrentPath( folder ));

		_flags = flags;

		mg_mgr_init( OUT &_mongoose, null );

		_connection = mg_bind( &_mongoose, url.c_str(), _EventHandler, null );
		CHECK_ERR( _connection );
		
		_options = {};
		_options.document_root = ".";

		mg_register_http_endpoint( _connection, "/upload", _HandleUpload, this );

		mg_set_protocol_http_websocket( _connection );

		AE_LOGI( SERVER_NAME " started: '"s << url << "'" );

		_looping = true;
		for (; _looping.load( EMemoryOrder::Relaxed );)
		{
			mg_mgr_poll( &_mongoose, 1000 );
		}

		mg_mgr_free( &_mongoose );
		_connection = null;
		
		AE_LOGI( "Server stopped" );
		return true;
	}
	
/*
=================================================
	_Stop
=================================================
*/
	void  CrashReportServer::_Stop ()
	{
		_looping.store( false, EMemoryOrder::Relaxed );
	}
	
/*
=================================================
	_EventHandler
=================================================
*/
	void  CrashReportServer::_EventHandler (mg_connection *nc, int ev, void *eventData, void* userData)
	{
		auto*	self = Cast<CrashReportServer>(userData);

		switch ( ev )
		{
			case MG_EV_ACCEPT :
			{
				char	addr[32] = {};
				mg_sock_addr_to_str( &nc->sa, OUT addr, CountOf(addr), MG_SOCK_STRINGIFY_IP | MG_SOCK_STRINGIFY_PORT );
				AE_LOGI( "new connection: "s << addr );
				break;
			}

			case MG_EV_CLOSE :
			{
				char	addr[32] = {};
				mg_sock_addr_to_str( &nc->sa, OUT addr, CountOf(addr), MG_SOCK_STRINGIFY_IP | MG_SOCK_STRINGIFY_PORT );
				AE_LOGI( "connection closed: "s << addr );
				break;
			}

			case MG_EV_HTTP_REQUEST :
			{
				auto&		msg		= *Cast<http_message>(eventData);
				StringView	method	{ msg.method.p, msg.method.len };
				StringView	url		{ msg.uri.p, msg.uri.len };
				StringView	body	{ msg.body.p, msg.body.len };
				
				if ( method == "POST" and url == "/stop" )
				{
					self->_Stop();
					mg_printf( nc, "%s", "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n" );
					nc->flags |= MG_F_SEND_AND_CLOSE;
				}
				else
				{
					mg_printf( nc,
							   "HTTP/1.1 404 Not Found\r\n"
							   "Server: " SERVER_NAME "\r\n"
							   "Content-Type: text/html\r\n"
							   "Content-Length: 0\r\n"
							   "Connection: close\r\n" );
					nc->flags |= MG_F_SEND_AND_CLOSE;
				}
				break;
			}

			case MG_EV_RECV :
				break;

			case MG_EV_SEND :
				break;
		}
	}
	
/*
=================================================
	_IPtoFileName
=================================================
*/
	String  CrashReportServer::_IPtoFileName (mg_connection* nc)
	{
		char	addr[32] = {};
		mg_sock_addr_to_str( &nc->sa, OUT addr, CountOf(addr), MG_SOCK_STRINGIFY_IP | MG_SOCK_STRINGIFY_PORT );

		String	result;
		result.reserve( CountOf(addr) );

		for (size_t i = 0; (i < CountOf(addr)) and (addr[i] != '\0'); ++i)
		{
			const char	c = addr[i];

			if ( (c >= 'A' and c <= 'Z') or (c >= 'a' and c <= 'z') or (c >= '0' and c <= '9') )
				result << c;
			else
				result << '_';
		}
		return result;
	}
	
/*
=================================================
	_HandleUpload
=================================================
*/
	void  CrashReportServer::_HandleUpload (mg_connection *nc, int ev, void* eventData, void* userData)
	{
		auto*	self = Cast<CrashReportServer>(userData);

		switch ( ev )
		{
			case MG_EV_HTTP_REQUEST :
			{
				FileWStream		file{ _IPtoFileName( nc )};
				bool			is_ok = false;

				if ( file.IsOpen() )
				{
					auto*	msg = Cast<http_message>(eventData);
				
					if ( file.Write( msg->body.p, BytesU{msg->body.len} ))
					{
						mg_printf( nc, "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n" );
						nc->flags |= MG_F_SEND_AND_CLOSE;
						is_ok = true;
					}
				}

				if ( not is_ok )
				{
					mg_printf( nc,	"HTTP/1.1 500 Failed to write to a file\r\n"
									"Content-Length: 0\r\n\r\n" );
					nc->flags |= MG_F_SEND_AND_CLOSE;
				}

				// for tests
				if ( AllBits( self->_flags, ECrashReportServerFalgs::ReceiveAndClose ))
					self->_Stop();

				break;
			}

		// ---- Multi-part ---- //

			case MG_EV_HTTP_PART_BEGIN :
			{
				auto*	mp = Cast<mg_http_multipart_part>(eventData);
				AE_LOGI( "MG_EV_HTTP_PART_BEGIN: "s << ToString<16>( size_t(mp->user_data) ));
				break;
			}

			case MG_EV_HTTP_PART_DATA :
			{
				auto*	mp = Cast<mg_http_multipart_part>(eventData);
				AE_LOGI( "MG_EV_HTTP_PART_DATA: "s << ToString<16>( size_t(mp->user_data) ));
				break;
			}
			
			case MG_EV_HTTP_PART_END :
			{
				auto*	mp = Cast<mg_http_multipart_part>(eventData);
				AE_LOGI( "MG_EV_HTTP_PART_END: "s << ToString<16>( size_t(mp->user_data) ));
				break;
			}
		}
	}

}	// namespace

/*
=================================================
	main
----
	argv[1]		- server url (IP)
	argv[2]		- working directory
	argv[3]		- flags
=================================================
*/
int  main (int argc, const char *argv[])
{
	CHECK_ERR( argc == 4 );

	CrashReportServer	server;
	CHECK_ERR( server.Start( argv[1], AE::STL::Path{argv[2]}, ECrashReportServerFalgs( StringToUInt( argv[3] ))), 1 );
	return 0;
}
