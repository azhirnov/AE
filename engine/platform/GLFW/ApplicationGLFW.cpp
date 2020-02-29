// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#ifdef AE_ENABLE_GLFW

# include "threading/TaskSystem/TaskScheduler.h"
# include "platform/GLFW/ApplicationGLFW.h"
# include "stl/Algorithms/StringUtils.h"
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
		_listener{ std::move(listener) },
		_mainThread{ std::this_thread::get_id() },
		_appStartTime{ TimePoint_t::clock::now() }
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

		SharedPtr<WindowGLFW>	wnd{ new WindowGLFW{ std::move(listener), _appStartTime }};
		CHECK_ERR( wnd->_Create( desc ));

		_windows.push_back( wnd );
		return wnd;
	}
	
/*
=================================================
	CreateVRDevice
=================================================
*/
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
	SharedPtr<RStream>  ApplicationGLFW::OpenResource ()
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
			int2	pos;
			int2	size;
			glfwGetMonitorWorkarea( monitors[i], OUT &pos.x, OUT &pos.y, OUT &size.x, OUT &size.y );

			int2	size_mm;
			glfwGetMonitorPhysicalSize( monitors[i], OUT &size_mm.x, OUT &size_mm.y );

			float2	scale;
			glfwGetMonitorContentScale( monitors[i], OUT &scale.x, OUT &scale.y );

			result[i].id			= Monitor::ID(i);
			result[i].workArea		= RectI{ pos, pos + size };
			result[i].physicalSize	= float2(size_mm) * 1.0e-3f;
			result[i].scale			= scale;

			if ( const GLFWvidmode* mode = glfwGetVideoMode( monitors[i] ))
			{
				result[i].region	= RectI{ pos, pos + int2{ mode->width, mode->height }};
				result[i].freq		= mode->refreshRate;
				result[i].colorBits	= ubyte3{ mode->redBits, mode->greenBits, mode->blueBits };
			}
			else
			{
				result[i].region	= result[i].workArea;
			}

			// calc PPI
			result[i].ppi = float2(result[i].region.Size()) / result[i].physicalSize * 0.0254f;

			result[i].name = glfwGetMonitorName( monitors[i] );
		}

		return result;
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

			for (size_t i = 0; i < _windows.size();)
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
	Run
=================================================
*/
	int  ApplicationGLFW::Run (UniquePtr<IAppListener> listener)
	{
		int	res = 0;
		{
			CHECK_ERR( glfwInit() == GLFW_TRUE, 1 );

			AE::App::ApplicationGLFW	app{ std::move(listener) };
			app._MainLoop();
		}
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
