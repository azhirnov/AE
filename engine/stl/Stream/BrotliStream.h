// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef AE_ENABLE_BROTLI

#include "stl/Stream/Stream.h"

namespace AE::STL
{

	//
	// Read-only Brotli Uncompression Stream
	//

	class BrotliRStream final : public RStream
	{
	// variables
	private:
		UniquePtr<RStream>		_stream;
		void *					_instance	= null;		// BrotliDecoderState
		Array<uint8_t>			_buffer;
		BytesU					_position;

		static const size_t		_BufferSize	= 4u << 20;


	// methods
	public:
		explicit BrotliRStream (UniquePtr<RStream> &&);
		~BrotliRStream () override;

		bool	IsOpen ()	const override		{ return _instance and _stream and _stream->IsOpen(); }
		BytesU	Position ()	const override		{ return _position; }
		BytesU	Size ()		const override		{ return _stream ? _stream->Size() : 0_b; }
		
		bool	SeekSet (BytesU pos) override;
		BytesU	Read2 (OUT void *buffer, BytesU size) override;

	};


	
	//
	// Write-only Brotli Compression Stream
	//

	class BrotliWStream final : public WStream
	{
	// variables
	private:
		UniquePtr<WStream>		_stream;
		void *					_instance	= null;		// BrotliEncoderState
		Array<uint8_t>			_buffer;
		BytesU					_position;

		static const size_t		_BufferSize	= 4u << 20;


	// methods
	public:
		explicit BrotliWStream (UniquePtr<WStream> &&);
		~BrotliWStream () override;
		
		bool	IsOpen ()	const override		{ return _instance and _stream and _stream->IsOpen(); }
		BytesU	Position ()	const override		{ return _position; }
		BytesU	Size ()		const override		{ return Position(); }
		
		BytesU	Write2 (const void *buffer, BytesU size) override;
		void	Flush () override				{ _Flush(); }

	private:
		bool _Flush ();
	};


}	// AE::STL

#endif	// AE_ENABLE_BROTLI
