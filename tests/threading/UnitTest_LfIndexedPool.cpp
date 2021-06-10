// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#include "threading/Containers/LfIndexedPool.h"
#include "../stl/UnitTest_Common.h"
using namespace AE::Threading;

namespace
{
	static void  LfIndexedPool_Test1 ()
	{
		using T = DebugInstanceCounter< int, 1 >;
	
		T::ClearStatistic();
		{
			LfIndexedPool< T, uint, 1024, 16 >	pool;
	
			for (uint i = 0; i < 1024*16*10; ++i)
			{
				uint	index;
				TEST( pool.Assign( OUT index ));
				TEST( pool.IsAssigned( index ));

				TEST( pool.Unassign( index ));
				TEST( not pool.IsAssigned( index ));
			}
		}
		TEST( T::CheckStatistic() );
	}


	static void  LfIndexedPool_Test2 ()
	{
		using T = DebugInstanceCounter< int, 2 >;
	
		T::ClearStatistic();
		{
			constexpr uint							count = 1024*16;
			LfIndexedPool< T, uint, count/16, 16 >	pool;
	
			for (uint i = 0; i < count+1; ++i)
			{
				uint	index;
				bool	res = pool.Assign( OUT index );

				if ( i < count )
				{
					TEST( res );
					TEST( pool.IsAssigned( index ));
				}
				else
					TEST( not res );
			}
		
			for (uint i = 0; i < count; ++i)
			{
				TEST( pool.IsAssigned( i ));
				TEST( pool.Unassign( i ));
				TEST( not pool.IsAssigned( i ));
			}
		}
		TEST( T::CheckStatistic() );
	}
}


extern void UnitTest_LfIndexedPool ()
{
	LfIndexedPool_Test1();
	LfIndexedPool_Test2();

	AE_LOGI( "UnitTest_LfIndexedPool - passed" );
}
