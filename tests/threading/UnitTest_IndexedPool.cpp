// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "threading/Containers/IndexedPool.h"
#include "threading/Containers/CachedIndexedPool.h"
#include "../stl/UnitTest_Common.h"
using namespace AE::Threading;


namespace
{
	void IndexedPool_Test1 ()
	{
		constexpr uint							count = 1024;
		IndexedPool< int, uint, count/16, 16 >	pool;
	
		for (size_t i = 0; i < count; ++i)
		{
			uint	idx;
			TEST( pool.Assign( OUT idx ) == (i < count) );
			TEST( (idx == i) == (i < count) );
		}
	
		for (size_t i = 0; i < count; ++i)
		{
			pool.Unassign( uint(i) );
		}
	}


	void IndexedPool_Test2 ()
	{
		constexpr uint							count = 1024;
		IndexedPool< int, uint, count/16, 16 >	pool;
		FixedArray< uint, 16 >					arr;

		for (size_t i = 0; i < count; i += arr.capacity())
		{
			TEST( pool.Assign( UMax, INOUT arr ) > 0 );
			TEST( arr.size() == arr.capacity() );
			pool.Unassign( UMax, INOUT arr );
		}
	}


	void IndexedPool_Test3 ()
	{
		using T = DebugInstanceCounter< int, 1 >;
	
		T::ClearStatistic();
		{
			constexpr uint							count = 1024;
			IndexedPool< T, uint, count/16, 16 >	pool;
	
			for (size_t i = 0; i < count; ++i)
			{
				uint	idx;
				TEST( pool.Assign( OUT idx ) == (i < count) );
				TEST( (idx == i) == (i < count) );
			}
	
			for (size_t i = 0; i < count; ++i)
			{
				pool.Unassign( uint(i) );
			}
		}
		TEST( T::CheckStatistic() );
	}


	void CachedIndexedPool_Test1 ()
	{
		CachedIndexedPool<uint, uint, 16, 16>	pool;

		uint	idx1, idx2;

		TEST( pool.Assign( OUT idx1 ));
		TEST( pool.Assign( OUT idx2 ));

		TEST( pool.Insert( idx1, 2 ).second );
		TEST( not pool.Insert( idx2, 2 ).second );
	
		pool.RemoveFromCache( idx2 );
		pool.RemoveFromCache( idx1 );
		pool.Unassign( idx2 );
		pool.Unassign( idx1 );
	}
}


extern void UnitTest_IndexedPool ()
{
	IndexedPool_Test1();
	IndexedPool_Test2();
	IndexedPool_Test3();
	CachedIndexedPool_Test1();

	AE_LOGI( "UnitTest_IndexedPool - passed" );
}
