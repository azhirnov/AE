// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "networking/Request.h"
#include "stl/Algorithms/StringUtils.h"

namespace AE::Networking
{
	
/*
=================================================
	Url
=================================================
*/
	RequestDesc&  RequestDesc::Url (String value) &
	{
		_url = std::move(value);
		return *this;
	}

	RequestDesc&& RequestDesc::Url (String value) &&
	{
		_url = std::move(value);
		return std::move(*this);
	}
		
/*
=================================================
	Method
=================================================
*/
	RequestDesc&  RequestDesc::Method (EMethod value) &
	{
		_method = value;
		return *this;
	}

	RequestDesc&& RequestDesc::Method (EMethod value) &&
	{
		_method = value;
		return std::move(*this);
	}
	
/*
=================================================
	Redirections
=================================================
*/
	RequestDesc&  RequestDesc::Redirections (uint value) &
	{
		_redirections = value;
		return *this;
	}

	RequestDesc&& RequestDesc::Redirections (uint value) &&
	{
		_redirections = value;
		return std::move(*this);
	}
	
/*
=================================================
	VerifyPeer
=================================================
*/
	RequestDesc&  RequestDesc::VerifyPeer (bool value) &
	{
		_verifyPeer = value;
		return *this;
	}

	RequestDesc&& RequestDesc::VerifyPeer (bool value) &&
	{
		_verifyPeer = value;
		return std::move(*this);
	}

/*
=================================================
	AddHeader
=================================================
*/
	RequestDesc&  RequestDesc::AddHeader (StringView name, StringView value) &
	{
		_headers.push_back( String(name) << ": " << value );
		return *this;
	}

	RequestDesc&& RequestDesc::AddHeader (StringView name, StringView value) &&
	{
		_headers.push_back( String(name) << ": " << value );
		return std::move(*this);
	}
	
/*
=================================================
	Content
=================================================
*/
	RequestDesc&  RequestDesc::Content (StringView value) &
	{
		_content = MakeUnique<MemRStream>( value );
		return *this;
	}

	RequestDesc&& RequestDesc::Content (StringView value) &&
	{
		_content = MakeUnique<MemRStream>( value );
		return std::move(*this);
	}
	
/*
=================================================
	Content
=================================================
*/
	RequestDesc&  RequestDesc::Content (Array<uint8_t> &&value) &
	{
		_content = MakeUnique<MemRStream>( std::move(value) );
		return *this;
	}

	RequestDesc&& RequestDesc::Content (Array<uint8_t> &&value) &&
	{
		_content = MakeUnique<MemRStream>( std::move(value) );
		return std::move(*this);
	}

/*
=================================================
	Content
=================================================
*/
	RequestDesc&  RequestDesc::Content (UniquePtr<RStream> value) &
	{
		_content = std::move(value);
		return *this;
	}

	RequestDesc&& RequestDesc::Content (UniquePtr<RStream> value) &&
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
	RequestTask::RequestTask () :
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
	RequestTask::ResponsePtr  RequestTask::Response ()
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
	BytesU  RequestTask::Sent ()
	{
		return BytesU{ _bytesSent.load( EMemoryOrder::Relaxed )};
	}
	
/*
=================================================
	Received
=================================================
*/
	BytesU  RequestTask::Received ()
	{
		return BytesU{ _bytesReceived.load( EMemoryOrder::Relaxed )};
	}

}	// AE::Networking
