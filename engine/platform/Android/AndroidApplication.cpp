// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "platform/Android/AndroidApplication.h"

#ifdef PLATFORM_ANDROID

# include "threading/TaskSystem/TaskScheduler.h"
# include "graphics/Vulkan/VSwapchain.h"

namespace AE::App
{

/*
=================================================
	GetApp
=================================================
*/
namespace {

	ND_ AndroidApplication&  GetApp ()
	{
		auto*	app = AndroidApplication::_GetAppInstance();
		ASSERT( app );
		return *app;
	}
}
/*
=================================================
	_GetAppInstance
=================================================
*/
	AndroidApplication*&  AndroidApplication::_GetAppInstance ()
	{
		static AndroidApplication*	app = new AndroidApplication{ AE_OnAppCreated() };
		return app;
	}

/*
=================================================
	constructor
=================================================
*/
	AndroidApplication::AndroidApplication (UniquePtr<IAppListener> listener) :
		_listener{ std::move(listener) }
	{
	}
	
/*
=================================================
	destructor
=================================================
*/
	AndroidApplication::~AndroidApplication ()
	{
		EXLOCK( _drCheck );

		_OnDestroy();
	}
	
/*
=================================================
	CreateWindow
=================================================
*/
	WindowPtr  AndroidApplication::CreateWindow (WndListenerPtr listener, const WindowDesc &desc)
	{
		EXLOCK( _drCheck );

		if ( _windows.size() and not _windows.front().second->_listener )
		{
			_windows.front().second->_SetListener( std::move(listener) );
			return _windows.front().second;
		}

		RETURN_ERR( "multi-window is not supported yet" );
	}
	
/*
=================================================
	CreateVRDevice
=================================================
*/
	VRDevicePtr  AndroidApplication::CreateVRDevice ()
	{
		return null;
	}
	
/*
=================================================
	OpenResource
=================================================
*/
	SharedPtr<RStream>  AndroidApplication::OpenResource ()
	{
		// TODO
		return null;
	}
		
/*
=================================================
	GetMonitors
=================================================
*/
	IApplication::Monitors_t  AndroidApplication::GetMonitors ()
	{
		EXLOCK( _drCheck );

		return { _displayInfo };
	}
	
/*
=================================================
	GetVulkanInstanceExtensions
=================================================
*/
	ArrayView<const char*>  AndroidApplication::GetVulkanInstanceExtensions ()
	{
		static const char*	extensions[] = {
			"VK_KHR_android_surface"
		};
		return extensions;
	}
	
/*
=================================================
	Terminate
=================================================
*/
	void  AndroidApplication::Terminate ()
	{
		EXLOCK( _drCheck );
		
		for (auto& obj_wnd : _windows)
		{
			obj_wnd.second->Close();
		}
	}
	
/*
=================================================
	Update
=================================================
*/
	void  AndroidApplication::Update ()
	{
		EXLOCK( _drCheck );

		if ( _listener )
			_listener->OnUpdate( *this );
		
		Threading::Scheduler().ProcessTask( Threading::IThread::EThread::Main, 0 );
		
		for (size_t i = 0; i < _windows.size();)
		{
			if ( _windows[i].second->_wndState != IWindow::EState::Destroyed )
				++i;
			else
				_windows.fast_erase( i );
		}

		if ( _windows.empty() )
		{
			_OnDestroy();
		}
	}
	
/*
=================================================
	_OnDestroy
=================================================
*/
	void  AndroidApplication::_OnDestroy ()
	{
		if ( _listener )
		{
			_listener->OnStop( *this );
			_listener.reset();
		}

		_java.application		= Default;
		_java.assetManager		= Default;
		_java.jniAssetMngr		= null;

		_methods.createWindow	= Default;

		// windows must be destroyed before destroying app
		for (auto& obj_wnd : _windows)
		{
			CHECK( obj_wnd.second->_wndState == IWindow::EState::Destroyed );
			CHECK( obj_wnd.second.use_count() == 1 );
		}
		_windows.clear();
	}

/*
=================================================
	_AddWindow
=================================================
*/
	AndroidApplication::WinID  AndroidApplication::_AddWindow (SharedPtr<AndroidWindow> wnd)
	{
		const WinID	id = ++_windowCounter;

		_windows.emplace_back( id, wnd );

		if ( not _started and _listener )
			_listener->OnStart( *this );

		return id;
	}
/*
=================================================
	native_OnCreate
=================================================
*/
	void JNICALL AndroidApplication::native_OnCreate (JNIEnv *env, jclass, jobject appCtx, jobject assetMngr)
	{
		auto&	app = GetApp();
		EXLOCK( app._drCheck );

		JavaEnv	je{ env };

		app._java.application	= JavaObj{ appCtx, je };
		app._java.assetManager	= JavaObj{ assetMngr, je };
		app._java.jniAssetMngr	= AAssetManager_fromJava( env, app._java.assetManager.Get() );

		app._java.application.Method( "ShowToast", OUT app._methods.showToast );
		app._java.application.Method( "IsNetworkConnected", OUT app._methods.isNetworkConnected );
		app._java.application.Method( "CreateWindow", OUT app._methods.createWindow );
	}
	
/*
=================================================
	native_SetDirectories
=================================================
*/
	void JNICALL AndroidApplication::native_SetDirectories (JNIEnv*, jclass,
															jstring internalAppData, jstring internalCache,
															jstring externalAppData, jstring externalCache, jstring externalStorage)
	{
	}
	
/*
=================================================
	OnJniLoad
=================================================
*/
	jint AndroidApplication::OnJniLoad (JavaVM* vm)
	{
		using namespace AE::Java;

		JNIEnv* env;
		if ( vm->GetEnv( OUT reinterpret_cast<void**>(&env), JavaEnv::Version ) != JNI_OK )
			return -1;

		JavaEnv::SetVM( vm );

		// register application native methods
		{
			JavaClass	app_class{ "AE/engine/BaseApplication" };
			CHECK_ERR( app_class );

			app_class.RegisterStaticMethod( "native_OnCreate", &AndroidApplication::native_OnCreate );
			app_class.RegisterStaticMethod( "native_SetDirectories", &AndroidApplication::native_SetDirectories );
		}

		// register activity native methods
		{
			JavaClass	wnd_class{ "AE/engine/BaseActivity" };
			CHECK_ERR( wnd_class );

			wnd_class.RegisterStaticMethod( "native_OnCreate", &AndroidWindow::native_OnCreate );
			wnd_class.RegisterStaticMethod( "native_OnDestroy", &AndroidWindow::native_OnDestroy );
			wnd_class.RegisterStaticMethod( "native_OnStart", &AndroidWindow::native_OnStart );
			wnd_class.RegisterStaticMethod( "native_OnStop", &AndroidWindow::native_OnStop );
			wnd_class.RegisterStaticMethod( "native_OnEnterForeground", &AndroidWindow::native_OnEnterForeground );
			wnd_class.RegisterStaticMethod( "native_OnEnterBackground", &AndroidWindow::native_OnEnterBackground );
			wnd_class.RegisterStaticMethod( "native_SurfaceChanged", &AndroidWindow::native_SurfaceChanged );
			wnd_class.RegisterStaticMethod( "native_SurfaceDestroyed", &AndroidWindow::native_SurfaceDestroyed );
			wnd_class.RegisterStaticMethod( "native_Update", &AndroidWindow::native_Update );
			wnd_class.RegisterStaticMethod( "native_OnKey", &AndroidWindow::native_OnKey );
			wnd_class.RegisterStaticMethod( "native_OnTouch", &AndroidWindow::native_OnTouch );
			wnd_class.RegisterStaticMethod( "native_OnOrientationChanged", &AndroidWindow::native_OnOrientationChanged );
		}

		CHECK( AndroidApplication::_GetAppInstance() != null );

		return JavaEnv::Version;
	}
	
/*
=================================================
	OnJniUnload
=================================================
*/
	void AndroidApplication::OnJniUnload (JavaVM* vm)
	{
		using namespace AE::Java;

		auto&	app = AndroidApplication::_GetAppInstance();
		delete app;
		app = null;
	
		JavaEnv::SetVM( null );
	}

}	// AE::App

#endif	// PLATFORM_ANDROID
