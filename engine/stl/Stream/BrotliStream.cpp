// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#ifdef AE_ENABLE_BROTLI

# include "stl/stream/BrotliStream.h"
# include "brotli/decode.h"
# include "brotli/encode.h"

namespace AE::STL
{

/*
=================================================
	constructor
=================================================
*/
	BrotliRStream::BrotliRStream (UniquePtr<RStream> &&stream) :
		_stream{ std::move(stream) }
	{
		_instance = BrotliDecoderCreateInstance( null, null, null );

		_buffer.reserve( _BufferSize );
	}
	
/*
=================================================
	destructor
=================================================
*/
	BrotliRStream::~BrotliRStream ()
	{
		if ( _instance )
			BrotliDecoderDestroyInstance( static_cast<BrotliDecoderState *>(_instance) );
	}
	
/*
=================================================
	SeekSet
=================================================
*/
	bool  BrotliRStream::SeekSet (BytesU pos)
	{
		if ( pos == _position )
		{
			ASSERT( _position == _stream->Position() );
			return true;
		}

		// not supported, yet
		return false;
	}
	
/*
=================================================
	Read2
=================================================
*/
	BytesU  BrotliRStream::Read2 (OUT void *buffer, BytesU size)
	{
		ASSERT( IsOpen() );
		ASSERT( not BrotliDecoderIsFinished( static_cast<BrotliDecoderState *>(_instance) ));
		ASSERT( _position == _stream->Position() );

		size_t				available_out	= size_t(size);
		uint8_t*			next_out		= static_cast<uint8_t *>( buffer );
		BrotliDecoderResult	result			= BROTLI_DECODER_RESULT_NEEDS_MORE_INPUT;
		BytesU				written;
		const BytesU		stream_size		= _stream->Size();

		for (;;)
		{
			switch ( result ) {
				case BROTLI_DECODER_RESULT_NEEDS_MORE_INPUT :
					break;		// read next part and decompress

				case BROTLI_DECODER_RESULT_SUCCESS :
					return written;

				case BROTLI_DECODER_RESULT_ERROR :
				case BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT :		// not supported
				default :
					ASSERT(false);
					return written;
			}

			_buffer.resize( Min( size_t(stream_size - _position), _buffer.capacity() ));

			size_t			available_in	= size_t(_stream->Read2( _buffer.data(), BytesU(_buffer.size()) ));
			uint8_t const*	next_in			= _buffer.data();
			
			if ( available_in == 0 )
				return written;

			_position += available_in;

			result = BrotliDecoderDecompressStream( static_cast<BrotliDecoderState *>(_instance), INOUT &available_in, INOUT &next_in, 
													INOUT &available_out, INOUT &next_out, null );
			
			written += BytesU(next_out - static_cast<uint8_t *>(buffer));
		}
	}
//-----------------------------------------------------------------------------
	
	
/*
=================================================
	constructor
=================================================
*/
	BrotliWStream::BrotliWStream (UniquePtr<WStream> &&stream) :
		_stream{ std::move(stream) }
	{
		_instance = BrotliEncoderCreateInstance( null, null, null );

		_buffer.resize( _BufferSize );
	}

/*
=================================================
	destructor
=================================================
*/
	BrotliWStream::~BrotliWStream ()
	{
		if ( _instance )
		{
			CHECK( _Flush() );
			BrotliEncoderDestroyInstance( static_cast<BrotliEncoderState *>(_instance) );
		}
	}

/*
=================================================
	Write2
=================================================
*/
	BytesU	BrotliWStream::Write2 (const void *buffer, BytesU size)
	{
		ASSERT( IsOpen() );
		ASSERT( not BrotliEncoderIsFinished( static_cast<BrotliEncoderState *>(_instance) ));

		size_t			available_in	= size_t(size);
		const uint8_t*	next_in			= static_cast<uint8_t const *>(buffer);
		size_t			available_out	= _buffer.size();
		uint8_t*		next_out		= _buffer.data();

		CHECK_ERR( BrotliEncoderCompressStream( static_cast<BrotliEncoderState *>(_instance), BROTLI_OPERATION_PROCESS,
											    INOUT &available_in, INOUT &next_in,
											    INOUT &available_out, INOUT &next_out, null ));
		ASSERT( available_in == 0 );
		
		const BytesU	out_size = BytesU(next_out - _buffer.data());

		if ( out_size > 0 )
			CHECK_ERR( _stream->Write( _buffer.data(), out_size ));

		_position += size;
		return size;
	}
	
/*
=================================================
	_Flush
=================================================
*/
	bool  BrotliWStream::_Flush ()
	{
		ASSERT( IsOpen() );

		if ( BrotliEncoderIsFinished( static_cast<BrotliEncoderState *>(_instance) ))
			return true;
		
		size_t		available_in	= 0;
		size_t		available_out	= _buffer.size();
		uint8_t*	next_out		= _buffer.data();

		CHECK_ERR( BrotliEncoderCompressStream( static_cast<BrotliEncoderState *>(_instance), BROTLI_OPERATION_FINISH,
												INOUT &available_in, null,
												INOUT &available_out, INOUT &next_out, null ));

		const BytesU	out_size = BytesU(next_out - _buffer.data());

		if ( out_size > 0 )
			CHECK_ERR( _stream->Write( _buffer.data(), out_size ));

		return true;
	}


}	// AE::STL

#endif	// AE_ENABLE_BROTLI
