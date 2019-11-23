// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "platform/GAPI/Vulkan/VDevice.h"

using namespace AE::Vulkan;


extern void Test_VulkanDevice ()
{
	VDeviceInitializer	dev;

	CHECK_FATAL( dev.CreateInstance( "TestApp", "AE", dev.GetRecomendedInstanceLayers(), dev.GetInstanceExtensions_v110(), {1,1} ));
	CHECK_FATAL( dev.ChooseHighPerformanceDevice() );
	CHECK_FATAL( dev.CreateLogicalDevice( Default, Default ));

	CHECK_FATAL( dev.DestroyLogicalDevice() );
	CHECK_FATAL( dev.DestroyInstance() );

	AE_LOGI( "Test_VulkanDevice - passed" );
}
