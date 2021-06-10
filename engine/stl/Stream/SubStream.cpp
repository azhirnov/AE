// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Stream/SubStream.h"

namespace AE::STL
{

/*
=================================================
	constructor
=================================================
*/
	RSubStream::RSubStream (const RC<RStream> &stream, Bytes offset, Bytes size) :
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
	bool  RSubStream::SeekSet (Bytes pos)
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
	Bytes  RSubStream::Read2 (OUT void *buffer, Bytes size)
	{
		ASSERT( IsOpen() );

		if ( _stream and _stream->SeekSet( _offset + _pos ))
		{
			Bytes	readn = _stream->Read2( buffer, size );
			_pos += readn;
			return readn;
		}
		else
			return 0_b;	// error or EOF
	}

}	// AE::STL
