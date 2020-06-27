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
		SharedPtr<RStream>		_stream;
		void *					_instance	= null;		// BrotliDecoderState
		Array<uint8_t>			_buffer;
		BytesU					_position;				// uncompressed size
		uint					_lastResult;

		static const size_t		_BufferSize	= 4u << 10;


	// methods
	public:
		explicit BrotliRStream (const SharedPtr<RStream> &stream);
		~BrotliRStream () override;

		bool	IsOpen ()	const override		{ return _instance and _stream and _stream->IsOpen(); }
		BytesU	Position ()	const override		{ return _position; }
		BytesU	Size ()		const override;
		
		bool	SeekSet (BytesU pos) override;
		BytesU	Read2 (OUT void *buffer, BytesU size) override;
	};


	
	//
	// Write-only Brotli Compression Stream
	//

	class BrotliWStream final : public WStream
	{
	// types
	public:
		struct Config
		{
			float	quality = 0.5f;
		};


	// variables
	private:
		SharedPtr<WStream>		_stream;
		void *					_instance	= null;		// BrotliEncoderState
		Array<uint8_t>			_buffer;
		BytesU					_position;				// uncompressed size

		static const size_t		_BufferSize	= 4u << 10;


	// methods
	public:
		explicit BrotliWStream (const SharedPtr<WStream> &stream, const Config &cfg = Default);
		~BrotliWStream () override;
		
		bool	IsOpen ()	const override		{ return _instance and _stream and _stream->IsOpen(); }
		BytesU	Position ()	const override		{ return _position; }
		BytesU	Size ()		const override		{ return Position(); }
		
		bool	SeekSet (BytesU) override		{ return false; }
		BytesU	Write2 (const void *buffer, BytesU size) override;
		void	Flush () override				{ _Flush(); }

	private:
		bool _Flush ();
	};


}	// AE::STL

#endif	// AE_ENABLE_BROTLI
