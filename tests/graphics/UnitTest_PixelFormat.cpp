// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#include "graphics/Private/EnumUtils.h"
#include "UnitTest_Common.h"

namespace
{
	static void PixelFormat_Test1 ()
	{
		for (EPixelFormat fmt = EPixelFormat(0); fmt < EPixelFormat::_Count; fmt = EPixelFormat(uint(fmt) + 1))
		{
			auto&	info = EPixelFormat_GetInfo( fmt );
			TEST( info.format == fmt );
		}
	}
}


extern void UnitTest_PixelFormat ()
{
	PixelFormat_Test1();
	AE_LOGI( "UnitTest_PixelFormat - passed" );
}
