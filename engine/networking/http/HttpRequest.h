// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'
/*
	Get		- The HTTP GET method is used to **read** (or retrieve) a representation of a resource.
			  GET requests are used only to read data and not change it.

	Put		- PUT is most-often utilized for **update** capabilities, PUT-ing to a known resource URI
			  with the request body containing the newly-updated representation of the original resource.

	Post	- The POST verb is most-often utilized to **create** new resources. In particular,
			  it's used to create subordinate resources.

	Delete	- DELETE is pretty easy to understand. It is used to **delete** a resource identified by a URI.

	https://www.restapitutorial.com/lessons/httpmethods.html
	https://www.restapitutorial.com/httpstatuscodes.html
*/

#pragma once

#include "threading/TaskSystem/TaskScheduler.h"
#include "stl/Stream/MemStream.h"

namespace AE::Networking
{
	using namespace AE::Threading;



	//
	// Http Request description
	//

	struct HttpRequestDesc
	{
	// types
	public:
		enum class EMethod : uint
		{
			Get,
			Put,
			Post,
			Patch,
			Head,
			Delete
		};


	// variables
	public:
		UniquePtr<RStream>		_content;
		String					_url;
		EMethod					_method				= EMethod::Get;
		uint					_redirections		= UMax;
		Array<String>			_headers;
		bool					_verifyPeer			= false;


	// methods
	public:
		HttpRequestDesc () {}
		explicit HttpRequestDesc (String url) : _url{ RVRef(url) } {}
		
		HttpRequestDesc&  Url (String value) &;
		HttpRequestDesc&& Url (String value) &&;
		
		HttpRequestDesc&  Method (EMethod value) &;
		HttpRequestDesc&& Method (EMethod value) &&;

		HttpRequestDesc&  Redirections (uint value) &;
		HttpRequestDesc&& Redirections (uint value) &&;
		
		HttpRequestDesc&  VerifyPeer (bool value) &;
		HttpRequestDesc&& VerifyPeer (bool value) &&;

		HttpRequestDesc&  AddHeader (StringView name, StringView value) &;
		HttpRequestDesc&& AddHeader (StringView name, StringView value) &&;

		// creates stream internally
		template <typename T> HttpRequestDesc&  Content (ArrayView<T> value) &;
		template <typename T> HttpRequestDesc&& Content (ArrayView<T> value) &&;
		HttpRequestDesc&  Content (Array<ubyte> &&value) &;
		HttpRequestDesc&& Content (Array<ubyte> &&value) &&;
		HttpRequestDesc&  Content (StringView value) &;
		HttpRequestDesc&& Content (StringView value) &&;

		// prefered
		HttpRequestDesc&  Content (UniquePtr<RStream> value) &;
		HttpRequestDesc&& Content (UniquePtr<RStream> value) &&;
	};


	using Request	= RC< class HttpRequest >;


	//
	// Http Request Task
	//

	class HttpRequest : public IAsyncTask
	{
	// types
	public:
		enum class ECode : uint
		{
			// non-http codes
			Unknown						= ~0u,
			OperationTimeout			= 1 << 16,
			ConnectionError				= 2 << 16,
			TimeoutAfterLastResponse	= 3 << 16,
			RequestCanceled				= 4 << 16,
			// http codes
			OK							= 200,
			Created						= 201,
			NonAuthoritativeInformation	= 203,
			NoContent					= 204,
			MovedPermanently			= 301,
			Found						= 302,		// redirection
			BadRequest					= 400,
			Unauthoried					= 401,
			Forbidden					= 403,
			NotFound					= 404,
			MethodNotAllowed			= 405,
			RequestTimeout				= 408,
			Conflict					= 409,
			InternalServerError			= 500,
			ServiceUnavailable			= 503,
			NetworkReadTimeoutError		= 598,
			NetworkConnectTimeoutError	= 599,
		};

		using Headers_t		= HashMap< String, String >;
		
		struct ResponseData
		{
			ECode			code		= ECode::Unknown;
			Headers_t		headers;
			Array<ubyte>	content;
		};
		using ResponsePtr	= UniquePtr< ResponseData >;


	// variables
	protected:
		ResponsePtr			_response;
		Atomic<usize>		_bytesSent;
		Atomic<usize>		_bytesReceived;


	// methods
	public:
		HttpRequest ();
		
		ND_ ResponsePtr  Response ();
		
		ND_ Bytes		Sent ();
		ND_ Bytes		Received ();
	};

	
	
/*
=================================================
	Content
=================================================
*/
	template <typename T>
	inline HttpRequestDesc&  HttpRequestDesc::Content (ArrayView<T> value) &
	{
		_content = MakeUnique<MemRStream>( value );
		return *this;
	}
	
	template <typename T>
	inline HttpRequestDesc&&  HttpRequestDesc::Content (ArrayView<T> value) &&
	{
		_content = MakeUnique<MemRStream>( value );
		return RVRef(*this);
	}

}	// AE::Networking
