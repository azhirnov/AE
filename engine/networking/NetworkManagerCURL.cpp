// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#ifdef AE_ENABLE_CURL
# include "networking/NetworkManager.h"
# include "threading/Primitives/DataRaceCheck.h"
# include "stl/Algorithms/StringUtils.h"
# include "stl/Algorithms/Cast.h"
# include "stl/Platforms/WindowsHeader.h"
# include <curl/curl.h>

# ifdef AE_DEBUG
#	define CURL_CALL( ... ) { \
			CURLcode _err_ = __VA_ARGS__; \
			ASSERT( _err_ == CURLE_OK ); \
		}
#	define CURLM_CALL( ... ) { \
			CURLMcode _err_ = __VA_ARGS__; \
			ASSERT( _err_ == CURLM_OK ); \
		}
#	define CURLSH_CALL( ... ) { \
			CURLSHcode _err_ = __VA_ARGS__; \
			ASSERT( _err_ == CURLSHE_OK ); \
		}
# else
#	define CURL_CALL( ... )		{ __VA_ARGS__; }
#	define CURLM_CALL( ... )	{ __VA_ARGS__; }
#	define CURLSH_CALL( ... )	{ __VA_ARGS__; }
# endif


using namespace AE::Threading;

namespace AE::Networking
{
	using TimePoint_t	= std::chrono::high_resolution_clock::time_point;
	using Duration_t	= TimePoint_t::clock::duration;
	using Settings_t	= NetworkManager::Settings;


	//
	// CURL Request Task
	//

	class CurlRequestTask final : public RequestTask
	{
	// variables
	private:
		CURL*					_curl				= null;
		curl_slist*				_slist				= null;
		CURLMcode				_addToCurlmResult	= CURLM_LAST;
		CURLcode				_completionResult	= CURLE_OK;
		UniquePtr<RStream>		_content;
		TimePoint_t				_lastResponseTime;
		bool					_complete			= false;

		DEBUG_ONLY(
			char				_errorBuffer [CURL_ERROR_SIZE] = {};
		)
		DataRaceCheck			_drCheck;


	// methods
	public:
		CurlRequestTask (const RequestDesc &desc, const Settings_t &settings);
		CurlRequestTask (RequestDesc &&desc, const Settings_t &settings);
		~CurlRequestTask ();

			void Run () override;
			void OnCancel () override;

			void Enque (CURLM* curlm, CURLSH* shared);
			void Complete (CURLM* curlm, CURLcode code);
		ND_ bool IsComplete (CURLM* curlm, Duration_t responseTimeout);

	private:
		bool  _Setup (const RequestDesc &desc, const Settings_t &settings);
		void  _Release ();

		static size_t _DownloadCallback (char *ptr, size_t size, size_t nmemb, void *userdata);
		static size_t _UploadCallback (char *buffer, size_t size, size_t nitems, void *userdata);
		static size_t _HeaderCallback (char *buffer, size_t size, size_t nitems, void *userdata);
	};
	
/*
=================================================
	constructor
=================================================
*/
	CurlRequestTask::CurlRequestTask (const RequestDesc &desc, const Settings_t &settings)
	{
		EXLOCK( _drCheck );
		_Setup( desc, settings );
	}

	CurlRequestTask::CurlRequestTask (RequestDesc &&desc, const Settings_t &settings) :
		_content{ std::move(desc._content) }
	{
		EXLOCK( _drCheck );
		_Setup( desc, settings );
	}

/*
=================================================
	destructor
=================================================
*/
	CurlRequestTask::~CurlRequestTask ()
	{
		_Release();
	}
	
/*
=================================================
	_Setup
=================================================
*/
	bool  CurlRequestTask::_Setup (const RequestDesc &desc, const Settings_t &settings)
	{
		using EMethod = RequestDesc::EMethod;

		_curl = curl_easy_init();
		CHECK_ERR( _curl );
		
		CURL_CALL( curl_easy_setopt( _curl, CURLOPT_URL, desc._url.c_str() ));
		CURL_CALL( curl_easy_setopt( _curl, CURLOPT_PRIVATE, this ));

		// for multithreading
		CURL_CALL( curl_easy_setopt( _curl, CURLOPT_NOSIGNAL, 1L ));
		
		// debugging
		#ifdef AE_DEBUG
			CURL_CALL( curl_easy_setopt( _curl, CURLOPT_ERRORBUFFER, _errorBuffer ));
			CURL_CALL( curl_easy_setopt( _curl, CURLOPT_VERBOSE, 0L ));
			CURL_CALL( curl_easy_setopt( _curl, CURLOPT_NOPROGRESS, 1L ));
		#else
			CURL_CALL( curl_easy_setopt( _curl, CURLOPT_NOPROGRESS, 1L ));
			CURL_CALL( curl_easy_setopt( _curl, CURLOPT_VERBOSE, 0L ));
		#endif

		if ( false ) {
			CURL_CALL( curl_easy_setopt( _curl, CURLOPT_INTERFACE, "TODO: local address" ));
		}

		// redirections
		if ( desc._redirections ) {
			CURL_CALL( curl_easy_setopt( _curl, CURLOPT_FOLLOWLOCATION, 1L ));
			CURL_CALL( curl_easy_setopt( _curl, CURLOPT_MAXREDIRS, int(desc._redirections) ));
		} else {
			// disable redirections
			CURL_CALL( curl_easy_setopt( _curl, CURLOPT_FOLLOWLOCATION, 0L ));
		}

		// SSL
		if ( true ) {
			CURL_CALL( curl_easy_setopt( _curl, CURLOPT_USE_SSL, CURLUSESSL_ALL ));
		}

		if ( desc._verifyPeer ) {
			CURL_CALL( curl_easy_setopt( _curl, CURLOPT_SSL_VERIFYPEER, 1L ));
		} else {
			CURL_CALL( curl_easy_setopt( _curl, CURLOPT_SSL_VERIFYPEER, 0L ));
		}
		
		bool		use_read_fn		= false;
		const long	content_size	= _content ? int(_content->RemainingSize()) : 0;

		BEGIN_ENUM_CHECKS();
		switch ( desc._method )
		{
			case EMethod::Get : {
				CURL_CALL( curl_easy_setopt( _curl, CURLOPT_HTTPGET, 1L ));
				break;
			}
			case EMethod::Put : {
				CURL_CALL( curl_easy_setopt( _curl, CURLOPT_UPLOAD, 1L ));
				CURL_CALL( curl_easy_setopt( _curl, CURLOPT_INFILESIZE, content_size ));
				use_read_fn = true;
				break;
			}
			case EMethod::Post : {
				CURL_CALL( curl_easy_setopt( _curl, CURLOPT_POST, 1L ));
				CURL_CALL( curl_easy_setopt( _curl, CURLOPT_POSTFIELDS, null ));
				CURL_CALL( curl_easy_setopt( _curl, CURLOPT_POSTFIELDSIZE, content_size ));
				use_read_fn = true;
				break;
			}
			case EMethod::Patch : {
				CURL_CALL( curl_easy_setopt( _curl, CURLOPT_UPLOAD, 1L ));
				CURL_CALL( curl_easy_setopt( _curl, CURLOPT_INFILESIZE, content_size ));
				CURL_CALL( curl_easy_setopt( _curl, CURLOPT_CUSTOMREQUEST, "PATCH" ));
				use_read_fn = true;
				break;
			}
			case EMethod::Head : {
				CURL_CALL( curl_easy_setopt( _curl, CURLOPT_NOBODY, 1L ));
				break;
			}
			case EMethod::Delete : {
				CURL_CALL( curl_easy_setopt( _curl, CURLOPT_POST, 1L ));
				CURL_CALL( curl_easy_setopt( _curl, CURLOPT_CUSTOMREQUEST, "DELETE" ));
				CURL_CALL( curl_easy_setopt( _curl, CURLOPT_POSTFIELDSIZE, content_size ));
				use_read_fn = true;
				break;
			}
		}
		END_ENUM_CHECKS();

		// setup read function
		if ( use_read_fn ) {
			CURL_CALL( curl_easy_setopt( _curl, CURLOPT_READDATA, this ));
			CURL_CALL( curl_easy_setopt( _curl, CURLOPT_READFUNCTION, &_UploadCallback ));
		}

		// setup header function
		CURL_CALL( curl_easy_setopt( _curl, CURLOPT_HEADERDATA, this ));
		CURL_CALL( curl_easy_setopt( _curl, CURLOPT_HEADERFUNCTION, &_HeaderCallback ));

		// write function
		CURL_CALL( curl_easy_setopt( _curl, CURLOPT_BUFFERSIZE, int(settings.downloadBufferSize) ));
		CURL_CALL( curl_easy_setopt( _curl, CURLOPT_WRITEDATA, this ));
		CURL_CALL( curl_easy_setopt( _curl, CURLOPT_WRITEFUNCTION, &_DownloadCallback ));

		// timeout
		if ( settings.connectionTimeout.count() ) {
			CURL_CALL( curl_easy_setopt( _curl, CURLOPT_CONNECTTIMEOUT_MS, int(settings.connectionTimeout.count()) ));
		}
		if ( settings.transferTimout.count() ) {
			CURL_CALL( curl_easy_setopt( _curl, CURLOPT_TIMEOUT_MS, int(settings.transferTimout.count()) ));
		}

		// add headers
		bool	has_user_agent		= false;
		bool	has_content_length	= false;

		for (auto& hdr : desc._headers)
		{
			has_user_agent		|= StartsWith( hdr, "User-Agent: " );
			has_content_length	|= StartsWith( hdr, "Content-Length: " );

			_slist = curl_slist_append( _slist, hdr.c_str() );
		}

		//if ( not has_user_agent )
		//	_slist = curl_slist_append( _slist, "TODO: set user agent" );

		// 'For compatibility with HTTP/1.0 applications, HTTP/1.1 requests containing a message-body MUST include a
		//  valid Content-Length header field unless the server is known to be HTTP/1.1 compliant.'
		// from https://www.w3.org/Protocols/rfc2616/rfc2616-sec4.html#sec4.4
		if ( not has_content_length ) {
			_slist = curl_slist_append( _slist, ("Content-Length: "s << ToString(content_size)).c_str() );
		}

		if ( _slist ) {
			CURL_CALL( curl_easy_setopt( _curl, CURLOPT_HTTPHEADER, _slist ));
		}

		// TODO:
		//	CURLOPT_ACCEPT_ENCODING
		//	CURLOPT_PROXY
		//	CURLOPT_FORBID_REUSE
		//	CURLOPT_SSLCERTTYPE
		//	CURLOPT_CAINFO
		//	CURLOPT_SSL_CTX_FUNCTION
		//	CURLOPT_SSL_CTX_DATA
		//	set empty 'Expect:' header http://www.iandennismiller.com/posts/curl-http1-1-100-continue-and-multipartform-data-post.html

		return true;
	}
	
/*
=================================================
	_Release
=================================================
*/
	void  CurlRequestTask::_Release ()
	{
		EXLOCK( _drCheck );

		if ( _curl )
		{
			curl_easy_setopt( _curl, CURLOPT_DEBUGDATA, null );
			curl_easy_cleanup( _curl );
			_curl = null;
		}

		if ( _slist )
		{
			curl_slist_free_all( _slist );
			_slist = null;
		}

		_complete = true;
	}
	
/*
=================================================
	_UploadCallback
=================================================
*/
	size_t  CurlRequestTask::_UploadCallback (char *buffer, size_t size, size_t nitems, void *userdata)
	{
		auto*	self = Cast<CurlRequestTask>( userdata );
		EXLOCK( self->_drCheck );
		
		if ( self->Status() != EStatus::InProgress )
			return CURLE_ABORTED_BY_CALLBACK;

		if ( not self->_content )
			return 0;

		size_t	required	= size * nitems;
		size_t	written		= size_t( self->_content->Read2( OUT buffer, BytesU{required} ));

		self->_bytesSent.fetch_add( written, memory_order_relaxed );

		return written;
	}

/*
=================================================
	_DownloadCallback
=================================================
*/
	size_t  CurlRequestTask::_DownloadCallback (char *ptr, size_t size, size_t nmemb, void *userdata)
	{
		auto*	self = Cast<CurlRequestTask>( userdata );
		EXLOCK( self->_drCheck );

		if ( self->Status() != EStatus::InProgress )
			return 0;

		self->_lastResponseTime = TimePoint_t::clock::now();

		size_t	download_size	= size * nmemb;
		if ( download_size == 0 )
			return 0;

		auto&	content = self->_response->content;
		size_t	offset	= content.size();

		content.resize( offset + download_size );
		std::memcpy( content.data() + offset, ptr, download_size );

		self->_bytesReceived.fetch_add( download_size, memory_order_relaxed );

		return download_size;
	}
	
/*
=================================================
	_HeaderCallback
=================================================
*/
	size_t  CurlRequestTask::_HeaderCallback (char *buffer, size_t size, size_t nitems, void *userdata)
	{
		auto*	self = Cast<CurlRequestTask>( userdata );
		EXLOCK( self->_drCheck );
		
		self->_lastResponseTime = TimePoint_t::clock::now();
		
		const size_t	header_size = size * nitems;

		if ( header_size > 0 and header_size <= CURL_MAX_HTTP_HEADER )
		{
			String	header{ buffer, header_size };
			header.erase( std::remove_if( header.begin(), header.end(), [](auto c){ return (c == '\n') | (c == '\r'); }), header.end() );
			
			if ( header.size() )
			{
				const size_t	separator = header.find( ": " );

				if ( separator != String::npos )
					self->_response->headers.insert_or_assign( header.substr( 0, separator ), header.substr( separator + 2 ));
				else
					self->_response->headers.insert_or_assign( std::move(header), String() );
			}
			return header_size;
		}

		return 0;
	}

/*
=================================================
	Run
=================================================
*/
	void CurlRequestTask::Run ()
	{
		EXLOCK( _drCheck );

		CURLcode	err = curl_easy_perform( _curl );
		
		return Complete( null, err );
	}
	
/*
=================================================
	Enque
=================================================
*/
	void CurlRequestTask::Enque (CURLM* curlm, CURLSH* shared)
	{
		EXLOCK( _drCheck );

		_lastResponseTime = TimePoint_t::clock::now();
		
		if ( shared ) {
			CURL_CALL( curl_easy_setopt( _curl, CURLOPT_SHARE, shared ));
		}

		_addToCurlmResult = curl_multi_add_handle( curlm, _curl );
	}
	
/*
=================================================
	Complete
=================================================
*/
	void CurlRequestTask::Complete (CURLM* curlm, CURLcode code)
	{
		EXLOCK( _drCheck );

		if ( curlm and _addToCurlmResult == CURLM_OK )
		{
			CURLM_CALL( curl_multi_remove_handle( curlm, _curl ));
		}

		_completionResult = code;
		switch ( code )
		{
			case CURLE_OPERATION_TIMEDOUT :
				_response->code = ECode::OperationTimeout;
				break;

			case CURLE_COULDNT_CONNECT :
			case CURLE_COULDNT_RESOLVE_PROXY :
			case CURLE_COULDNT_RESOLVE_HOST :
				_response->code = ECode::ConnectionError;
				break;
		}

		long	response_code = 0;
		if ( curl_easy_getinfo( _curl, CURLINFO_RESPONSE_CODE, OUT &response_code ) == CURLE_OK and response_code != 0 ) {
			_response->code = ECode(response_code);
		}

		_complete = true;

		if ( _response->code != ECode::OK )
		{
			char*	url = null;
			CURL_CALL( curl_easy_getinfo( _curl, CURLINFO_EFFECTIVE_URL, OUT &url ));

			AE_LOGI( "http request code: "s << ToString(uint(_response->code))
				<< (url ? (", url: "s << url) : "")
				DEBUG_ONLY( << ", msg: " << _errorBuffer )
			);
		}

		_Release();
	}
	
/*
=================================================
	IsComplete
=================================================
*/
	bool CurlRequestTask::IsComplete (CURLM* curlm, Duration_t responseTimeout)
	{
		EXLOCK( _drCheck );

		if ( _complete						or
			 _addToCurlmResult != CURLM_OK	or
			 _completionResult != CURLE_OK )
		{
			return true;
		}

		EStatus	status = Status();

		if ( (_curl != null) & (curlm != null) & (status == EStatus::Cancellation) )
		{
			_addToCurlmResult	= CURLM_LAST;
			_response->code		= ECode::RequestCanceled;

			CURLM_CALL( curl_multi_remove_handle( curlm, _curl ));
			return true;
		}

		if ( status > EStatus::_Finished )
			return true;
		
		if ( auto dt = (TimePoint_t::clock::now() - _lastResponseTime); dt > responseTimeout )
		{
			_response->code = ECode::TimeoutAfterLastResponse;
			AE_LOGI( "http request timed out" );
			return true;
		}
		
		return false;
	}

/*
=================================================
	OnCancel
=================================================
*/
	void CurlRequestTask::OnCancel ()
	{
		EXLOCK( _drCheck );
		
		ASSERT( _addToCurlmResult == CURLM_LAST );

		_Release();
	}

}	// AE::Networking
//-----------------------------------------------------------------------------



#include "stl/Platforms/PlatformUtils.h"

namespace AE::Networking
{
	//
	// CURL Thread
	//

	class CurlThread final : public IThread
	{
	// types
	public:
		using ThreadMask	= BitSet< uint(EThread::_Count) >;

		using Request		= SharedPtr< CurlRequestTask >;
		using Requests_t	= Array< Request >;

		using MilliSeconds	= RequestDesc::MilliSeconds;


	// variables
	private:
		std::thread				_thread;
		Atomic<uint>			_looping;
		Requests_t				_activeRequests;
		CURLM*					_curlm			= null;
		CURLSH*					_curlShared		= null;
		TimePoint_t				_lastTick;
		Settings_t const&		_settings;


	// methods
	public:
		CurlThread (CURLM* curlm, CURLSH* shared, const Settings_t &set) :
			_curlm{curlm}, _curlShared{shared}, _settings{set} {}

		bool  Attach (uint uid) override;
		void  Detach () override;

		NtStringView  DbgName () const override		{ return "network"; }

	private:
		void _Tick ();
	};
	
/*
=================================================
	Attach
=================================================
*/
	bool  CurlThread::Attach (uint uid)
	{
		_thread = std::thread{[this, uid] ()
		{
			uint	seed = uid;
			
			PlatformUtils::SetThreadName( DbgName() );
			AE_VTUNE( __itt_thread_set_name( DbgName().c_str() ));

			_looping.store( 1, memory_order_relaxed );
			for (; _looping.load( memory_order_relaxed );)
			{
				const auto	start_time	= TimePoint_t::clock::now();
				AsyncTask	task		= Scheduler().PullTask( EThread::Network, ++seed );
				
				if ( task )
				{
					if ( auto curl_task = DynCast<CurlRequestTask>(task) )
					{
						curl_task->Enque( _curlm, _curlShared );
						_activeRequests.push_back( curl_task );
					}
					else
					{
						_RunTask( task );
					}
				}
				
				_Tick();

				const auto	dt = (TimePoint_t::clock::now() - start_time);

				if ( dt < _settings.minFrameTime )
					std::this_thread::sleep_for( _settings.minFrameTime - dt );
			}
		}};
		return true;
	}
	
/*
=================================================
	_Tick
=================================================
*/
	void  CurlThread::_Tick ()
	{
		int	running_handles = -1;
		CURLM_CALL( curl_multi_perform( _curlm, OUT &running_handles ));
		
		const bool	changed	= running_handles != int(_activeRequests.size());

		// mark completed requests
		if ( running_handles == 0 or changed )
		{
			for (;;)
			{
				int			msgs_in_queue	= 0;
				CURLMsg*	msg				= curl_multi_info_read( _curlm, &msgs_in_queue );
			
				if ( not msg )
					break;
			
				if ( msg->msg != CURLMSG_DONE )
					continue;
			
				void*	ptr = null;
				CURL_CALL( curl_easy_getinfo( msg->easy_handle, CURLINFO_PRIVATE, OUT &ptr ));

				if ( not ptr )
				{
					ASSERT( ptr );
					continue;
				}

				Cast<CurlRequestTask>( ptr )->Complete( _curlm, msg->data.result );
			}
		}
		
		// remove completed requests
		const auto	time		= TimePoint_t::clock::now();
		const auto	max_delay	= _settings.responseDelay;

		if ( changed or (time - _lastTick > MilliSeconds(4)) )
		{
			_lastTick = time;

			for (auto iter = _activeRequests.begin(); iter != _activeRequests.end();)
			{
				if ( (*iter)->IsComplete( _curlm, max_delay ))
				{
					_OnTaskFinish( *iter );
					iter = _activeRequests.erase( iter );
				}
				else
					++iter;
			}
		}
	}

/*
=================================================
	Detach
=================================================
*/
	void  CurlThread::Detach ()
	{
		if ( _looping.exchange( 0, memory_order_relaxed ))
		{
			_thread.join();
		}
	}

}	// AE::Networking
//-----------------------------------------------------------------------------

	
namespace AE::Networking
{
	//
	// Internal implementation
	//

	class NetworkManager::_InternalImpl
	{
	// types
	public:
		using CurlThreadPtr = SharedPtr< CurlThread >;
		using Threads_t		= FixedArray< CurlThreadPtr, 8 >;

	// variables
	public:
		CURLM*			curlm		= null;
		CURLSH*			curlShared	= null;
		bool			hasSSL		= false;

		const Settings	settings;

		Threads_t		threads;

	// methods
	public:
		_InternalImpl (const Settings &settings) : settings{settings} {}
	};
	
/*
=================================================
	Instance
=================================================
*/
	NetworkManager&  NetworkManager::Instance ()
	{
		static NetworkManager	network;
		return network;
	}

/*
=================================================
	constructor
=================================================
*/
	NetworkManager::NetworkManager ()
	{
	}
	
/*
=================================================
	destructor
=================================================
*/
	NetworkManager::~NetworkManager ()
	{
		CHECK( not _impl );
	}

/*
=================================================
	Setup
=================================================
*/
	bool NetworkManager::Setup (const Settings &settings)
	{
		CHECK_ERR( not _impl );
		_impl.reset( new _InternalImpl{ settings });

		if ( curl_version_info_data* info = curl_version_info( CURLVERSION_NOW ))
		{
			AE_LOGI( "CURL version "s << info->version );

			_impl->hasSSL = info->features & (CURL_VERSION_MULTI_SSL | CURL_VERSION_SSL);

			if ( info->features & CURL_VERSION_MULTI_SSL )
			{
				const curl_ssl_backend**	available = null;
				curl_global_sslset( CURLSSLBACKEND_NONE, null, OUT &available );
				
				for (uint i = 0; available and available[i]; ++i)
				{
					if ( curl_global_sslset( available[i]->id, null, null ) == CURLSSLSET_OK )
					{
						AE_LOGI( "CURL supports SSL with '"s << available[i]->name << "' backend" );
						break;
					}
				}
			}
			else
			if ( info->features & CURL_VERSION_SSL )
			{
				AE_LOGI( "CURL supports SSL with '"s << info->ssl_version << "' backend" );
			}
			else
			{
				AE_LOGI( "CURL doesn't support SSL" );
			}
		}


		// init curl
		curl_global_init( CURL_GLOBAL_ALL );
		
		_impl->curlm = curl_multi_init();
		CHECK_ERR( _impl->curlm );
		
		_impl->curlShared = curl_share_init();
		CHECK_ERR( _impl->curlShared );

		CURLSH_CALL( curl_share_setopt( _impl->curlShared, CURLSHOPT_SHARE, CURL_LOCK_DATA_COOKIE ));
		CURLSH_CALL( curl_share_setopt( _impl->curlShared, CURLSHOPT_SHARE, CURL_LOCK_DATA_DNS ));
		CURLSH_CALL( curl_share_setopt( _impl->curlShared, CURLSHOPT_SHARE, CURL_LOCK_DATA_SSL_SESSION ));


		// start thread
		auto	ct = MakeShared<CurlThread>( _impl->curlm, _impl->curlShared, _impl->settings );

		_impl->threads.push_back( ct );
		Scheduler().AddThread( ct );

		return true;
	}
	
/*
=================================================
	Release
=================================================
*/
	bool NetworkManager::Release ()
	{
		CHECK_ERR( _impl );

		// wait for threads
		for (auto& thread : _impl->threads) {
			thread->Detach();
		}
		_impl->threads.clear();
		
		// cleanup curl
		if ( _impl->curlm )
		{
			CURLM_CALL( curl_multi_cleanup( _impl->curlm ));
			_impl->curlm = null;
		}

		if ( _impl->curlShared )
		{
			CURLSH_CALL( curl_share_cleanup( _impl->curlShared ));
			_impl->curlShared = null;
		}

		curl_global_cleanup();
		_impl.reset();

		return true;
	}
	
/*
=================================================
	HasSSL
=================================================
*/
	bool  NetworkManager::HasSSL () const
	{
		return _impl->hasSSL;
	}

/*
=================================================
	Send
=================================================
*/
	Request  NetworkManager::Send (RequestDesc &&desc, StrongDeps &&dependsOn)
	{
		CHECK_ERR( _impl );
		return Cast<Request::element_type>( Scheduler().Run<CurlRequestTask>( std::move(dependsOn), std::move(desc), _impl->settings ));
	}
	
	Request  NetworkManager::Send (const RequestDesc &desc, StrongDeps &&dependsOn)
	{
		CHECK_ERR( _impl );
		CHECK_ERR( not desc._content );	// can't move const value
		return Cast<Request::element_type>( Scheduler().Run<CurlRequestTask>( std::move(dependsOn), desc, _impl->settings ));
	}

/*
=================================================
	Send
=================================================
*/
	Request  NetworkManager::Send (RequestDesc &&desc, WeakDeps &&dependsOn)
	{
		CHECK_ERR( _impl );
		return Cast<Request::element_type>( Scheduler().Run<CurlRequestTask>( std::move(dependsOn), std::move(desc), _impl->settings ));
	}
	
	Request  NetworkManager::Send (const RequestDesc &desc, WeakDeps &&dependsOn)
	{
		CHECK_ERR( _impl );
		CHECK_ERR( not desc._content );	// can't move const value
		return Cast<Request::element_type>( Scheduler().Run<CurlRequestTask>( std::move(dependsOn), desc, _impl->settings ));
	}

/*
=================================================
	Download
=================================================
*/
	Promise<Array<uint8_t>>  NetworkManager::Download (String url)
	{
		CHECK( _impl );
		Request	req = Send( RequestDesc{ std::move(url) }.Method( RequestDesc::EMethod::Get ));

		return MakePromise(	[req] () -> PromiseResult<Array<uint8_t>>
							{
								auto	resp = req->Response();

								if ( resp->code == RequestTask::ECode::OK )
								{
									return std::move(resp->content);
								}
								return PromiseNullResult();
							},
							StrongDeps{req}
							//IAsyncTask::EThread::Network
						);
	}


}	// AE::Networking

#endif	// AE_ENABLE_CURL
