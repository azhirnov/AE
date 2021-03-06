// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "platform/Public/IApplication.h"

using namespace AE::App;

extern void UnitTest_GfxResourceID ();
extern void UnitTest_DescriptorSet ();
extern void Test_VulkanDevice ();
extern void Test_VulkanSwapchain (IApplication &app, IWindow &wnd);
extern void Test_VulkanRenderGraph (IApplication &app, IWindow &wnd);


#ifdef PLATFORM_ANDROID
extern int Test_Graphics (IApplication &app, IWindow &wnd)
{
	UnitTest_GfxResourceID();
	UnitTest_DescriptorSet();

	Test_VulkanDevice();
	Test_VulkanSwapchain( app, wnd );
	Test_VulkanRenderGraph( app, wnd );

	AE_LOGI( "Tests.Graphics finished" );
	return 0;
}
#endif


#if defined(PLATFORM_WINDOWS) or defined(PLATFORM_LINUX)

	class WndListener final : public IWindow::IWndListener
	{
	private:
		IApplication&	_app;

	public:
		WndListener (IApplication &app) : _app{app} {}
		~WndListener () override {}

		void OnCreate (IWindow &) override {}
		void OnDestroy (IWindow &) override {}
		void OnStart (IWindow &) override {}
		void OnStop (IWindow &) override {}
		void OnEnterForeground (IWindow &) override {}
		void OnEnterBackground (IWindow &) override {}
		void OnResize (IWindow &, const uint2 &) override {}
		void OnUpdate (IWindow &) override {}
		void OnSurfaceDestroyed (IWindow &wnd) override {}

		void OnSurfaceCreated (IWindow &wnd) override
		{
			Test_VulkanDevice();
			Test_VulkanSwapchain( _app, wnd );
			Test_VulkanRenderGraph( _app, wnd );

			AE_LOGI( "Tests.Graphics finished" );
			wnd.Close();
		}
	};

	class AppListener final : public IApplication::IAppListener
	{
	private:
		WindowPtr	_window;
		
	public:
		AppListener () {}
		~AppListener () override {}
		
		void OnStart (IApplication &app) override
		{
			UnitTest_GfxResourceID();
			UnitTest_DescriptorSet();

			#ifdef AE_CI_BUILD
				// CI can't create window and does not support vulkan
				app.Terminate();
			#else
				_window = app.CreateWindow( MakeUnique<WndListener>( app ), Default );
				CHECK_FATAL( _window );
			#endif
		}

		void OnUpdate (IApplication &app) override
		{
			if ( _window and _window->GetState() == IWindow::EState::Destroyed )
				app.Terminate();
		}

		void OnStop (IApplication &) override {}
	};

	UniquePtr<IApplication::IAppListener>  AE_OnAppCreated ()
	{
		return MakeUnique<AppListener>();
	}

#endif
