// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "platform/Public/Monitor.h"
#include "platform/Public/InputEventQueue.h"

namespace AE::App
{

	//
	// Window Description
	//

	struct WindowDesc
	{
		String			title;
		uint2			size		= {640, 480};
		Monitor::ID		monitorId	= Default;
		bool			resizable	= false;
		bool			fullscreen	= false;
	};
	

	//
	// Native Window
	//

	struct NativeWindow
	{
		#ifdef PLATFORM_WINDOWS
			void*	hinstance		= null;
			void*	hwnd			= null;
		#endif
		#ifdef PLATFORM_ANDROID
			void*	nativeWindow	= null;	// ANativeWindow
		#endif
	};


	//
	// Window interface
	//

	class IWindow
	{
	// types
	public:
		class IWndListener
		{
		// interface
		public:
			virtual ~IWndListener () {}
			
			virtual void OnCreate (IWindow &wnd) = 0;
			virtual void OnDestroy (IWindow &wnd) = 0;

			virtual void OnUpdate (IWindow &wnd) = 0;
			virtual void OnResize (IWindow &wnd, const uint2 &newSize) = 0;
			
			virtual void OnStart (IWindow &wnd) = 0;
			virtual void OnStop (IWindow &wnd) = 0;

			virtual void OnEnterForeground (IWindow &wnd) = 0;
			virtual void OnEnterBackground (IWindow &wnd) = 0;
			
			virtual void OnSurfaceCreated (IWindow &wnd) = 0;
			virtual void OnSurfaceDestroyed (IWindow &wnd) = 0;
		};

		enum class EState
		{
			Unknown,
			Created,
			Started,
			InForeground,
			InBackground,
			Stopped,
			Destroyed,
		};


	// interface
	public:
		virtual void  Close () = 0;

		ND_ virtual uint2				GetSurfaceSize () const = 0;
		ND_ virtual EState				GetState () const = 0;
		ND_ virtual Monitor				GetMonitor () const = 0;

		ND_ virtual InputEventQueue&	GetInputEventQueue () = 0;	// TODO: return shared pointer ?
		ND_ virtual NativeWindow		GetNative () = 0;

		// desctop only
		virtual void  SetSize (const uint2 &size) = 0;
		virtual void  SetPosition (const int2 &pos) = 0;
		virtual void  SetPosition (Monitor::ID monitor, const int2 &pos) = 0;
		virtual void  SetTitle (NtStringView title) = 0;
	};


}	// AE::App
