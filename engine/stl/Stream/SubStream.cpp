// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Stream/SubStream.h"

namespace AE::STL
{

/*
=================================================
	constructor
=================================================
*/
	RSubStream::RSubStream (const SharedPtr<RStream> &stream, BytesU offset, BytesU size) :
		_stream{ stream }, _offset{ offset }, _size{ size }
	{}
		
/*
=================================================
	IsOpen
=================================================
*/
	bool  RSubStream::IsOpen () const
	{
		return _stream and _stream->IsOpen();
	}
	
/*
=================================================
	SeekSet
=================================================
*/
	bool  RSubStream::SeekSet (BytesU pos)
	{
		ASSERT( IsOpen() );

		if ( pos > _size )
			return false;

		if ( _stream and _stream->SeekSet( _offset + pos ))
		{
			_pos = pos;
			return true;
		}
		return false;
	}
	
/*
=================================================
	Read2
=================================================
*/
	BytesU  RSubStream::Read2 (OUT void *buffer, BytesU size)
	{
		ASSERT( IsOpen() );

		if ( _stream and _stream->SeekSet( _offset + _pos ))
		{
			BytesU	readn = _stream->Read2( buffer, size );
			_pos += readn;
			return readn;
		}
		else
			return 0_b;	// error or EOF
	}

}	// AE::STL
