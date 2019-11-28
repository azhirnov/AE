// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "platform/App/Public/IApplication.h"

using namespace AE::App;

extern bool Test_GLFW ();
extern bool Test_VulkanDevice ();
extern bool Test_VulkanSwapchain (IApplication &app, IWindow &wnd);


#ifdef PLATFORM_ANDROID
extern int Test_Platform (IApplication &app, IWindow &wnd)
{
	Test_GLFW();
	Test_VulkanDevice();
	Test_VulkanSwapchain( app, wnd );

	AE_LOGI( "Tests.Platform finished" );
	return 0;
}
#endif
