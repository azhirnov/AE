// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Containers/FixedTupleArray.h"
#include "UnitTest_Common.h"


namespace
{
	static void  FixedTupleArray_Test1 ()
	{
		using T1 = DebugInstanceCounter< int, 1 >;
		using T2 = DebugInstanceCounter< float, 1 >;

		T1::ClearStatistic();
		T2::ClearStatistic();
		{
			FixedTupleArray< 32, T1, T2 >	arr;

			arr.push_back( T1(1), T2(2.2f) );
			arr.push_back( T1(2), T2(3.3f) );
	
			TEST( arr.size() == 2 );
			TEST( arr.get<T1>()[0] == T1(1) );
			TEST( arr.get<T1>()[1] == T1(2) );
			TEST( arr.get<T2>()[0] == T2(2.2f) );
			TEST( arr.get<T2>()[1] == T2(3.3f) );

			arr.pop_back();
			TEST( arr.size() == 1 );
			TEST( arr.get<T1>()[0] == T1(1) );
			TEST( arr.get<T2>()[0] == T2(2.2f) );

			arr.clear();
			TEST( arr.size() == 0 );
		}
		TEST( T1::CheckStatistic() );
		TEST( T2::CheckStatistic() );
	}


	static void  FixedTupleArray_Test2 ()
	{
		using T1 = DebugInstanceCounter< int, 2 >;
		using T2 = DebugInstanceCounter< float, 2 >;

		T1::ClearStatistic();
		T2::ClearStatistic();
		{
			FixedTupleArray< 32, T1, T2 >	arr;

			arr.insert( 0, T1(1), T2(2.2f) );
			TEST( arr.size() == 1 );
			TEST( arr.get<T1>()[0] == T1(1) );
			TEST( arr.get<T2>()[0] == T2(2.2f) );
			
			arr.insert( 0, T1(2), T2(3.3f) );
			TEST( arr.size() == 2 );
			TEST( arr.get<T1>()[0] == T1(2) );
			TEST( arr.get<T1>()[1] == T1(1) );
			TEST( arr.get<T2>()[0] == T2(3.3f) );
			TEST( arr.get<T2>()[1] == T2(2.2f) );
	
			arr.insert( 3, T1(3), T2(4.4f) );
			TEST( arr.size() == 3 );
			TEST( arr.get<T1>()[0] == T1(2) );
			TEST( arr.get<T1>()[1] == T1(1) );
			TEST( arr.get<T1>()[2] == T1(3) );
			TEST( arr.get<T2>()[0] == T2(3.3f) );
			TEST( arr.get<T2>()[1] == T2(2.2f) );
			TEST( arr.get<T2>()[2] == T2(4.4f) );
	
			arr.insert( 2, T1(4), T2(5.5f) );
			TEST( arr.size() == 4 );
			TEST( arr.get<T1>()[0] == T1(2) );
			TEST( arr.get<T1>()[1] == T1(1) );
			TEST( arr.get<T1>()[2] == T1(4) );
			TEST( arr.get<T1>()[3] == T1(3) );
			TEST( arr.get<T2>()[0] == T2(3.3f) );
			TEST( arr.get<T2>()[1] == T2(2.2f) );
			TEST( arr.get<T2>()[2] == T2(5.5f) );
			TEST( arr.get<T2>()[3] == T2(4.4f) );

			arr.fast_erase( 0 );
			TEST( arr.size() == 3 );
			TEST( arr.get<T1>()[0] == T1(3) );
			TEST( arr.get<T1>()[1] == T1(1) );
			TEST( arr.get<T1>()[2] == T1(4) );
			TEST( arr.get<T2>()[0] == T2(4.4f) );
			TEST( arr.get<T2>()[1] == T2(2.2f) );
			TEST( arr.get<T2>()[2] == T2(5.5f) );
		}
		TEST( T1::CheckStatistic() );
		TEST( T2::CheckStatistic() );
	}


	static void  FixedTupleArray_Test3 ()
	{
		using T1 = DebugInstanceCounter< int, 3 >;
		using T2 = DebugInstanceCounter< int, 4 >;

		T1::ClearStatistic();
		T2::ClearStatistic();
		{
			FixedTupleArray< 32, T1, T2 >	arr;

			arr.resize( 2 );
			TEST( arr.size() == 2 );
			TEST( arr.get<T1>()[0] == T1() );
			TEST( arr.get<T1>()[1] == T1() );
			TEST( arr.get<T2>()[0] == T2() );
			TEST( arr.get<T2>()[1] == T2() );

		}
		TEST( T1::CheckStatistic() );
		TEST( T2::CheckStatistic() );
	}


	static void  FixedTupleArray_Test4 ()
	{
		using T1 = DebugInstanceCounter< int, 4 >;
		using T2 = DebugInstanceCounter< float, 4 >;

		T1::ClearStatistic();
		T2::ClearStatistic();
		{
			FixedTupleArray< 32, T1, T2 >	arr;
			
			arr.push_back( T1(1), T2(2.2f) );
			arr.push_back( T1(2), T2(3.3f) );
			arr.push_back( T1(3), T2(4.5f) );
			arr.push_back( T1(4), T2(5.8f) );

			arr.erase( 2 );
			TEST( arr.size() == 3 );
			TEST( arr.get<T1>()[0] == T1(1) );
			TEST( arr.get<T1>()[1] == T1(2) );
			TEST( arr.get<T1>()[2] == T1(4) );
			TEST( arr.get<T2>()[0] == T2(2.2f) );
			TEST( arr.get<T2>()[1] == T2(3.3f) );
			TEST( arr.get<T2>()[2] == T2(5.8f) );
		}
		TEST( T1::CheckStatistic() );
		TEST( T2::CheckStatistic() );
	}
}


extern void UnitTest_FixedTupleArray ()
{
	FixedTupleArray_Test1();
	FixedTupleArray_Test2();
	FixedTupleArray_Test3();
	FixedTupleArray_Test4();

	AE_LOGI( "UnitTest_FixedTupleArray - passed" );
}
