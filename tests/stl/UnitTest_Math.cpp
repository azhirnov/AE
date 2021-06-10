// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Math/Math.h"
#include "stl/CompileTime/Math.h"
#include "stl/Math/GLM.h"
#include "stl/Platforms/CPUInfo.h"
#include "UnitTest_Common.h"


namespace
{
	static void  CheckIntrinsics ()
	{
		auto&	info = CPUInfo::Get();
		
		#if (GLM_ARCH & GLM_ARCH_SSE3_BIT)
			TEST( info.SSE3 );
		#endif
		#if (GLM_ARCH & GLM_ARCH_SSE41_BIT)
			TEST( info.SSE41 );
		#endif
		#if (GLM_ARCH & GLM_ARCH_SSE42_BIT)
			TEST( info.SSE42 );
		#endif
		#if (GLM_ARCH & GLM_ARCH_AVX_BIT)
			TEST( info.AVX );
		#endif
		#if (GLM_ARCH & GLM_ARCH_AVX2_BIT)
			TEST( info.AVX2 );
		#endif
		#if (GLM_ARCH & GLM_ARCH_NEON_BIT)
			TEST( info.NEON );
		#endif

		TEST( info.POPCNT );
	}


	static void  IsIntersects_Test1 ()
	{
		TEST( IsIntersects( 2, 6, 5, 8 ));
		TEST( IsIntersects( 2, 6, 0, 3 ));
		TEST( IsIntersects( 2, 6, 3, 5 ));
		TEST( not IsIntersects( 2, 6, 6, 8 ));
		TEST( not IsIntersects( 2, 6, -3, 2 ));
	}


	static void  Wrap_Test1 ()
	{
		float b0 = Wrap( 1.0f, 2.0f, 5.0f );	TEST( Equals( b0, 4.0f ));
		float b1 = Wrap( 6.0f, 2.0f, 5.0f );	TEST( Equals( b1, 3.0f ));
		float b2 = Wrap( 4.0f, 2.0f, 5.0f );	TEST( Equals( b2, 4.0f ));
		float b4 = Wrap( 1.5f, 2.0f, 5.0f );	TEST( Equals( b4, 4.5f ));
		float b5 = Wrap( 5.5f, 2.0f, 5.0f );	TEST( Equals( b5, 2.5f ));
		float b6 = Wrap( 15.0f, 0.0f, 5.0f );	TEST( Equals( b6, 0.0f ));
		float b7 = Wrap( 2.0f, -5.0f, 0.0f );	TEST( Equals( b7, -3.0f ));
		float b10 = Wrap( 3.99f, 0.0f, 4.0f );	TEST( Equals( b10, 3.99f ));
	}
}


extern void UnitTest_Math ()
{
	CheckIntrinsics();
	IsIntersects_Test1();
	Wrap_Test1();

	AE_LOGI( "UnitTest_Math - passed" );
}
