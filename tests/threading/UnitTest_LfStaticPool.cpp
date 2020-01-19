// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "threading/Containers/LfStaticPool.h"
#include "../stl/UnitTest_Common.h"
using namespace AE::Threading;

namespace
{
	void LfStaticPool_Test1 ()
	{
		using T = DebugInstanceCounter< uint, 1 >;
	
		T::ClearStatistic();
		{
			LfStaticPool< T, 32 >		storage;

			for (uint i = 0; i < 20; ++i)
			{
				TEST( storage.Append( T(i) ));
			}
		
			BitSet<20>	bits{ ~0u };
			for (uint i = 0; i < 20; ++i)
			{
				T	val;
				TEST( storage.Extract( OUT val ));

				TEST( bits[val.value] );
				bits.reset( val.value );
			}

			for (uint i = 0; i < 10; ++i)
			{
				T	val;
				TEST( not storage.Extract( OUT val ));
			}
		}
		TEST( T::CheckStatistic() );
	}

	
	void LfStaticPool_Test2 ()
	{
		using T = DebugInstanceCounter< uint, 2 >;
	
		T::ClearStatistic();
		{
			LfStaticPool< T, 4*64 >		storage;

			for (uint i = 0; i < 5*64; ++i)
			{
				if ( i < 4*64 )
					TEST( storage.Append( T(0) ))
				else
					TEST( not storage.Append( T(1) ));
			}

			for (uint i = 0; i < 5*64; ++i)
			{
				T	val;

				if ( i < 4*64 )
					TEST( storage.Extract( OUT val ))
				else
					TEST( not storage.Extract( OUT val ));
			}
		}
		TEST( T::CheckStatistic() );
	}

	
	void LfStaticPool_Test3 ()
	{
		using T = DebugInstanceCounter< uint, 2 >;
	
		T::ClearStatistic();
		{
			LfStaticPool< T, 4*64 >		storage;

			for (uint i = 0; i < 5*64; ++i)
			{
				if ( i < 4*64 )
					TEST( storage.Append( T(0) ))
				else
					TEST( not storage.Append( T(1) ));
			}

			for (uint i = 0; i < 64; ++i)
			{
				T	val;
				TEST( storage.Extract( OUT val ));
			}

			storage.Clear();
		}
		TEST( T::CheckStatistic() );
	}
}


extern void UnitTest_LfStaticPool ()
{
	LfStaticPool_Test1();
	LfStaticPool_Test2();
	LfStaticPool_Test3();

	AE_LOGI( "UnitTest_LfStaticPool - passed" );
}
