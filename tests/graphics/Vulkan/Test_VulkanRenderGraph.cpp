// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#ifdef AE_ENABLE_VULKAN

# include "Test_VulkanRenderGraph.h"
# include "stl/Algorithms/StringUtils.h"

using namespace AE::App;

extern void Test_VulkanRenderGraph (IApplication &app, IWindow &wnd)
{
	VRGTest		test;
	CHECK_FATAL( test.Run( app, wnd ));

	AE_LOGI( "Test_VulkanRenderGraph - passed" );
}

/*
=================================================
	constructor
=================================================
*/
VRGTest::VRGTest () :
	_vulkan{},
	_swapchain{ _vulkan }
{
}

/*
=================================================
	Run
=================================================
*/
bool VRGTest::Run (IApplication &app, IWindow &wnd)
{
	bool	result = true;
	
	result &= _Create( app, wnd );
	result &= _RunTests();
	_Destroy();

	return result;
}

/*
=================================================
	_Create
=================================================
*/
bool VRGTest::_Create (IApplication &app, IWindow &wnd)
{
	Array<const char*>		instance_ext	{ _vulkan.GetInstanceExtensions_v110() };
	ArrayView<const char*>	window_ext		= app.GetVulkanInstanceExtensions();
	instance_ext.insert( instance_ext.end(), window_ext.begin(), window_ext.end() );

	CHECK_ERR( _vulkan.CreateInstance( "TestApp", "AE", _vulkan.GetRecomendedInstanceLayers(), instance_ext, {1,1} ));
	CHECK_ERR( _vulkan.ChooseHighPerformanceDevice() );
	CHECK_ERR( _vulkan.CreateLogicalDevice( Default, _vulkan.GetDeviceExtensions_v110() ));
	
	CHECK_ERR( _swapchain.CreateSurface( wnd.GetNative() ));
	CHECK_ERR( _swapchain.Create( wnd.GetSurfaceSize() ));

	_resourceMngr.reset( new VResourceManager{ _vulkan });
	CHECK_ERR( Cast<VResourceManager>(_resourceMngr)->Initialize() );

	_renderGraph.reset( new VRenderGraph{ *Cast<VResourceManager>(_resourceMngr) });
	CHECK_ERR( Cast<VRenderGraph>(_renderGraph)->Initialize() );

	return true;
}

/*
=================================================
	_RunTests
=================================================
*/
bool VRGTest::_RunTests ()
{
	bool	result = true;

	if ( not _tests.empty() )
	{
		TestFunc_t&	func	= _tests.front();
		bool		passed	= (this->*func)();

		_testsPassed += uint(passed);
		_testsFailed += uint(not passed);
		_tests.pop_front();
	}
	else
	{
		AE_LOGE( "Tests passed: " + ToString( _testsPassed ) + ", failed: " + ToString( _testsFailed ));

		if ( _testsFailed )
			result = false;
	}
	return result;
}

/*
=================================================
	_Destroy
=================================================
*/
void VRGTest::_Destroy ()
{
	if ( _renderGraph ) {
		Cast<VRenderGraph>(_renderGraph)->Deinitialize();
		_renderGraph.reset();
	}

	if ( _resourceMngr ) {
		Cast<VResourceManager>(_resourceMngr)->Deinitialize();
		_resourceMngr.reset();
	}

	_swapchain.Destroy();
	_swapchain.DestroySurface();

	CHECK( _vulkan.DestroyLogicalDevice() );
	CHECK( _vulkan.DestroyInstance() );
}


#endif	// AE_ENABLE_VULKAN
