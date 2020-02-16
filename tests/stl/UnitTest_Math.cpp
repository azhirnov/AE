// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Math/Math.h"
#include "stl/CompileTime/Math.h"
#include "UnitTest_Common.h"


namespace
{
	void IsIntersects_Test1 ()
	{
		TEST( IsIntersects( 2, 6, 5, 8 ));
		TEST( IsIntersects( 2, 6, 0, 3 ));
		TEST( IsIntersects( 2, 6, 3, 5 ));
		TEST( not IsIntersects( 2, 6, 6, 8 ));
		TEST( not IsIntersects( 2, 6, -3, 2 ));
	}


	void Wrap_Test1 ()
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
	IsIntersects_Test1();
	Wrap_Test1();

	AE_LOGI( "UnitTest_Math - passed" );
}
