// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "platform/App/Public/IApplication.h"

using namespace AE::App;

//extern void Test_GLFW ();
extern void Test_VulkanDevice ();
extern void Test_VulkanSwapchain (IApplication &app, IWindow &wnd);


#ifdef PLATFORM_ANDROID
extern int Test_Platform (IApplication &app, IWindow &wnd)
{
	Test_VulkanDevice();
	Test_VulkanSwapchain( app, wnd );

	AE_LOGI( "Tests.Platform finished" );
	return 0;
}
#endif


#if defined(PLATFORM_WINDOWS) or defined(PLATFORM_LINUX)

	class WndListener final : public IWindow::IWndListener
	{
	private:
		IApplication&	_app;
		uint			_counter	= 0;

	public:
		WndListener (IApplication &app) : _app{app} {}
		~WndListener () override {}

		void OnCreate (IWindow &) override {}
		void OnDestroy (IWindow &) override {}
		void OnStart (IWindow &) override {}
		void OnStop (IWindow &) override {}
		void OnEnterForeground (IWindow &) override {}
		void OnEnterBackground (IWindow &) override {}
		void OnSurfaceDestroyed (IWindow &) override {}
		void OnResize (IWindow &, const uint2 &) override {}

		void OnUpdate (IWindow &wnd) override
		{
			if ( ++_counter == 100 )
			{
				wnd.Close();
			}
		}

		void OnSurfaceCreated (IWindow &wnd) override
		{
			Test_VulkanDevice();
			Test_VulkanSwapchain( _app, wnd );
		}
	};

	class AppListener final : public IApplication::IAppListener
	{
	private:
		WindowPtr  _window;
		
	public:
		AppListener () {}
		~AppListener () override {}
		
		void OnStart (IApplication &app) override
		{
			#ifndef AE_CI_BUILD
				_window = app.CreateWindow( MakeUnique<WndListener>( app ), Default );
				CHECK_FATAL( _window );
			#else
				app.Terminate();
			#endif
		}

		void OnUpdate (IApplication &app) override {}
		void OnStop (IApplication &) override {}
	};

	UniquePtr<IApplication::IAppListener>  AE_OnAppCreated ()
	{
		return MakeUnique<AppListener>();
	}

#endif
