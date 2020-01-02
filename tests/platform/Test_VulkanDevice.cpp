// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#ifdef AE_ENABLE_VULKAN
# include "platform/GAPI/Vulkan/VDevice.h"

extern void Test_VulkanDevice ()
{
	using namespace AE::Vulkan;
	{
		VDeviceInitializer	dev;

		CHECK_FATAL( dev.CreateInstance( "TestApp", "AE", dev.GetRecomendedInstanceLayers(), dev.GetInstanceExtensions_v110(), {1,1} ));
		CHECK_FATAL( dev.ChooseHighPerformanceDevice() );
		CHECK_FATAL( dev.CreateLogicalDevice( Default, Default ));

		CHECK_FATAL( dev.DestroyLogicalDevice() );
		CHECK_FATAL( dev.DestroyInstance() );
	}
	AE_LOGI( "Test_VulkanDevice - passed" );
}

#else

extern void Test_VulkanDevice ()
{}

#endif
