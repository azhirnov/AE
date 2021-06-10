// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#include "networking/http/HttpRequest.h"
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
		_url = RVRef(value);
		return *this;
	}

	HttpRequestDesc&& HttpRequestDesc::Url (String value) &&
	{
		_url = RVRef(value);
		return RVRef(*this);
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
		return RVRef(*this);
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
		return RVRef(*this);
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
		return RVRef(*this);
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
		return RVRef(*this);
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
		return RVRef(*this);
	}
	
/*
=================================================
	Content
=================================================
*/
	HttpRequestDesc&  HttpRequestDesc::Content (Array<ubyte> &&value) &
	{
		_content = MakeUnique<MemRStream>( RVRef(value) );
		return *this;
	}

	HttpRequestDesc&& HttpRequestDesc::Content (Array<ubyte> &&value) &&
	{
		_content = MakeUnique<MemRStream>( RVRef(value) );
		return RVRef(*this);
	}

/*
=================================================
	Content
=================================================
*/
	HttpRequestDesc&  HttpRequestDesc::Content (UniquePtr<RStream> value) &
	{
		_content = RVRef(value);
		return *this;
	}

	HttpRequestDesc&& HttpRequestDesc::Content (UniquePtr<RStream> value) &&
	{
		_content = RVRef(value);
		return RVRef(*this);
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

		return RVRef(_response);
	}
	
/*
=================================================
	Sent
=================================================
*/
	Bytes  HttpRequest::Sent ()
	{
		return Bytes{ _bytesSent.load( EMemoryOrder::Relaxed )};
	}
	
/*
=================================================
	Received
=================================================
*/
	Bytes  HttpRequest::Received ()
	{
		return Bytes{ _bytesReceived.load( EMemoryOrder::Relaxed )};
	}

}	// AE::Networking
