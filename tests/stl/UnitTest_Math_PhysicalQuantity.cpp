// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Math/PhysicalQuantity.h"
#include "stl/Math/PhysicalQuantityVec.h"
#include "UnitTest_Common.h"

namespace
{
	void PhysicalDimensions_Test1 ()
	{
		using Dim = DefaultPhysicalDimensions;
	
		STATIC_ASSERT( Dim::MeterPerSecond::meters.numerator == 1 );
		STATIC_ASSERT( Dim::MeterPerSecond::meters.denominator == 1 );
		STATIC_ASSERT( Dim::MeterPerSecond::seconds.numerator == -1 );
		STATIC_ASSERT( Dim::MeterPerSecond::seconds.denominator == 1 );
	}


	void PhysicalQuantity_Test1 ()
	{
		using Meters			= DefaultPhysicalQuantity<float>::Meters;
		using MetersPerSeconds	= DefaultPhysicalQuantity<float>::MetersPerSeconds;
		using Seconds			= DefaultPhysicalQuantity<float>::Seconds;
		using Milliseconds		= DefaultPhysicalQuantity<float>::Milliseconds;
		using Accel				= DefaultPhysicalQuantity<float>::MetersPerSquareSeconds;

		Meters				a1 = Meters{1.0f} + Meters{2.0f};						TEST(Equals( a1.GetNonScaled(), 3.0f ));
		MetersPerSeconds	a2 = Meters{10.0f} / Seconds{2.0f};						TEST(Equals( a2.GetNonScaled(), 5.0f ));
		MetersPerSeconds	a3 = Meters{14.0f} / Milliseconds{7.0f};				TEST(Equals( a3.GetNonScaled(), 2000.0f, 0.001f ));
		Accel				a4 = Meters{100.0f} / Seconds{2.0f} / Seconds{5.0f};	TEST(Equals( a4.GetNonScaled(), 10.0f ));
	}


	void PhysicalQuantityVec_Test1 ()
	{
		using Meters			= DefaultPhysicalQuantity<float>::Meters;
		using Seconds			= DefaultPhysicalQuantity<float>::Seconds;
		using MetersPerSeconds	= DefaultPhysicalQuantity<float>::MetersPerSeconds;
		using LightYears		= DefaultPhysicalQuantity<float>::LightYears;
		using Meters3			= PhysicalQuantityVec< Meters, 3 >;
		using Seconds3			= PhysicalQuantityVec< Seconds, 3 >;
		using MetersPerSeconds3	= PhysicalQuantityVec< MetersPerSeconds, 3 >;
		using LightYears3		= PhysicalQuantityVec< LightYears, 3 >;

		MetersPerSeconds3	a1 = Meters3{ 1.0f, 2.0f, 3.0f } / Seconds3{Seconds{4.0f}};

		float				b2 = Distance( float3{2.0f, 3.0f, 8.0f}, float3{-4.0f, 1.0f, 5.0f} );
		LightYears			a2 = Distance( LightYears3{2.0f, 3.0f, 8.0f}, LightYears3{-4.0f, 1.0f, 5.0f} );
		TEST(Equals( a2.GetNonScaled(), b2 ));
	
		float				b3 = DistanceSqr( float3{2.0f, 3.0f, 8.0f}, float3{-4.0f, 1.0f, 5.0f} );
		auto				a3 = DistanceSqr( LightYears3{2.0f, 3.0f, 8.0f}, LightYears3{-4.0f, 1.0f, 5.0f} );
		TEST(Equals( a3.GetNonScaled(), b3 ));

		LightYears			a4 = Min( a2, LightYears{1.0f} + LightYears{2.0f} );
		TEST(Equals( a4.GetNonScaled(), 3.0f ));
	}
}


extern void UnitTest_Math_PhysicalQuantity ()
{
	PhysicalDimensions_Test1();
	PhysicalQuantity_Test1();
	PhysicalQuantityVec_Test1();

	AE_LOGI( "UnitTest_Math_PhysicalQuantity - passed" );
}
