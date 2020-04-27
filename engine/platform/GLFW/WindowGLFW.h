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

	// variables
	private:
		GLFWwindow *				_window				= null;
		UniquePtr<IWndListener>		_listener;
		InputEventQueueWriter		_eventQueue;
		EState						_wndState			= Default;
		ApplicationGLFW &			_app;

		DataRaceCheck				_drCheck;


	// methods
	public:
		~WindowGLFW ();
		
		void				Close () override;
		
		uint2				GetSurfaceSize () const override;
		EState				GetState () const override;
		Monitor				GetMonitor () const override;

		InputEventQueue&	GetInputEventQueue () override;
		NativeWindow		GetNative () override;

		void  SetSize (const uint2 &size) override;
		void  SetPosition (const int2 &pos) override;
		void  SetPosition (Monitor::ID monitor, const int2 &pos) override;
		void  SetTitle (NtStringView title) override;


	private:
		explicit WindowGLFW (ApplicationGLFW &app, UniquePtr<IWndListener>);

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
	};


}	// AE::App

#endif	// AE_ENABLE_GLFW
