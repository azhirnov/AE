// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'
/*
	TODO:
	Windows:
		* build from git
		* run tests:
			* on specified GPU

	Android:
		???
*/

#include "BuildServerApi.h"

#include "stl/Platforms/WindowsHeader.h"

#pragma warning(disable: 4005)
#include "mongoose.h"

#define SERVER_NAME	"BuildServer"

namespace
{

	//
	// Build Server
	//

	class BuildServer
	{
	// types
	private:
		struct BuildInfo
		{
			Atomic<bool>	looping			{true};
			
			std::thread		thread;
			Atomic<bool>	threadJoined	{false};

			Mutex			outputGuard;
			String			output;
			
			Mutex			timeGuard;
			TimePoint_t		startTime;
			TimePoint_t		endTime;

			BuildInfo (std::thread &&t) : thread{std::move(t)} {}
		};

		using BuildID			= uint64_t;
		using ActiveBuilds_t	= HashMap< BuildID, UniquePtr<BuildInfo> >;


	// variables
	private:
		mg_mgr					_mongoose;
		mg_connection *			_connection		= null;
		mg_serve_http_opts		_options;
		Atomic<bool>			_looping;

		ActiveBuilds_t			_activeBuilds;
		const uint				_maxBuilds		= 10;
		const milliseconds		_keepBuildsFor	{60'000};

		Path					_deployDir;


	// methods
	public:
		BuildServer () {}
		~BuildServer () {}

		bool  Start (NtStringView url, Path workDir, Path deployDir);

	private:
		void  _Stop ();
		
		static void  _EventHandler (mg_connection* nc, int ev, void* eventData, void* userData);

		bool  _RunScript (StringView src, OUT BuildID &id);
		void  _CheckActiveBuilds ();

		void  _ProcessMessage (mg_connection* nc, const http_message &msg);

		static void  _RemoveAll (const Path &folder);
	};
//-----------------------------------------------------------------------------



/*
=================================================
	Start
=================================================
*/
	bool  BuildServer::Start (NtStringView url, Path workDir, Path deployDir)
	{
		ScriptEngineMultithreadingScope	eng_scope;

		// initialize
		{
			workDir		= FileSystem::ToAbsolute( workDir );
			deployDir	= FileSystem::ToAbsolute( deployDir );

			AE_LOGI( "Working folder: '"s << workDir.string() << "'" );
			AE_LOGI( "Deploy folder: '"s << deployDir.string() << "'" );

			CHECK_ERR( FileSystem::IsDirectory( workDir ));

			_RemoveAll( workDir );
			FileSystem::CreateDirectory( workDir );
			CHECK_ERR( FileSystem::SetCurrentPath( workDir ));
		
			CHECK_ERR( FileSystem::IsDirectory( deployDir ));
			_deployDir = deployDir;

			mg_mgr_init( OUT &_mongoose, null );
		
			_connection = mg_bind( &_mongoose, url.c_str(), _EventHandler, this );
			CHECK_ERR( _connection );
		
			_options = {};
			_options.document_root = ".";

			mg_set_protocol_http_websocket( _connection );

			AE_LOGI( SERVER_NAME " started: '"s << url << "'" );
		
			_looping = true;
		}

		// event loop
		{
			const uint	poll_interval			= 100;		// ms
			const uint	check_builds_interval	= 10'000;	// ms

			for (uint i = 0; _looping.load( EMemoryOrder::Relaxed ); ++i)
			{
				mg_mgr_poll( &_mongoose, poll_interval );
			
				if ( i * poll_interval > check_builds_interval )
				{
					_CheckActiveBuilds();
					i = 0;
				}
			}
		}

		// cleanup
		{
			mg_mgr_free( &_mongoose );
			_connection = null;
		
			// stop all threads
			for (auto& [id, build] : _activeBuilds)
			{
				build->looping.store( false, EMemoryOrder::Relaxed );

				if ( not build->threadJoined.exchange(true) )
				{
					build->thread.join();
				}
			}
			_activeBuilds.clear();

			// remove temporary directory
			_RemoveAll( workDir );
			
			AE_LOGI( "Server stopped" );
		}

		return true;
	}
	
/*
=================================================
	_Stop
=================================================
*/
	void  BuildServer::_Stop ()
	{
		_looping.store( false, EMemoryOrder::Relaxed );
	}
	
/*
=================================================
	_EventHandler
=================================================
*/
	void  BuildServer::_EventHandler (mg_connection *nc, int ev, void *eventData, void* userData)
	{
		auto*	self = Cast<BuildServer>(userData);

		switch ( ev )
		{
			/*case MG_EV_ACCEPT :
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
			}*/

			case MG_EV_HTTP_REQUEST :
			{
				self->_ProcessMessage( nc, *Cast<http_message>(eventData) );
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
	_ProcessMessage
=================================================
*/
	void  BuildServer::_ProcessMessage (mg_connection* nc, const http_message &msg)
	{
		StringView	method	{ msg.method.p, msg.method.len };
		StringView	url		{ msg.uri.p, msg.uri.len };
		StringView	body	{ msg.body.p, msg.body.len };

		StringView	build_url			= "/build";
		StringView	build_status_url	= "/build_status/";
		StringView	stop_url			= "/stop";

		StringView	put_method			= "PUT";
		StringView	get_method			= "GET";

		if ( method == put_method and url == build_url )
		{
			if ( _activeBuilds.size() > _maxBuilds )
			{
				mg_printf( nc,
							"HTTP/1.1 500 To many active builds, try later\r\n"
							"Content-Length: 0\r\n\r\n" );
				nc->flags |= MG_F_SEND_AND_CLOSE;
				return;
			}

			BuildID	id;

			if ( _RunScript( body, OUT id ))
			{
				mg_printf( nc,
						   ("HTTP/1.1 200 OK\r\n"
							"Content-Type: text/plain\r\n\r\n"
							"BuildID: "s << ToString<16>( id ) << "\r\n").c_str() );
				nc->flags |= MG_F_SEND_AND_CLOSE;
			}
			else
			{
				mg_printf( nc,
						   ("HTTP/1.1 500 Failed to run script\r\n"
							"Content-Type: text/plain\r\n\r\n"
							"BuildID: "s << ToString<16>( id ) << "\r\n").c_str() );
				nc->flags |= MG_F_SEND_AND_CLOSE;
			}
		}
		else
		if ( method == get_method and StartsWith( url, build_status_url ))
		{
			BuildID	id		= StringToUInt64( url.substr( build_status_url.length() ), 16 );
			auto	iter	= _activeBuilds.find( id );

			if ( iter != _activeBuilds.end() )
			{
				String	log;
				{
					EXLOCK( iter->second->outputGuard );
					std::swap( log, iter->second->output );
				}

				if ( log.empty() and not iter->second->looping.load( EMemoryOrder::Relaxed ))
				{
					log = "=== eof ===";
				}
				
				mg_printf( nc,
						   ("HTTP/1.1 200 OK\r\n"
							"Content-Type: text/plain\r\n\r\n"s
							<< log).c_str() );
				nc->flags |= MG_F_SEND_AND_CLOSE;
			}
			else
			{
				mg_printf( nc,
							"HTTP/1.1 400 Failed to find active build\r\n"
							"Content-Length: 0\r\n\r\n" );
				nc->flags |= MG_F_SEND_AND_CLOSE;
			}
		}
		else
		if ( method == put_method and url == stop_url )
		{
			_Stop();
			mg_printf( nc,
					   "HTTP/1.1 200 OK\r\n"
					   "Content-Length: 0\r\n\r\n" );
			nc->flags |= MG_F_SEND_AND_CLOSE;
		}
		else
		{
			mg_printf( nc,
						"HTTP/1.1 404 Not Found\r\n"
						"Server: " SERVER_NAME "\r\n"
						"Content-Type: text/html\r\n"
						"Content-Length: 0\r\n\r\n" );
			nc->flags |= MG_F_SEND_AND_CLOSE;
		}
	}
	
/*
=================================================
	_RemoveAll
=================================================
*/
	void  BuildServer::_RemoveAll (const Path &folder)
	{
	#if 0
		FileSystem::RemoveAll( folder );

	#elif defined(PLATFORM_WINDOWS)
		String	cmd;
		cmd << "rmdir /Q /S \"" << folder.string() << '"';

		WindowsProcess::Execute( cmd, WindowsProcess::EFlags::NoWindow | WindowsProcess::EFlags::UseCommandPromt );

	#else
		#error not implemented
	#endif
	}

/*
=================================================
	_RunScript
=================================================
*/
	bool  BuildServer::_RunScript (StringView src, OUT BuildID &id)
	{
		String	script_src = "int main (BuildScriptApi &api) {\n"s << src << "\nreturn 0;\n}\n";
		Random	rnd;

		do {
			id = rnd.Uniform( 0ull, ~0ull );
		}
		while ( _activeBuilds.count( id ) > 0 );
		
		auto&	info = _activeBuilds.insert_or_assign( id, UniquePtr<BuildInfo>{} ).first->second;

		info.reset( new BuildInfo{ std::thread{ [&info, id, src = std::move(script_src), deploy_dir = _deployDir] () -> bool
		{
			{
				EXLOCK( info->timeGuard );
				info->startTime = TimePoint_t::clock::now();
			}
			ScriptThreadScope	scr_thread_scope;

			Path	work_dir = FileSystem::CurrentPath().append( "build-" + ToString<16>( id ));
			_RemoveAll( work_dir );
			
			if ( not FileSystem::CreateDirectory( work_dir ))
			{
				EXLOCK( info->outputGuard );
				info->output << "failed to create working directory\n";
				return false;
			}

			auto	se = MakeRC<ScriptEngine>();
			if ( not se->Create() )
			{
				EXLOCK( info->outputGuard );
				info->output << "failed to create script engine\n";
				return false;
			}
		
			if ( not BuildScriptApi::Bind( se ))
			{
				EXLOCK( info->outputGuard );
				info->output << "failed to bind script objects\n";
				return false;
			}

			BuildScriptApi	api{ info->output, info->outputGuard, info->looping, work_dir, std::move(deploy_dir) };
			auto			mod = se->CreateModule({ {"def", std::move(src)} });
			auto			scr = se->CreateScript< int (BuildScriptApi *) >( "main", mod );
			Optional<int>	res = scr->Run( &api );

			if ( not (res.has_value() and *res == 0) )
			{
				EXLOCK( info->outputGuard );
				info->output << "script exit with error: "s << ToString( res.value_or(0) ) << "\n";
			}

			_RemoveAll( work_dir );
			CHECK( not FileSystem::Exists( work_dir ));

			{
				EXLOCK( info->timeGuard );
				info->endTime = TimePoint_t::clock::now();
			}
			info->looping.store( false, EMemoryOrder::Relaxed );

			return true;
		}}});

		return true;
	}
	
/*
=================================================
	_CheckActiveBuilds
=================================================
*/
	void  BuildServer::_CheckActiveBuilds ()
	{
		for (auto iter = _activeBuilds.begin(); iter != _activeBuilds.end();)
		{
			auto&	build = iter->second;

			if ( build->looping.load( EMemoryOrder::Relaxed ))
			{
				++iter;
				continue;
			}

			// remove finished builds
			{
				EXLOCK( build->timeGuard );
				auto	dt = std::chrono::duration_cast<milliseconds>( TimePoint_t::clock::now() - build->endTime );

				if ( dt > _keepBuildsFor )
				{
					iter = _activeBuilds.erase( iter );
					continue;
				}
			}

			++iter;
		}
	}

}	// namespace

/*
=================================================
	main
----
	argv[1]		- server url (IP)
	argv[2]		- working directory
	argv[3]		- deploy directory
=================================================
*/
int  wmain (int argc, const wchar_t *argv[])
{
	CHECK_ERR( argc == 4 );

	BuildServer	server;
	CHECK_ERR( server.Start( ToAnsiString<char>(argv[1]), Path{argv[2]}, Path{argv[3]} ), 1 );
	return 0;
}
