// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef AE_ENABLE_GLFW
# include "platform/Public/IWindow.h"

typedef struct GLFWwindow GLFWwindow;

namespace AE::App
{

	//
	// GLFW Window
	//

	class WindowGLFW final : public IWindow
	{
		friend class ApplicationGLFW;

	// types
	private:
		using Milliseconds	= InputEventQueue::Milliseconds;
		using TimePoint_t	= std::chrono::high_resolution_clock::time_point;


	// variables
	private:
		GLFWwindow *			_window		= null;
		UniquePtr<IWndListener>	_listener;
		InputEventQueueWriter	_eventQueue;
		const TimePoint_t		_appStartTime;
		EState					_wndState	= Default;

		DataRaceCheck			_drCheck;


	// methods
	public:
		~WindowGLFW ();
		
		void					Close () override;
		
		uint2					GetSurfaceSize () const override;
		EState					GetState () const override			{ return _wndState; }

		InputEventQueue const&	GetInputEventQueue () override;
		NativeWindow			GetNative () override;
		
		void  SetSize (const uint2 &size) override;
		void  SetPosition (const int2 &pos) override;
		void  SetPosition (Monitor::ID monitor, const int2 &pos) override;
		void  SetTitle (NtStringView title) override;


	private:
		WindowGLFW (UniquePtr<IWndListener>, TimePoint_t);

		bool  _Create (const WindowDesc &desc);
		void  _Destroy ();
		bool  _ProcessMessages ();

		void  _SetState (EState newState);

		static void _GLFW_RefreshCallback (GLFWwindow* wnd);
		static void _GLFW_ResizeCallback (GLFWwindow* wnd, int w, int h);
		static void _GLFW_KeyCallback (GLFWwindow* wnd, int key, int, int, int);
		static void _GLFW_MouseButtonCallback (GLFWwindow* wnd, int button, int action, int mods);
		static void _GLFW_CursorPosCallback (GLFWwindow* wnd, double xpos, double ypos);
		static void _GLFW_MouseWheelCallback (GLFWwindow* wnd, double dx, double dy);
		static void _GLFW_IconifyCallback (GLFWwindow* wnd, int iconified);
		
		ND_ static EKey  _MapKey (int key);

		ND_ Milliseconds  _Timestamp ();
	};


}	// AE::App

#endif	// AE_ENABLE_GLFW
