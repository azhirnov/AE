// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "platform/Public/IWindow.h"
#include "platform/Public/IApplication.h"
using namespace AE::App;

#ifdef AE_ENABLE_VULKAN

# include "graphics/Vulkan/VDevice.h"
# include "graphics/Vulkan/VSwapchain.h"
using namespace AE::Graphics;

extern void Test_VulkanSwapchain (IApplication &app, IWindow &wnd)
{
	VDeviceInitializer		vulkan;
	VSwapchainInitializer	swapchain{ vulkan };

	ArrayView<const char*>	window_ext = app.GetVulkanInstanceExtensions();

	CHECK_FATAL( vulkan.CreateInstance( "TestApp", "AE", vulkan.GetRecomendedInstanceLayers(), window_ext ));
	CHECK_FATAL( vulkan.ChooseHighPerformanceDevice() );
	CHECK_FATAL( vulkan.CreateLogicalDevice( Default ));
	
	CHECK_FATAL( swapchain.CreateSurface( wnd.GetNative() ));
	CHECK_FATAL( swapchain.Create( null, wnd.GetSurfaceSize() ));
	
	swapchain.Destroy( null );
	swapchain.DestroySurface();

	CHECK_FATAL( vulkan.DestroyLogicalDevice() );
	CHECK_FATAL( vulkan.DestroyInstance() );

	AE_LOGI( "Test_VulkanSwapchain - passed" );
}

#else

extern void Test_VulkanSwapchain (IApplication &, IWindow &)
{}

#endif	// AE_ENABLE_VULKAN
