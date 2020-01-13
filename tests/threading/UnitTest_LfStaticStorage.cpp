// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "threading/Containers/LfStaticStorage.h"
#include "UnitTest_Common.h"

namespace
{
	void LfStaticStorage_Test1 ()
	{
		LfStaticStorage< uint, 32 >		storage;

		for (uint i = 0; i < 20; ++i)
		{
			TEST( storage.Append( i ));
		}
		
		BitSet<20>	bits{ ~0u };
		for (uint i = 0; i < 20; ++i)
		{
			uint	val = 0;
			TEST( storage.Extract( OUT val ));

			TEST( bits[i] );
			bits.reset( i );
		}

		for (uint i = 0; i < 10; ++i)
		{
			uint	val = 0;
			TEST( not storage.Extract( OUT val ));
		}
	}

	
	void LfStaticStorage_Test2 ()
	{
		LfStaticStorage< uint, 4*64 >		storage;

		for (uint i = 0; i < 5*64; ++i)
		{
			if ( i < 4*64 )
				TEST( storage.Append( 0 ))
			else
				TEST( not storage.Append( 0 ));
		}

		for (uint i = 0; i < 5*64; ++i)
		{
			if ( i < 4*64 )
				TEST( storage.Remove( i ))
			else
				TEST( not storage.Remove( i ));
		}
	}
}


extern void UnitTest_LfStaticStorage ()
{
	LfStaticStorage_Test1();
	LfStaticStorage_Test2();

	AE_LOGI( "UnitTest_LfStaticStorage - passed" );
}
