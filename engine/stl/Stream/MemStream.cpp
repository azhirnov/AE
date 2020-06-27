// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Stream/MemStream.h"

namespace AE::STL
{
	
/*
=================================================
	constructor
=================================================
*/
	MemRStream::MemRStream (StringView data)
	{
		_data.assign( Cast<uint8_t>(data.data()), Cast<uint8_t>(data.data() + data.size()) );
	}

	MemRStream::MemRStream (ArrayView<uint8_t> data) : _data{data}
	{}

	MemRStream::MemRStream (Array<uint8_t> &&data) : _data{std::move(data)}
	{}

/*
=================================================
	SeekSet
=================================================
*/
	bool  MemRStream::SeekSet (BytesU pos)
	{
		_pos = Min( pos, Size() );
		return _pos == pos;
	}
	
/*
=================================================
	Read2
=================================================
*/
	BytesU  MemRStream::Read2 (OUT void *buffer, BytesU size)
	{
		size = Min( size, Size() - _pos );

		std::memcpy( OUT buffer, _data.data() + _pos, size_t(size) );
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

		uint8_t		buf [1 << 14];

		_pos = 0_b;
		_data.clear();
		_data.reserve( sizeof(buf) * 2 );

		for (;;)
		{
			const BytesU	readn = file.Read2( buf, BytesU::SizeOf(buf) );

			if ( readn == 0_b )
				break;

			const size_t	old_size = _data.size();

			_data.resize( old_size + size_t(readn) );

			std::memcpy( _data.data() + BytesU{old_size}, buf, size_t(readn) );
		}
		return true;
	}
//-----------------------------------------------------------------------------


	
/*
=================================================
	SeekSet
=================================================
*/
	bool  MemWStream::SeekSet (BytesU pos)
	{
		if ( pos > BytesU{_data.size()} )
			return false;

		_pos = pos;
		return true;
	}
	
/*
=================================================
	Write2
=================================================
*/
	BytesU  MemWStream::Write2 (const void *buffer, BytesU size)
	{
		_data.resize( Max( _data.size(), size_t(_pos + size) ));
		std::memcpy( OUT _data.data() + _pos, buffer, size_t(size) );

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
