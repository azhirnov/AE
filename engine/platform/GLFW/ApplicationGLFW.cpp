// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#ifdef AE_ENABLE_GLFW

# include "threading/TaskSystem/TaskScheduler.h"
# include "platform/GLFW/ApplicationGLFW.h"
# include "stl/Algorithms/StringUtils.h"
# include "stl/Platforms/PlatformUtils.h"
# include "GLFW/glfw3.h"

namespace AE::App
{

/*
=================================================
	GLFW_ErrorCallback
=================================================
*/
namespace {
	void GLFW_ErrorCallback (int code, const char* msg)
	{
		AE_LOGE( "GLFW error: " + ToString( code ) + ", msg: \"" + msg + "\"" );
	}
}

/*
=================================================
	constructor
=================================================
*/
	ApplicationGLFW::ApplicationGLFW (UniquePtr<IAppListener> listener) :
		_isRunning{ true },
		_listener{ RVRef(listener) },
		_mainThread{ std::this_thread::get_id() }
	{
		glfwSetErrorCallback( &GLFW_ErrorCallback );
	}

/*
=================================================
	destructor
=================================================
*/
	ApplicationGLFW::~ApplicationGLFW ()
	{
		EXLOCK( _drCheck );

		// windows must be destroyed before destroying app
		for (auto& wnd : _windows)
		{
			CHECK( not wnd.lock() );
		}
				
		glfwTerminate();
	}
	
/*
=================================================
	CreateWindow
=================================================
*/
	WindowPtr  ApplicationGLFW::CreateWindow (WndListenerPtr listener, const WindowDesc &desc)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( _isRunning.load( EMemoryOrder::Relaxed ));
		CHECK_ERR( listener );

		SharedPtr<WindowGLFW>	wnd{ new WindowGLFW{ *this, RVRef(listener) }};
		CHECK_ERR( wnd->_Create( desc ));

		_windows.push_back( wnd );
		return wnd;
	}
	
/*
=================================================
	CreateVRDevice
=================================================
*
	VRDevicePtr  ApplicationGLFW::CreateVRDevice ()
	{
		EXLOCK( _drCheck );

		// not supported
		return null;
	}
	
/*
=================================================
	OpenResource
=================================================
*/
	RC<RStream>  ApplicationGLFW::OpenResource ()
	{
		EXLOCK( _drCheck );

		// not supported
		return null;
	}
	
/*
=================================================
	GetMonitors
=================================================
*/
	ApplicationGLFW::Monitors_t  ApplicationGLFW::GetMonitors ()
	{
		EXLOCK( _drCheck );

		int				count;
		GLFWmonitor**	monitors = glfwGetMonitors( OUT &count );

		if ( not monitors or count <= 0 )
			return Default;

		Monitors_t	result;
		result.resize( count );

		for (int i = 0; i < count; ++i)
		{
			GetMonitorInfo( monitors[i], OUT result[i] );

			result[i].id = Monitor::ID(i);
		}

		return result;
	}
	
/*
=================================================
	GetMonitorInfo
=================================================
*/
	bool  ApplicationGLFW::GetMonitorInfo (GLFWmonitor* ptr, OUT Monitor &result)
	{
		CHECK_ERR( ptr );

		int2	pos;
		int2	size;
		glfwGetMonitorWorkarea( ptr, OUT &pos.x, OUT &pos.y, OUT &size.x, OUT &size.y );

		int2	size_mm;
		glfwGetMonitorPhysicalSize( ptr, OUT &size_mm.x, OUT &size_mm.y );

		float2	scale;
		glfwGetMonitorContentScale( ptr, OUT &scale.x, OUT &scale.y );

		result.workArea		= RectI{ pos, pos + size };
		result.physicalSize	= float2(size_mm) * 1.0e-3f;
		result.scale		= scale;

		if ( const GLFWvidmode* mode = glfwGetVideoMode( ptr ))
		{
			result.region	= RectI{ pos, pos + int2{ mode->width, mode->height }};
			result.freq		= mode->refreshRate;
			result.colorBits	= ubyte3{ mode->redBits, mode->greenBits, mode->blueBits };
		}
		else
		{
			result.region	= result.workArea;
		}

		// calc PPI
		const float	meter_to_inch = 0.0254f;
		result.ppi = float2(result.region.Size()) / result.physicalSize * meter_to_inch;

		result.name = glfwGetMonitorName( ptr );

		// set native handle
		#ifdef PLATFORM_WINDOWS
			POINT	pt = { pos.x+1, pos.y+1 };
			result.native = BitCast<Monitor::NativeMonitor_t>( ::MonitorFromPoint( pt, MONITOR_DEFAULTTONULL ));
		#else
			AE_COMPILATION_MESSAGE( "no implementation for current platform!" );
		#endif

		return true;
	}

/*
=================================================
	GetVulkanInstanceExtensions
=================================================
*/
	ArrayView<const char*>  ApplicationGLFW::GetVulkanInstanceExtensions ()
	{
		ArrayView<const char*>	result;
		
		uint32_t		required_extension_count = 0;
		const char **	required_extensions		 = glfwGetRequiredInstanceExtensions( OUT &required_extension_count );

		return ArrayView<const char*>{ required_extensions, required_extension_count };
	}

/*
=================================================
	_MainLoop
=================================================
*/
	void  ApplicationGLFW::_MainLoop ()
	{
		EXLOCK( _drCheck );

		_listener->OnStart( *this );

		for (; _isRunning.load( EMemoryOrder::Relaxed );)
		{
			glfwPollEvents();

			_listener->OnUpdate( *this );

			Threading::Scheduler().ProcessTask( Threading::IThread::EThread::Main, 0 );

			for (usize i = 0; i < _windows.size();)
			{
				if ( auto wnd = _windows[i].lock(); wnd and wnd->_ProcessMessages() )
					++i;
				else
					_windows.fast_erase( i );
			}

			if ( _windows.empty() )
			{
				std::this_thread::yield();
			}
		}

		_listener->OnStop( *this );
	}
	
/*
=================================================
	Terminate
=================================================
*/
	void  ApplicationGLFW::Terminate ()
	{
		_isRunning.store( false, EMemoryOrder::Relaxed );
	}
	
/*
=================================================
	GetSupportedGAPI
=================================================
*/
	EGraphicsApi  ApplicationGLFW::GetSupportedGAPI ()
	{
	#ifdef PLATFORM_WINDOWS
		const char	vk_lib_name[] = "vulkan-1.dll";
		const char	gl_lib_name[] = "opengl32.dll";
	#endif

		EGraphicsApi	result = Zero;

		Library		vk_lib;
		Library		gl_lib;

		if ( vk_lib.Load( vk_lib_name ))
			result |= EGraphicsApi::Vulkan;

		if ( gl_lib.Load( gl_lib_name ))
			result |= EGraphicsApi::OpenGL;

		return result;
	}

/*
=================================================
	Run
=================================================
*/
	int  ApplicationGLFW::Run (UniquePtr<IAppListener> listener)
	{
		int	res = 0;
		{
			CHECK_ERR( glfwInit() == GLFW_TRUE, 1 );

			AE::App::ApplicationGLFW	app{ RVRef(listener) };
			app._MainLoop();
		}

		AE_OnAppDestroyed();
		return res;
	}

}	// AE::App


/*
=================================================
	main
=================================================
*/
extern int main ()
{
	return AE::App::ApplicationGLFW::Run( AE_OnAppCreated() );
}

#endif	// AE_ENABLE_GLFW
