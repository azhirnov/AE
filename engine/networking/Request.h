// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "threading/TaskSystem/TaskScheduler.h"
#include "stl/Stream/MemStream.h"

namespace AE::Networking
{
	using namespace AE::Threading;



	//
	// Request description
	//

	struct RequestDesc
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

		using MilliSeconds	= std::chrono::duration<uint, std::milli>;


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
		RequestDesc () {}
		explicit RequestDesc (String url) : _url{ std::move(url) } {}
		
		RequestDesc&  Url (String value) &;
		RequestDesc&& Url (String value) &&;
		
		RequestDesc&  Method (EMethod value) &;
		RequestDesc&& Method (EMethod value) &&;

		RequestDesc&  Redirections (uint value) &;
		RequestDesc&& Redirections (uint value) &&;
		
		RequestDesc&  VerifyPeer (bool value) &;
		RequestDesc&& VerifyPeer (bool value) &&;

		RequestDesc&  AddHeader (StringView name, StringView value) &;
		RequestDesc&& AddHeader (StringView name, StringView value) &&;

		// creates stream internally
		template <typename T> RequestDesc&  Content (ArrayView<T> value) &;
		template <typename T> RequestDesc&& Content (ArrayView<T> value) &&;
		RequestDesc&  Content (Array<uint8_t> &&value) &;
		RequestDesc&& Content (Array<uint8_t> &&value) &&;
		RequestDesc&  Content (StringView value) &;
		RequestDesc&& Content (StringView value) &&;

		// prefered
		RequestDesc&  Content (UniquePtr<RStream> value) &;
		RequestDesc&& Content (UniquePtr<RStream> value) &&;
	};


	using Request	= SharedPtr< class RequestTask >;


	//
	// Request Task
	//

	class RequestTask : public IAsyncTask
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
			NoContent					= 204,
			MovedPermanently			= 301,
			Found						= 302,
			BadRequest					= 400,
			MethodNotAllowed			= 405,
			RequestTimeout				= 408,
		};

		using Headers_t		= HashMap< String, String >;
		
		struct ResponseData
		{
			ECode			code		= ECode::Unknown;
			Headers_t		headers;
			Array<uint8_t>	content;
		};
		using ResponsePtr	= UniquePtr< ResponseData >;


	// variables
	protected:
		ResponsePtr			_response;
		Atomic<size_t>		_bytesSent;
		Atomic<size_t>		_bytesReceived;


	// methods
	public:
		RequestTask ();
		
		ND_ ResponsePtr  Response ();
		
		ND_ BytesU		Sent ();
		ND_ BytesU		Received ();
	};

	
	
/*
=================================================
	Content
=================================================
*/
	template <typename T>
	inline RequestDesc&  RequestDesc::Content (ArrayView<T> value) &
	{
		_content = MakeUnique<MemRStream>( value );
		return *this;
	}
	
	template <typename T>
	inline RequestDesc&&  RequestDesc::Content (ArrayView<T> value) &&
	{
		_content = MakeUnique<MemRStream>( value );
		return std::move(*this);
	}

}	// AE::Networking
