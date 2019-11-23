// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Common.h"

extern void Test_VulkanDevice ();


int main ()
{
	Test_VulkanDevice();

	AE_LOGI( "Tests.Platform finished" );
	return 0;
}
