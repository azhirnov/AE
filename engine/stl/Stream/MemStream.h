// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

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
		Array<ubyte>		_data;
		Bytes				_pos;


	// methods
	public:
		MemRStream () {}
		~MemRStream () {}

		explicit MemRStream (StringView data);
		explicit MemRStream (ArrayView<ubyte> data);
		explicit MemRStream (Array<ubyte> &&data);
		
		bool	IsOpen ()	const override	{ return true; }
		Bytes	Position ()	const override	{ return _pos; }
		Bytes	Size ()		const override	{ return Bytes{_data.size()}; }

		bool	SeekSet (Bytes pos) override;
		Bytes	Read2 (OUT void *buffer, Bytes size) override;

		bool	Decompress (RStream &file);
	};



	//
	// Memory Write-only Stream
	//

	class MemWStream final : public WStream
	{
	// variables
	private:
		Array<ubyte>		_data;
		Bytes				_pos;


	// methods
	public:
		MemWStream () {
			_data.reserve( 4u << 10 );
		}
		
		explicit MemWStream (Bytes bufferSize) {
			_data.reserve( usize(bufferSize) );
		}

		~MemWStream () {}
		
		bool	IsOpen ()	const override		{ return true; }
		Bytes	Position ()	const override		{ return Bytes(_pos); }
		Bytes	Size ()		const override		{ return Bytes(_data.size()); }
		
		bool	SeekSet (Bytes pos) override;
		Bytes	Write2 (const void *buffer, Bytes size) override;

		void  Flush () override
		{
			// do nothing
		}

		ND_ ArrayView<ubyte>  GetData () const	{ return _data; }

		void  Clear ();
	};


}	// AE::STL
