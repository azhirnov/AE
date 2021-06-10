// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef AE_ENABLE_VULKAN

# include "stl/Utils/FileSystem.h"
# include "stl/Algorithms/StringUtils.h"

# include "graphics/Vulkan/VDevice.h"
# include "graphics/Vulkan/VSwapchain.h"
# include "graphics/Vulkan/VResourceManager.h"
# include "graphics/Vulkan/VCommandBufferTypes.h"

# include "platform/Public/IWindow.h"
# include "platform/Public/IApplication.h"

using namespace AE::Graphics;

using AE::Threading::Scheduler;
using EStatus = AE::Threading::IAsyncTask::EStatus;


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

	UniqueID<PipelinePackID>	_pipelinePack;

	TestQueue_t					_tests;
	uint						_testsPassed		= 0;
	uint						_testsFailed		= 0;
	
	Path						_refDumpPath;


// methods
public:
	VRGTest ();
	~VRGTest () {}

	bool  Run (AE::App::IApplication &app, AE::App::IWindow &wnd);

private:
	bool  _Create (AE::App::IApplication &app, AE::App::IWindow &wnd);
	bool  _RunTests ();
	void  _Destroy ();

	void  _AddVulkanHooks ();
	void  _RemoveVulkanHooks ();
	void  _EnableLogging (bool enable);
	void  _GetLog (OUT String &log) const;
	bool  _CompareDumps (StringView filename) const;

	bool  _CompilePipelines ();
	
	template <typename Arg0, typename ...Args>
	void  DeleteResources (Arg0 &arg0, Args& ...args);

	ND_ static String  _GetFuncName (StringView src);

private:
	bool  Test_LocalResManager ();
	bool  Test_LocalResRangesManager ();

	bool  Test_CopyBuffer1 ();
};



template <typename Arg0, typename ...Args>
inline void  VRGTest::DeleteResources (Arg0 &arg0, Args& ...args)
{
	CHECK( RenderGraph().GetResourceManager().ReleaseResource( INOUT arg0 ));	// must be released
		
	if constexpr ( CountOf<Args...>() > 0 )
		DeleteResources( FwdArg<Args&>( args )... );
}
	

inline String  VRGTest::_GetFuncName (StringView src)
{
	usize	pos = src.find_last_of( "::" );

	if ( pos != StringView::npos )
		return String{ src.substr( pos+1 )};
	else
		return String{ src };
}

# define TEST_NAME	_GetFuncName( AE_FUNCTION_NAME )

#endif	// AE_ENABLE_VULKAN
