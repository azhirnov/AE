// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Stream/MemStream.h"
#include "stl/Stream/BrotliStream.h"
#include "UnitTest_Common.h"

namespace
{
	void BrotliStream_Test1 ()
	{
		const String	str1 = "12i12ienmqpwom12euj1029podmksjhjbjcnalsmoiiwujkcmsalsc,posaasjncsalkmxaz";

		Array<uint8_t>	file_data;

		// compress
		{
			auto*			stream = new MemWStream{};
			BrotliWStream	encoder{ UniquePtr<WStream>{stream} };

			TEST( encoder.IsOpen() );
			TEST( encoder.Write( str1 ));
			encoder.Flush();

			file_data.assign( stream->GetData().begin(), stream->GetData().end() );
		}

		// uncompress
		{
			BrotliRStream	decoder{ UniquePtr<RStream>{ new MemRStream{ file_data }} };
			String			str2;

			TEST( decoder.IsOpen() );
			TEST( decoder.Read( str1.length(), OUT str2 ));

			TEST( str1 == str2 );
		}
	}
}


extern void UnitTest_BrotliStream ()
{
	BrotliStream_Test1();

    AE_LOGI( "UnitTest_BrotliStream - passed" );
}
