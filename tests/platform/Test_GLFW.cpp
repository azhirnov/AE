// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#ifdef AE_ENABLE_GLFW
# include "platform/GLFW/ApplicationGLFW.h"

using namespace AE::App;

namespace
{
	Array<int>	events;


	class WndListener final : public IWindow::IWndListener
	{
	private:
		uint	_counter	= 0;

	public:
		void OnCreate (IWindow &) override
		{
			events.push_back( 2 );
		}
		
		void OnStart (IWindow &) override
		{
			events.push_back( 3 );
		}

		void OnEnterForeground (IWindow &) override
		{
			events.push_back( 5 );
		}

		void OnSurfaceCreated (IWindow &) override
		{
			events.push_back( 4 );
		}

		void OnEnterBackground (IWindow &) override
		{
			events.push_back( 6 );
		}

		void OnSurfaceDestroyed (IWindow &) override
		{
			events.push_back( 7 );
		}

		void OnStop (IWindow &) override
		{
			events.push_back( 8 );
		}

		void OnDestroy (IWindow &) override
		{
			events.push_back( 9 );
		}

		void OnUpdate (IWindow &wnd) override
		{
			if ( ++_counter > 1000 )
			{
				wnd.Close();
			}
		}
		
		void OnResize (IWindow &, const uint2 &) override
		{}
	};


	class AppListener final : public IApplication::IAppListener
	{
	private:
		WindowPtr	_window;

	public:
		void OnStart (IApplication &app) override
		{
			events.push_back( 1 );

			auto	monitors = app.GetMonitors();
			CHECK_FATAL( monitors.size() >= 1 );

			WindowDesc	desc;
			desc.monitor	= Monitor::ID(1);	// try to show window on second monitor

			_window = app.CreateWindow( MakeUnique<WndListener>(), desc );
			CHECK_FATAL( _window );
		}

		void OnStop (IApplication &) override
		{
			_window = null;

			events.push_back( 10 );
		}
			
		void OnUpdate (IApplication &app) override
		{
			if ( _window and _window->GetState() == IWindow::EState::Destroyed )
				app.Terminate();
		}
	};
}


extern void Test_GLFW ()
{
	ApplicationGLFW::Run( MakeUnique<AppListener>() );

	CHECK_FATAL( events == Array<int>{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 });

	AE_LOGI( "Test_GLFW - passed" );
}

#else

extern void Test_GLFW ()
{}

#endif
