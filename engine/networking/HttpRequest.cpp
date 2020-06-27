// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "networking/HttpRequest.h"
#include "stl/Algorithms/StringUtils.h"

namespace AE::Networking
{
	
/*
=================================================
	Url
=================================================
*/
	HttpRequestDesc&  HttpRequestDesc::Url (String value) &
	{
		_url = std::move(value);
		return *this;
	}

	HttpRequestDesc&& HttpRequestDesc::Url (String value) &&
	{
		_url = std::move(value);
		return std::move(*this);
	}
		
/*
=================================================
	Method
=================================================
*/
	HttpRequestDesc&  HttpRequestDesc::Method (EMethod value) &
	{
		_method = value;
		return *this;
	}

	HttpRequestDesc&& HttpRequestDesc::Method (EMethod value) &&
	{
		_method = value;
		return std::move(*this);
	}
	
/*
=================================================
	Redirections
=================================================
*/
	HttpRequestDesc&  HttpRequestDesc::Redirections (uint value) &
	{
		_redirections = value;
		return *this;
	}

	HttpRequestDesc&& HttpRequestDesc::Redirections (uint value) &&
	{
		_redirections = value;
		return std::move(*this);
	}
	
/*
=================================================
	VerifyPeer
=================================================
*/
	HttpRequestDesc&  HttpRequestDesc::VerifyPeer (bool value) &
	{
		_verifyPeer = value;
		return *this;
	}

	HttpRequestDesc&& HttpRequestDesc::VerifyPeer (bool value) &&
	{
		_verifyPeer = value;
		return std::move(*this);
	}

/*
=================================================
	AddHeader
=================================================
*/
	HttpRequestDesc&  HttpRequestDesc::AddHeader (StringView name, StringView value) &
	{
		_headers.push_back( String(name) << ": " << value );
		return *this;
	}

	HttpRequestDesc&& HttpRequestDesc::AddHeader (StringView name, StringView value) &&
	{
		_headers.push_back( String(name) << ": " << value );
		return std::move(*this);
	}
	
/*
=================================================
	Content
=================================================
*/
	HttpRequestDesc&  HttpRequestDesc::Content (StringView value) &
	{
		_content = MakeUnique<MemRStream>( value );
		return *this;
	}

	HttpRequestDesc&& HttpRequestDesc::Content (StringView value) &&
	{
		_content = MakeUnique<MemRStream>( value );
		return std::move(*this);
	}
	
/*
=================================================
	Content
=================================================
*/
	HttpRequestDesc&  HttpRequestDesc::Content (Array<uint8_t> &&value) &
	{
		_content = MakeUnique<MemRStream>( std::move(value) );
		return *this;
	}

	HttpRequestDesc&& HttpRequestDesc::Content (Array<uint8_t> &&value) &&
	{
		_content = MakeUnique<MemRStream>( std::move(value) );
		return std::move(*this);
	}

/*
=================================================
	Content
=================================================
*/
	HttpRequestDesc&  HttpRequestDesc::Content (UniquePtr<RStream> value) &
	{
		_content = std::move(value);
		return *this;
	}

	HttpRequestDesc&& HttpRequestDesc::Content (UniquePtr<RStream> value) &&
	{
		_content = std::move(value);
		return std::move(*this);
	}
//-----------------------------------------------------------------------------


/*
=================================================
	constructor
=================================================
*/
	HttpRequest::HttpRequest () :
		IAsyncTask{ EThread::Network },
		_response{ new ResponseData{} },
		_bytesSent{ 0 },
		_bytesReceived{ 0 }
	{}
	
/*
=================================================
	Response
=================================================
*/
	HttpRequest::ResponsePtr  HttpRequest::Response ()
	{
		if ( not IsFinished() )
		{
			ASSERT( !"request is in progress" );
			return null;
		}

		return std::move(_response);
	}
	
/*
=================================================
	Sent
=================================================
*/
	BytesU  HttpRequest::Sent ()
	{
		return BytesU{ _bytesSent.load( EMemoryOrder::Relaxed )};
	}
	
/*
=================================================
	Received
=================================================
*/
	BytesU  HttpRequest::Received ()
	{
		return BytesU{ _bytesReceived.load( EMemoryOrder::Relaxed )};
	}

}	// AE::Networking
