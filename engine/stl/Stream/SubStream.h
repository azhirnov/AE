// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'
/*
	Warning: you can not simultaneously use 'sub-stream' that created from 'stream' and 'stream',
	because 'sub-stream' modifies offset in 'stream'.
*/

#pragma once

#include "stl/Stream/Stream.h"

namespace AE::STL
{

	//
	// Read-only sub stream
	//

	class RSubStream final : public RStream
	{
	// variables
	private:
		SharedPtr<RStream>	_stream;
		BytesU				_pos;
		const BytesU		_offset;
		const BytesU		_size;


	// methods
	public:
		RSubStream (const SharedPtr<RStream> &stream, BytesU offset, BytesU size);
		
		bool	IsOpen ()	const override;
		BytesU	Position ()	const override	{ return _pos; }
		BytesU	Size ()		const override	{ return _size; }

		bool	SeekSet (BytesU pos) override;
		BytesU	Read2 (OUT void *buffer, BytesU size) override;
	};


}	// AE::STL
