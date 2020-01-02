// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#ifdef AE_ENABLE_VULKAN

# include "platform/GAPI/Vulkan/VDevice.h"
# include "platform/GAPI/Vulkan/VSwapchain.h"

# include "platform/App/Public/IWindow.h"
# include "platform/App/Public/IApplication.h"

using namespace AE::App;
using namespace AE::Vulkan;

extern void Test_VulkanSwapchain (IApplication &app, IWindow &wnd)
{
	VDeviceInitializer		vulkan;
	VSwapchainInitializer	swapchain{ vulkan };

	Array<const char*>		instance_ext	{ vulkan.GetInstanceExtensions_v110() };
	ArrayView<const char*>	window_ext		= app.GetVulkanInstanceExtensions();
	instance_ext.insert( instance_ext.end(), window_ext.begin(), window_ext.end() );

	CHECK_FATAL( vulkan.CreateInstance( "TestApp", "AE", vulkan.GetRecomendedInstanceLayers(), instance_ext, {1,1} ));
	CHECK_FATAL( vulkan.ChooseHighPerformanceDevice() );
	CHECK_FATAL( vulkan.CreateLogicalDevice( Default, vulkan.GetDeviceExtensions_v110() ));
	
	CHECK_FATAL( swapchain.CreateSurface( wnd.GetNative() ));
	CHECK_FATAL( swapchain.Create( wnd.GetSurfaceSize() ));
	
	swapchain.Destroy();
	swapchain.DestroySurface();

	CHECK_FATAL( vulkan.DestroyLogicalDevice() );
	CHECK_FATAL( vulkan.DestroyInstance() );

	AE_LOGI( "Test_VulkanSwapchain - passed" );
}

#else

extern void Test_VulkanSwapchain (IApplication &, IWindow &)
{}

#endif
