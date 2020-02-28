// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "graphics/Public/IDs.h"
#include "UnitTest_Common.h"

namespace
{
	static void  GfxResourceID_Test1 ()
	{
		{
			GfxResourceID	id{ 1, 2, GfxResourceID::EType::Buffer };
			auto	i0 = id.Index();		TEST( i0 == 1 );
			auto	i1 = id.Generation();	TEST( i1 == 2 );
			auto	i2 = id.ResourceType();	TEST( i2 == GfxResourceID::EType::Buffer );
		}{
			GfxResourceID	id{ 0x7FFF-1, 0x3FFF-1, GfxResourceID::EType::Image };
			auto	i0 = id.Index();		TEST( i0 == 0x7FFF-1 );
			auto	i1 = id.Generation();	TEST( i1 == 0x3FFF-1 );
			auto	i2 = id.ResourceType();	TEST( i2 == GfxResourceID::EType::Image );
		}
	}
}


extern void UnitTest_GfxResourceID ()
{
	GfxResourceID_Test1();

	AE_LOGI( "UnitTest_GfxResourceID - passed" );
}
