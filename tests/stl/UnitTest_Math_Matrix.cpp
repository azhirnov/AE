// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Math/Matrix.h"
#include "stl/Math/MatrixStorage.h"
#include "UnitTest_Common.h"


namespace
{
	void MatrixStorage_Test1 ()
	{
		using CMat4x2_t = MatrixStorage< float, 4, 2, EMatrixOrder::ColumnMajor >;
		using RMat4x2_t = MatrixStorage< float, 4, 2, EMatrixOrder::RowMajor >;

		STATIC_ASSERT( VecSize<CMat4x2_t::Column_t> == 2 );
		STATIC_ASSERT( VecSize<CMat4x2_t::Row_t> == 4 );
		STATIC_ASSERT( sizeof(CMat4x2_t) == sizeof(float) * 4 * 2 );

		STATIC_ASSERT( VecSize<RMat4x2_t::Row_t> == 4 );
		STATIC_ASSERT( VecSize<RMat4x2_t::Column_t> == 2 );
		STATIC_ASSERT( sizeof(RMat4x2_t) == sizeof(float) * 4 * 2 );
	};


	void MatrixStorage_Test2 ()
	{
		using CMat4x2_t = MatrixStorage< float, 4, 2, EMatrixOrder::ColumnMajor, sizeof(float)*4 >;
		using RMat4x2_t = MatrixStorage< float, 4, 2, EMatrixOrder::RowMajor, sizeof(float)*4 >;

		STATIC_ASSERT( VecSize<CMat4x2_t::Column_t> == 2 );
		STATIC_ASSERT( VecSize<CMat4x2_t::Row_t> == 4 );
		STATIC_ASSERT( sizeof(CMat4x2_t) == sizeof(float) * 4 * 4 );

		STATIC_ASSERT( VecSize<RMat4x2_t::Row_t> == 4 );
		STATIC_ASSERT( VecSize<RMat4x2_t::Column_t> == 2 );
		STATIC_ASSERT( sizeof(RMat4x2_t) == sizeof(float) * 4 * 2 );
	};


	void MatrixStorage_Test3 ()
	{
		using float3x4_t = MatrixStorage< float, 3, 4, EMatrixOrder::ColumnMajor >;

		const float3x4_t  m1{};

		const float3x4_t  m2{ float4(1.0f, 2.0f, 3.0f, 4.0f),
							  float4(5.0f, 6.0f, 7.0f, 8.0f),
							  float4(9.0f, 10.0f, 11.0f, 12.0f) };

		TEST( All( m2[0] == float4(1.0f, 2.0f, 3.0f, 4.0f) ));
		TEST( All( m2[1] == float4(5.0f, 6.0f, 7.0f, 8.0f) ));
		TEST( All( m2[2] == float4(9.0f, 10.0f, 11.0f, 12.0f) ));
	
		const float3x4_t  m3{ 1.0f, 2.0f, 3.0f, 4.0f,
							  5.0f, 6.0f, 7.0f, 8.0f,
							  9.0f, 10.0f, 11.0f, 12.0f };

		TEST( All( m3[0] == float4(1.0f, 2.0f, 3.0f, 4.0f) ));
		TEST( All( m3[1] == float4(5.0f, 6.0f, 7.0f, 8.0f) ));
		TEST( All( m3[2] == float4(9.0f, 10.0f, 11.0f, 12.0f) ));
	}


	void MatrixStorage_Test4 ()
	{
		using float3x4_t = MatrixStorage< float, 3, 4, EMatrixOrder::RowMajor >;

		const float3x4_t  m1{};

		const float3x4_t  m2{ float3(1.0f, 2.0f, 3.0f),
							  float3(4.0f, 5.0f, 6.0f),
							  float3(7.0f, 8.0f, 9.0f),
							  float3(10.0f, 11.0f, 12.0f) };

		TEST( All( m2[0] == float3(1.0f, 2.0f, 3.0f) ));
		TEST( All( m2[1] == float3(4.0f, 5.0f, 6.0f) ));
		TEST( All( m2[2] == float3(7.0f, 8.0f, 9.0f) ));
		TEST( All( m2[3] == float3(10.0f, 11.0f, 12.0f) ));

		const float3x4_t  m3{ 1.0f, 2.0f, 3.0f,
							  4.0f, 5.0f, 6.0f,
							  7.0f, 8.0f, 9.0f,
							  10.0f, 11.0f, 12.0f };

		TEST( All( m3[0] == float3(1.0f, 2.0f, 3.0f) ));
		TEST( All( m3[1] == float3(4.0f, 5.0f, 6.0f) ));
		TEST( All( m3[2] == float3(7.0f, 8.0f, 9.0f) ));
		TEST( All( m3[3] == float3(10.0f, 11.0f, 12.0f) ));
	}
}


extern void UnitTest_Math_Matrix ()
{
	MatrixStorage_Test1();
	MatrixStorage_Test2();
	MatrixStorage_Test3();
	MatrixStorage_Test4();

	AE_LOGI( "UnitTest_Math_Matrix - passed" );
}
