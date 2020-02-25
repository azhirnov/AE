// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef AE_ENABLE_VULKAN

# include "graphics/Public/ResourceManager.h"
# include "graphics/Public/RenderGraph.h"

# include "graphics/Vulkan/VDevice.h"
# include "graphics/Vulkan/VSwapchain.h"

# include "platform/Public/IWindow.h"
# include "platform/Public/IApplication.h"

# include "stl/Algorithms/StringUtils.h"

using namespace AE::Graphics;


class VRGTest
{
// types
private:
	using TestFunc_t	= bool (VRGTest::*) ();
	using TestQueue_t	= Deque< TestFunc_t >;


// variables
public:
	VDeviceInitializer			_vulkan;
	VSwapchainInitializer		_swapchain;

	UniquePtr<IResourceManager>	_resourceMngr;
	UniquePtr<IRenderGraph>		_renderGraph;

	UniqueID<PipelinePackID>	_pipelinePack;

	TestQueue_t					_tests;
	uint						_testsPassed		= 0;
	uint						_testsFailed		= 0;
	

// methods
public:
	VRGTest ();
	~VRGTest () {}

	bool  Run (AE::App::IApplication &app, AE::App::IWindow &wnd);

private:
	bool  _Create (AE::App::IApplication &app, AE::App::IWindow &wnd);
	bool  _RunTests ();
	void  _Destroy ();

	bool  _CompilePipelines ();
	
	template <typename Arg0, typename ...Args>
	void  DeleteResources (Arg0 &arg0, Args& ...args);

	ND_ static String  _GetFuncName (StringView src);

private:
	bool  Test_Buffer ();
	bool  Test_Image ();
	bool  Test_CopyBuffer1 ();
	bool  Test_CopyBuffer2 ();
	bool  Test_CopyImage1 ();
	
	bool  Test_Draw1 ();
	bool  Test_DrawAsync1 ();
};



template <typename Arg0, typename ...Args>
inline void  VRGTest::DeleteResources (Arg0 &arg0, Args& ...args)
{
	CHECK( _resourceMngr->ReleaseResource( INOUT arg0 ));	// must be released
		
	if constexpr ( CountOf<Args...>() > 0 )
		DeleteResources( std::forward<Args&>( args )... );
}
	

inline String  VRGTest::_GetFuncName (StringView src)
{
	size_t	pos = src.find_last_of( "::" );

	if ( pos != StringView::npos )
		return String{ src.substr( pos+1 )};
	else
		return String{ src };
}

# define TEST_NAME	_GetFuncName( AE_FUNCTION_NAME )

#endif	// AE_ENABLE_VULKAN
