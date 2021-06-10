// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Stream/MemStream.h"
#include "stl/Math/Vec.h"

namespace AE::STL
{
	
/*
=================================================
	constructor
=================================================
*/
	MemRStream::MemRStream (StringView data)
	{
		_data.assign( Cast<ubyte>(data.data()), Cast<ubyte>(data.data() + data.size()) );
	}

	MemRStream::MemRStream (ArrayView<ubyte> data) : _data{data}
	{}

	MemRStream::MemRStream (Array<ubyte> &&data) : _data{RVRef(data)}
	{}

/*
=================================================
	SeekSet
=================================================
*/
	bool  MemRStream::SeekSet (Bytes pos)
	{
		_pos = Min( pos, Size() );
		return _pos == pos;
	}
	
/*
=================================================
	Read2
=================================================
*/
	Bytes  MemRStream::Read2 (OUT void *buffer, Bytes size)
	{
		size = Min( size, Size() - _pos );

		MemCopy( OUT buffer, _data.data() + _pos, size );
		_pos += size;

		return size;
	}
	
/*
=================================================
	Decompress
=================================================
*/
	bool  MemRStream::Decompress (RStream &file)
	{
		CHECK_ERR( file.IsOpen() );

		ubyte		buf [1 << 14];

		_pos = 0_b;
		_data.clear();
		_data.reserve( sizeof(buf) * 2 );

		for (;;)
		{
			const Bytes	readn = file.Read2( buf, Bytes::SizeOf(buf) );

			if ( readn == 0_b )
				break;

			const usize	old_size = _data.size();

			_data.resize( old_size + usize(readn) );

			MemCopy( _data.data() + Bytes{old_size}, buf, readn );
		}
		return true;
	}
//-----------------------------------------------------------------------------


	
/*
=================================================
	SeekSet
=================================================
*/
	bool  MemWStream::SeekSet (Bytes pos)
	{
		if ( pos > Bytes{_data.size()} )
			return false;

		_pos = pos;
		return true;
	}
	
/*
=================================================
	Write2
=================================================
*/
	Bytes  MemWStream::Write2 (const void *buffer, Bytes size)
	{
		_data.resize( Max( _data.size(), usize(_pos + size) ));
		MemCopy( OUT _data.data() + _pos, buffer, size );

		_pos += size;
		return size;
	}
	
/*
=================================================
	Clear
=================================================
*/
	void  MemWStream::Clear ()
	{
		_pos = 0_b;
		_data.clear();
	}

}	// AE::STL
