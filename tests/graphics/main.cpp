// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#include "platform/Public/IApplication.h"

using namespace AE::App;
using namespace AE::Threading;

extern void UnitTest_DescriptorSet ();
//extern void UnitTest_FormattedText ();
extern void UnitTest_ImageDesc ();
extern void UnitTest_ImageSwizzle ();
extern void UnitTest_PixelFormat ();
extern void UnitTest_VertexInput ();

#ifdef AE_ENABLE_VULKAN
extern void Test_VulkanDevice ();
extern void Test_VulkanSwapchain (IApplication &app, IWindow &wnd);
extern void Test_VulkanRenderGraph (IApplication &app, IWindow &wnd);
#endif


#ifdef PLATFORM_ANDROID
extern int Test_Graphics (IApplication &app, IWindow &wnd)
{
	UnitTest_DescriptorSet();
	//UnitTest_FormattedText();
	UnitTest_ImageDesc();
	UnitTest_ImageSwizzle();
	UnitTest_PixelFormat();
	UnitTest_VertexInput();
	
	#ifdef AE_ENABLE_VULKAN
	Test_VulkanDevice();
	Test_VulkanSwapchain( app, wnd );
	Test_VulkanRenderGraph( app, wnd );
	#endif

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
		void OnSurfaceDestroyed (IWindow &) override {}

		void OnSurfaceCreated (IWindow &wnd) override
		{
			#ifdef AE_ENABLE_VULKAN
			Test_VulkanDevice();
			Test_VulkanSwapchain( _app, wnd );
			Test_VulkanRenderGraph( _app, wnd );
			#endif

			wnd.Close();
			AE_LOGI( "Tests.Graphics finished" );
		}
	};

	class AppListener final : public IApplication::IAppListener
	{
	private:
		WindowPtr	_window;
		
	public:
		AppListener ()
		{
			TaskScheduler::CreateInstance();
			Scheduler().Setup( 1 );
		}

		~AppListener () override
		{
			Scheduler().Release();
			TaskScheduler::DestroyInstance();
		}

		void OnStart (IApplication &app) override
		{
			UnitTest_DescriptorSet();
			//UnitTest_FormattedText();
			UnitTest_ImageDesc();
			UnitTest_ImageSwizzle();
			UnitTest_PixelFormat();
			UnitTest_VertexInput();
			
			_window = app.CreateWindow( MakeUnique<WndListener>( app ), Default );
			CHECK_FATAL( _window );
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

	void  AE_OnAppDestroyed ()
	{
		CHECK_FATAL( AE_DUMP_MEMLEAKS() );
	}

#endif	// PLATFORM_WINDOWS
