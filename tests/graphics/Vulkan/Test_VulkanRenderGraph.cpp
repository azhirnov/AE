// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#ifdef AE_ENABLE_VULKAN

# include "Test_VulkanRenderGraph.h"

# include "graphics/Vulkan/RenderGraph/VRenderGraph.h"
# include "graphics/Vulkan/VResourceManager.h"

# include "stl/Algorithms/StringUtils.h"
# include "stl/Utils/FileSystem.h"
# include "stl/Platforms/PlatformUtils.h"
# include "stl/Stream/FileStream.h"

# include "threading/TaskSystem/WorkerThread.h"

# include "pipeline_compiler/PipelineCompiler.h"

using namespace AE::App;
using namespace AE::Threading;


/*
=================================================
	Test_VulkanRenderGraph
=================================================
*/
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
	_tests.emplace_back( &VRGTest::Test_Buffer );
	_tests.emplace_back( &VRGTest::Test_Image );
	_tests.emplace_back( &VRGTest::Test_CopyBuffer1 );
	_tests.emplace_back( &VRGTest::Test_CopyBuffer2 );
	_tests.emplace_back( &VRGTest::Test_CopyImage1 );
	_tests.emplace_back( &VRGTest::Test_VirtualRes1 );
	_tests.emplace_back( &VRGTest::Test_Draw1 );
	_tests.emplace_back( &VRGTest::Test_DrawAsync1 );
	_tests.emplace_back( &VRGTest::Test_Compute1 );
}

/*
=================================================
	Run
=================================================
*/
bool  VRGTest::Run (IApplication &app, IWindow &wnd)
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
bool  VRGTest::_Create (IApplication &app, IWindow &wnd)
{
	ArrayView<const char*>	window_ext = app.GetVulkanInstanceExtensions();

	CHECK_ERR( _vulkan.CreateInstance( "TestApp", "AE", _vulkan.GetRecomendedInstanceLayers(), window_ext ));
	CHECK_ERR( _vulkan.ChooseHighPerformanceDevice() );
	CHECK_ERR( _vulkan.CreateLogicalDevice( Default ));

	// this is a test and the test should fail for any validation error
	_vulkan.CreateDebugCallback( DefaultDebugMessageSeverity );
	//								  [] (const VDeviceInitializer::DebugReport &rep) { CHECK_FATAL(not rep.isError); });

	_resourceMngr.reset( new VResourceManager{ _vulkan });
	CHECK_ERR( Cast<VResourceManager>(_resourceMngr)->Initialize() );
	
	CHECK_ERR( _swapchain.CreateSurface( wnd.GetNative() ));
	CHECK_ERR( _swapchain.Create( _resourceMngr.get(), wnd.GetSurfaceSize() ));

	_renderGraph.reset( new VRenderGraph{ *Cast<VResourceManager>(_resourceMngr) });
	CHECK_ERR( Cast<VRenderGraph>(_renderGraph)->Initialize() );

	CHECK_ERR( _CompilePipelines() );


	CHECK_ERR( Scheduler().Setup( 1 ));
	Scheduler().AddThread( MakeShared<WorkerThread>() );

	return true;
}

/*
=================================================
	_CompilePipelines
=================================================
*/
bool  VRGTest::_CompilePipelines ()
{
#ifdef PLATFORM_WINDOWS
	using namespace AE::PipelineCompiler;

	decltype(&CompilePipelines)		compile_pipelines = null;

	Path	dll_path;
	CHECK_ERR( FileSystem::Search( "PipelineCompiler.dll", 3, 3, OUT dll_path ));

	Library		lib;
	CHECK_ERR( lib.Load( dll_path ));
	CHECK_ERR( lib.GetProcAddr( "CompilePipelines", OUT compile_pipelines ));
		
	CHECK_ERR( FileSystem::FindAndSetCurrent( "tests/graphics/Vulkan", 5 ));
	
	const wchar_t*	pipeline_folder[]	= { L"pipelines" };
	const wchar_t*	shader_folder[]		= { L"shaders" };
	Path			output				= L"temp";
	
	FileSystem::CreateDirectories( output );

	output.append( L"pipelines.bin" );

	PipelinesInfo	info = {};
	info.inPipelines		= null;
	info.inPipelineCount	= 0;
	info.pipelineFolders	= pipeline_folder;
	info.pipelineFolderCount= uint(CountOf( pipeline_folder ));
	info.includeDirs		= null;
	info.includeDirCount	= 0;
	info.shaderFolders		= shader_folder;
	info.shaderFolderCount	= uint(CountOf( shader_folder ));
	info.outputPackName		= output.c_str();

	CHECK_ERR( compile_pipelines( &info ));

	auto	file = MakeShared<FileRStream>( output );
	CHECK_ERR( file->IsOpen() );

	_pipelinePack = _resourceMngr->LoadPipelinePack( file );
	CHECK_ERR( _pipelinePack );
#endif

	return true;
}

/*
=================================================
	_RunTests
=================================================
*/
bool  VRGTest::_RunTests ()
{
	for (;;)
	{
		if ( not _tests.empty() )
		{
			TestFunc_t&	func	= _tests.front();
			bool		passed	= (this->*func)();

			_testsPassed += uint(passed);
			_testsFailed += uint(not passed);
			_tests.pop_front();

			Scheduler().ProcessTask( IAsyncTask::EThread::Main, 0 );
		}
		else
		{
			AE_LOGI( "Tests passed: " + ToString( _testsPassed ) + ", failed: " + ToString( _testsFailed ));
			break;
		}
	}
	return not _testsFailed;
}

/*
=================================================
	_Destroy
=================================================
*/
void  VRGTest::_Destroy ()
{
	if ( _renderGraph )
	{
		Cast<VRenderGraph>(_renderGraph)->Deinitialize();
		_renderGraph.reset();
	}

	_swapchain.Destroy( _resourceMngr.get() );
	_swapchain.DestroySurface();

	if ( _resourceMngr )
	{
		_resourceMngr->ReleaseResource( _pipelinePack );

		Cast<VResourceManager>(_resourceMngr)->Deinitialize();
		_resourceMngr.reset();
	}

	CHECK( _vulkan.DestroyLogicalDevice() );
	CHECK( _vulkan.DestroyInstance() );

	Scheduler().Release();
}

#endif	// AE_ENABLE_VULKAN
