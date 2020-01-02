// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Algorithms/ArrayUtils.h"
#include "UnitTest_Common.h"


namespace
{
	void LowerBound_Test1 ()
	{
		Array<int>	arr = { 0, 1, 2, 3, 4, 4, 4, 5, 6, 7, 7, 7, 7, 7, 8, 9, 10, 11, 12 };
		ArrayView	view = arr;
		size_t		pos;
	
		pos = LowerBound( view, 5 );		TEST( pos == 7 );
		pos = LowerBound( view, 9 );		TEST( pos == 15 );
		pos = LowerBound( view, 4 );		TEST( pos == 4 );
		pos = LowerBound( view, 7 );		TEST( pos == 9 );
		pos = LowerBound( view, 12 );		TEST( pos == 18 );
		pos = LowerBound( view, -1 );		TEST( pos == UMax );
		pos = LowerBound( view, 20 );		TEST( pos == UMax );
	}


	void BinarySearch_Test1 ()
	{
		Array<int>	arr = { 0, 1, 2, 3, 4, 4, 4, 5, 6, 7, 7, 7, 7, 7, 8, 9, 10, 11, 12 };
		ArrayView	view = arr;
		size_t		pos;
	
		pos = BinarySearch( view, 0 );		TEST( pos == 0 );
		pos = BinarySearch( view, 5 );		TEST( pos == 7 );
		pos = BinarySearch( view, 9 );		TEST( pos == 15 );
		pos = BinarySearch( view, 4 );		TEST( pos >= 4 and pos <= 6 );
		pos = BinarySearch( view, 7 );		TEST( pos >= 9 and pos <= 13 );
		pos = BinarySearch( view, 12 );		TEST( pos == 18 );
		pos = BinarySearch( view, -1 );		TEST( pos == UMax );
		pos = BinarySearch( view, 20 );		TEST( pos == UMax );
	}


	void BinarySearch_Test2 ()
	{
		Array<int>	arr = { 0, 1, 2, 3, 4, 4, 4, 5, 6, 7, 7, 7, 7, 7, 8, 9, 10, 11, 12, 13 };
		ArrayView	view = arr;
		size_t		pos;
	
		pos = BinarySearch( view, 0 );		TEST( pos == 0 );
		pos = BinarySearch( view, 5 );		TEST( pos == 7 );
		pos = BinarySearch( view, 9 );		TEST( pos == 15 );
		pos = BinarySearch( view, 4 );		TEST( pos >= 4 and pos <= 6 );
		pos = BinarySearch( view, 7 );		TEST( pos >= 9 and pos <= 13 );
		pos = BinarySearch( view, 13 );		TEST( pos == 19 );
		pos = BinarySearch( view, -1 );		TEST( pos == UMax );
		pos = BinarySearch( view, 20 );		TEST( pos == UMax );
	}


	void BinarySearch_Test3 ()
	{
		Array<int>	arr1 = { 0 };
		Array<int>	arr2 = { 1, 2 };
		size_t		pos;
	
		pos = BinarySearch( arr1, 0 );		TEST( pos == 0 );
		pos = BinarySearch( arr1, -1 );		TEST( pos == UMax );
		pos = BinarySearch( arr1, 20 );		TEST( pos == UMax );
	
		pos = BinarySearch( arr2, 1 );		TEST( pos == 0 );
		pos = BinarySearch( arr2, 2 );		TEST( pos == 1 );
		pos = BinarySearch( arr2, -1 );		TEST( pos == UMax );
		pos = BinarySearch( arr2, 20 );		TEST( pos == UMax );
	}


	void ExponentialSearch_Test1 ()
	{
		Array<int>	arr = { 0, 1, 2, 3, 4, 4, 4, 5, 6, 7, 7, 7, 7, 7, 8, 9, 10, 11, 12 };
		ArrayView	view = arr;
		size_t		pos;
	
		pos = ExponentialSearch( view, 5 );		TEST( pos == 7 );
		pos = ExponentialSearch( view, 9 );		TEST( pos == 15 );
		pos = ExponentialSearch( view, 4 );		TEST( pos >= 4 and pos <= 6 );
		pos = ExponentialSearch( view, 7 );		TEST( pos >= 9 and pos <= 13 );
		pos = ExponentialSearch( view, 12 );	TEST( pos == 18 );
		pos = ExponentialSearch( view, -1 );	TEST( pos == UMax );
		pos = ExponentialSearch( view, 20 );	TEST( pos == UMax );
	}
}


extern void UnitTest_Array ()
{
	LowerBound_Test1();
	BinarySearch_Test1();
	BinarySearch_Test2();
	BinarySearch_Test3();
	ExponentialSearch_Test1();

	AE_LOGI( "UnitTest_Array - passed" );
}
