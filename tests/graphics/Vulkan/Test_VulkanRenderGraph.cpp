// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#ifdef AE_ENABLE_VULKAN

# include "stl/Algorithms/StringUtils.h"
# include "stl/Platforms/PlatformUtils.h"
# include "stl/Stream/FileStream.h"
# include "stl/Algorithms/StringParser.h"

# include "Test_VulkanRenderGraph.h"

# include "threading/TaskSystem/WorkerThread.h"

# include "pipeline_compiler/PipelineCompiler.h"

using namespace AE::App;
using namespace AE::Threading;

static constexpr bool	UpdateAllReferenceDumps = true;

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
	_vulkan{ True{"enable info log"} },
	_swapchain{ _vulkan },
	_refDumpPath{ AE_CURRENT_DIR "/Vulkan/ref" }
{
	_tests.emplace_back( &VRGTest::Test_LocalResManager );
	_tests.emplace_back( &VRGTest::Test_LocalResRangesManager );
	_tests.emplace_back( &VRGTest::Test_CopyBuffer1 );
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
	CHECK_ERR( _vulkan.CheckConstantLimits() );

	// this is a test and the test should fail for any validation error
	_vulkan.CreateDebugCallback( DefaultDebugMessageSeverity,
								 [] (const VDeviceInitializer::DebugReport &rep) { AE_LOGI(rep.message);  CHECK_FATAL(not rep.isError); });

	_AddVulkanHooks();

	VRenderGraph::CreateInstance( _vulkan );

	GraphicsCreateInfo	info;
	info.maxFrames		= 2;
	info.staging.readStaticSize .fill( 8_Mb );
	info.staging.writeStaticSize.fill( 8_Mb );
	info.staging.maxReadDynamicSize		= 0_b;
	info.staging.maxWriteDynamicSize	= 0_b;

	auto&	rg = RenderGraph();
	CHECK_ERR( rg.Initialize( info ));
	
	CHECK_ERR( _swapchain.CreateSurface( wnd.GetNative() ));
	CHECK_ERR( _swapchain.Create( &rg.GetResourceManager(), wnd.GetSurfaceSize() ));

	//CHECK_ERR( _CompilePipelines() );

	CHECK_ERR( Scheduler().Setup( 1 ));
	Scheduler().AddThread( MakeRC<WorkerThread>( WorkerThread::ThreadMask{}.set( uint(WorkerThread::EThread::Worker) ).set( uint(WorkerThread::EThread::Renderer) ),
												 nanoseconds{1}, milliseconds{4}, "render thread" ));

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

	Path	dll_path{ AE_PIPELINE_COMPILER_LIBRARY };
	//CHECK_ERR( FileSystem::Search( CMAKE_INTDIR "/PipelineCompiler.dll", 3, 3, OUT dll_path ));

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

	auto	file = MakeRC<FileRStream>( output );
	CHECK_ERR( file->IsOpen() );

	_pipelinePack = RenderGraph().GetResourceManager().LoadPipelinePack( file );
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
			_EnableLogging( true );

			TestFunc_t&	func	= _tests.front();
			bool		passed	= (this->*func)();
			
			_EnableLogging( false );

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
	_RemoveVulkanHooks();

	_swapchain.Destroy( &RenderGraph().GetResourceManager() );
	_swapchain.DestroySurface();

	RenderGraph().GetResourceManager().ReleaseResource( _pipelinePack );
	VRenderGraph::DestroyInstance();

	CHECK( _vulkan.DestroyLogicalDevice() );
	CHECK( _vulkan.DestroyInstance() );
}

/*
=================================================
	_CompareDumps
=================================================
*/
bool  VRGTest::_CompareDumps (StringView filename) const
{
	Path	fname = _refDumpPath;	fname.append( String{filename} << ".txt" );

	String	right;
	_GetLog( OUT right );
		
	// override dump
	if ( UpdateAllReferenceDumps )
	{
		FileWStream		wfile{ fname };
		CHECK_ERR( wfile.IsOpen() );
		CHECK_ERR( wfile.Write( StringView{right} ));
		return true;
	}

	// read from file
	String	left;
	{
		FileRStream		rfile{ fname };
		CHECK_ERR( rfile.IsOpen() );
		CHECK_ERR( rfile.Read( usize(rfile.Size()), OUT left ));
	}

	usize		l_pos	= 0;
	usize		r_pos	= 0;
	uint2		line_number;
	StringView	line_str[2];

	const auto	LeftValid	= [&l_pos, &left ] ()	{ return l_pos < left.length(); };
	const auto	RightValid	= [&r_pos, &right] ()	{ return r_pos < right.length(); };
		
	const auto	IsEmptyLine	= [] (StringView str)
	{
		for (auto& c : str) {
			if ( c != '\n' and c != '\r' and c != ' ' and c != '\t' )
				return false;
		}
		return true;
	};


	// compare line by line
	for (; LeftValid() and RightValid(); )
	{
		// read left line
		do {
			StringParser::ReadLineToEnd( left, INOUT l_pos, OUT line_str[0] );
			++line_number[0];
		}
		while ( IsEmptyLine( line_str[0] ) and LeftValid() );

		// read right line
		do {
			StringParser::ReadLineToEnd( right, INOUT r_pos, OUT line_str[1] );
			++line_number[1];
		}
		while ( IsEmptyLine( line_str[1] ) and RightValid() );

		if ( line_str[0] != line_str[1] )
		{
			RETURN_ERR( "in: "s << filename << "\n\n"
						<< "line mismatch:" << "\n(" << ToString( line_number[0] ) << "): " << line_str[0]
						<< "\n(" << ToString( line_number[1] ) << "): " << line_str[1] );
		}
	}

	if ( LeftValid() != RightValid() )
	{
		RETURN_ERR( "in: "s << filename << "\n\n" << "sizes of dumps are not equal!" );
	}
		
	return true;
}

#endif	// AE_ENABLE_VULKAN
