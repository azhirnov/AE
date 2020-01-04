// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Math/Vec.h"
#include "UnitTest_Common.h"


namespace
{
	template <typename T>
	void TestVec2Align ()
	{
		using Self = Vec<T,2>;
		STATIC_ASSERT( offsetof(Self, x) + sizeof(T) == offsetof(Self, y) );
		STATIC_ASSERT( sizeof(T)*(2-1) == (offsetof(Self, y) - offsetof(Self, x)) );
	}
	
	template <typename T>
	void TestVec3Align ()
	{
		using Self = Vec<T,3>;
		STATIC_ASSERT( offsetof(Self, x) + sizeof(T) == offsetof(Self, y) );
		STATIC_ASSERT( offsetof(Self, y) + sizeof(T) == offsetof(Self, z) );
		STATIC_ASSERT( sizeof(T)*(3-1) == (offsetof(Self, z) - offsetof(Self, x)) );
	}
	
	template <typename T>
	void TestVec4Align ()
	{
		using Self = Vec<T,4>;
		STATIC_ASSERT( offsetof(Self, x) + sizeof(T) == offsetof(Self, y) );
		STATIC_ASSERT( offsetof(Self, y) + sizeof(T) == offsetof(Self, z) );
		STATIC_ASSERT( offsetof(Self, z) + sizeof(T) == offsetof(Self, w) );
		STATIC_ASSERT( sizeof(T)*(4-1) == (offsetof(Self, w) - offsetof(Self, x)) );
	}

	template <typename T>
	void TestVecAlign ()
	{
		// check is supported cast Vec to array
		TestVec2Align<T>();
		TestVec3Align<T>();
		TestVec4Align<T>();
	}

	
	void Vec_Test1 ()
	{
		TestVecAlign<float>();
		TestVecAlign<int>();
		TestVecAlign<double>();
		TestVecAlign<int64_t>();
		TestVecAlign<uint16_t>();
	}

	
	void Vec_Test2 ()
	{
		TEST(All( float2(1.1f, 2.2f) == float2(0.1f, 0.2f) + float2(1.0f, 2.0f) ));
	}
}


extern void UnitTest_Math_Vec ()
{
	Vec_Test1();
	Vec_Test2();

	AE_LOGI( "UnitTest_Math_Vec - passed" );
}
