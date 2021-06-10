// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#ifdef AE_ENABLE_BROTLI

# include "stl/Stream/BrotliStream.h"
# include "stl/Math/Vec.h"
# include "brotli/decode.h"
# include "brotli/encode.h"

namespace AE::STL
{

/*
=================================================
	constructor
=================================================
*/
	BrotliRStream::BrotliRStream (const RC<RStream> &stream) :
		_stream{ RVRef(stream) },
		_position{ _stream->Position() },
		_lastResult{ BROTLI_DECODER_RESULT_NEEDS_MORE_INPUT }
	{
		_instance = BrotliDecoderCreateInstance( null, null, null );
		
		_buffer.resize( _BufferSize );
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
	bool  BrotliRStream::SeekSet (Bytes pos)
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
	Bytes  BrotliRStream::Read2 (OUT void *buffer, Bytes size)
	{
		ASSERT( IsOpen() );
		ASSERT( _position == _stream->Position() );

		BrotliDecoderResult	result			= BrotliDecoderResult(_lastResult);
		Bytes				written;
		const Bytes		stream_size		= _stream->Size();
		
		// decompress next part
		if ( result == BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT )
		{
			usize			available_out	= usize(size);
			ubyte*		next_out		= static_cast<ubyte *>( buffer );
			usize			available_in	= 0;
			ubyte const*	next_in			= null;

			result = BrotliDecoderDecompressStream( static_cast<BrotliDecoderState *>(_instance), INOUT &available_in, INOUT &next_in, 
													INOUT &available_out, INOUT &next_out, null );
			
			written += Bytes(next_out - static_cast<ubyte *>(buffer));
		}
		
		// read next part and decompress
		for (; (result == BROTLI_DECODER_RESULT_NEEDS_MORE_INPUT) and (written < size);)
		{
			usize			available_out	= usize(size - written);
			ubyte*		next_out		= static_cast<ubyte *>( buffer + written );
			usize			available_in	= usize(_stream->Read2( _buffer.data(), Bytes(_buffer.size()) ));
			ubyte const*	next_in			= _buffer.data();
			
			if ( available_in == 0 )
				return written;

			_position += available_in;

			result = BrotliDecoderDecompressStream( static_cast<BrotliDecoderState *>(_instance), INOUT &available_in, INOUT &next_in, 
													INOUT &available_out, INOUT &next_out, null );
			
			written += Bytes(next_out - static_cast<ubyte *>(buffer));
		}

		switch ( result )
		{
			case BROTLI_DECODER_RESULT_SUCCESS :
			case BROTLI_DECODER_RESULT_NEEDS_MORE_INPUT :
				_lastResult = uint(BROTLI_DECODER_RESULT_NEEDS_MORE_INPUT);
				return written;

			case BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT :
				_lastResult = uint(result);
				return written;

			case BROTLI_DECODER_RESULT_ERROR :		// not supported
			default :
				ASSERT(false);
				return written;
		}
	}
	
/*
=================================================
	Size
=================================================
*/
	Bytes  BrotliRStream::Size () const
	{
		ASSERT( !"not supported for compressed file" );
		return ~0_b;
	}
//-----------------------------------------------------------------------------
	
	
/*
=================================================
	constructor
=================================================
*/
	BrotliWStream::BrotliWStream (const RC<WStream> &stream, const Config &cfg) :
		_stream{ RVRef(stream) }
	{
		_instance = BrotliEncoderCreateInstance( null, null, null );

		if ( _instance )
		{
			BrotliEncoderSetParameter( static_cast<BrotliEncoderState *>(_instance),
									   BROTLI_PARAM_QUALITY,
									   uint( Lerp( float(BROTLI_MIN_QUALITY), float(BROTLI_MAX_QUALITY), Clamp( cfg.quality, 0.0f, 1.0f )) + 0.5f ));
		}

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
	Bytes	BrotliWStream::Write2 (const void *buffer, Bytes size)
	{
		ASSERT( IsOpen() );
		ASSERT( not BrotliEncoderIsFinished( static_cast<BrotliEncoderState *>(_instance) ));

		usize			available_in	= usize(size);
		ubyte const*	next_in			= static_cast<ubyte const *>(buffer);
		usize			available_out	= _buffer.size();
		ubyte*		next_out		= _buffer.data();

		CHECK_ERR( BrotliEncoderCompressStream( static_cast<BrotliEncoderState *>(_instance), BROTLI_OPERATION_PROCESS,
											    INOUT &available_in, INOUT &next_in,
											    INOUT &available_out, INOUT &next_out, null ));
		ASSERT( available_in == 0 );	// out: amount of unused input - should be zero
		
		const Bytes	out_size = Bytes(next_out - _buffer.data());

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
		
		usize		available_in	= 0;
		usize		available_out	= _buffer.size();
		ubyte*	next_out		= _buffer.data();

		CHECK_ERR( BrotliEncoderCompressStream( static_cast<BrotliEncoderState *>(_instance), BROTLI_OPERATION_FINISH,
												INOUT &available_in, null,
												INOUT &available_out, INOUT &next_out, null ));

		const Bytes	out_size = Bytes(next_out - _buffer.data());

		if ( out_size > 0 )
			CHECK_ERR( _stream->Write( _buffer.data(), out_size ));

		return true;
	}


}	// AE::STL

#endif	// AE_ENABLE_BROTLI
