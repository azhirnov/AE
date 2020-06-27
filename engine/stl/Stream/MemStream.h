// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Stream/Stream.h"

namespace AE::STL
{

	//
	// Memory read-only Stream
	//

	class MemRStream final : public RStream
	{
	// variables
	private:
		Array<uint8_t>		_data;
		BytesU				_pos;


	// methods
	public:
		MemRStream () {}
		~MemRStream () {}

		explicit MemRStream (StringView data);
		explicit MemRStream (ArrayView<uint8_t> data);
		explicit MemRStream (Array<uint8_t> &&data);
		
		bool	IsOpen ()	const override	{ return true; }
		BytesU	Position ()	const override	{ return _pos; }
		BytesU	Size ()		const override	{ return BytesU{_data.size()}; }

		bool	SeekSet (BytesU pos) override;
		BytesU	Read2 (OUT void *buffer, BytesU size) override;

		bool	Decompress (RStream &file);
	};



	//
	// Memory Write-only Stream
	//

	class MemWStream final : public WStream
	{
	// variables
	private:
		Array<uint8_t>		_data;
		BytesU				_pos;


	// methods
	public:
		MemWStream () {
			_data.reserve( 4u << 10 );
		}
		
		explicit MemWStream (BytesU bufferSize) {
			_data.reserve( size_t(bufferSize) );
		}

		~MemWStream () {}
		
		bool	IsOpen ()	const override		{ return true; }
		BytesU	Position ()	const override		{ return BytesU(_pos); }
		BytesU	Size ()		const override		{ return BytesU(_data.size()); }
		
		bool	SeekSet (BytesU pos) override;
		BytesU	Write2 (const void *buffer, BytesU size) override;

		void  Flush () override
		{
			// do nothing
		}

		ND_ ArrayView<uint8_t>  GetData () const	{ return _data; }

		void  Clear ();
	};


}	// AE::STL
