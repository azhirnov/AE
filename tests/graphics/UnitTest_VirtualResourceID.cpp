// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "graphics/Public/RenderGraph.h"
#include "UnitTest_Common.h"

namespace
{
	void VirtualResourceID_Test1 ()
	{
		{
			VirtualResourceID	id{ 1, 2, VirtualResourceID::EResType::Buffer, "1" };
			auto	i0 = id.Index();		TEST( i0 == 1 );
			auto	i1 = id.Generation();	TEST( i1 == 2 );
			auto	i2 = id.ResourceType();	TEST( i2 == VirtualResourceID::EResType::Buffer );
		}{
			VirtualResourceID	id{ 0xFFFF-1, 0x3FFF-1, VirtualResourceID::EResType::Image, "1" };
			auto	i0 = id.Index();		TEST( i0 == 0xFFFF-1 );
			auto	i1 = id.Generation();	TEST( i1 == 0x3FFF-1 );
			auto	i2 = id.ResourceType();	TEST( i2 == VirtualResourceID::EResType::Image );
		}
	}
}


extern void UnitTest_VirtualResourceID ()
{
	VirtualResourceID_Test1();

	AE_LOGI( "UnitTest_VirtualResourceID - passed" );
}
