// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Common.h"

extern void Test_VulkanDevice ();


int main ()
{
	#ifndef AE_CI_BUILD
		Test_VulkanDevice();
	#endif

	AE_LOGI( "Tests.Platform finished" );
	return 0;
}
