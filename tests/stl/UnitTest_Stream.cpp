// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Stream/StdStream.h"
#include "stl/Stream/MemStream.h"
#include "stl/Stream/BrotliStream.h"
#include "UnitTest_Common.h"


namespace
{
#ifdef AE_ENABLE_BROTLI
	static void  BrotliStream_Test1 ()
	{
		const String	str1 = "12i12ienmqpwom12euj1029podmksjhjbjcnalsmoiiwujkcmsalsc,posaasjncsalkmxaz";

		Array<ubyte>	file_data;

		// compress
		{
			auto			stream = MakeRC<MemWStream>();
			BrotliWStream	encoder{ stream };

			TEST( encoder.IsOpen() );
			TEST( encoder.Write( str1 ));
			encoder.Flush();

			file_data.assign( stream->GetData().begin(), stream->GetData().end() );
		}

		// uncompress
		{
			BrotliRStream	decoder{ MakeRC<MemRStream>( file_data )};
			String			str2, str3;

			TEST( decoder.IsOpen() );
			TEST( decoder.Read( str1.length() / 2, OUT str2 ));
			TEST( decoder.Read( str1.length() - str2.length(), OUT str3 ));

			TEST( str1 == (str2 + str3) );
		}
	}
#endif	// AE_ENABLE_BROTLI


	static void  StdStream_Test1 ()
	{
		String			src_str = "4324356uytsdgfh1243rttrasfdg";
		RC<RStream>		mem = MakeRC<MemRStream>( src_str );

		StreambufWrap<char>	streambuf{ mem };
		std::istream		stream{ &streambuf };

		String				dst_str;
		dst_str.resize( src_str.length() );

		stream.read( dst_str.data(), dst_str.length() );
		TEST( src_str == dst_str );

		// read again
		dst_str.clear();
		dst_str.resize( src_str.length() );

		stream.seekg( 0, std::ios_base::beg );
		stream.read( dst_str.data(), dst_str.length() );
		TEST( src_str == dst_str );
	}
}


extern void UnitTest_Stream ()
{
	#ifdef AE_ENABLE_BROTLI
	BrotliStream_Test1();
	#endif

	StdStream_Test1();

	AE_LOGI( "UnitTest_Stream - passed" );
}
