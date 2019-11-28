// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "platform/App/Public/Common.h"

#ifdef PLATFORM_ANDROID

# include "platform/App/Public/IWindow.h"
# include "platform/App/Android/Java.h"
# include <android/native_window.h>

namespace AE::App
{
	using namespace AE::Java;


	//
	// Android Window
	//

	class AndroidWindow final : public IWindow
	{
		friend class AndroidApplication;

	// types
	private:
		using WinID	= jint;


	// variables
	private:
		UniquePtr< IWndListener >	_listener;
		InputEventQueueWriter		_eventQueue;
		EState						_wndState	= Default;
		uint2						_surfSize;

		struct {
			JavaObj						window;
			ANativeWindow*				nativeWindow	= null;
		}							_java;
		struct {
			JavaMethod< void () >		close;
		}							_methods;

		DataRaceCheck				_drCheck;


	// methods
	public:
		AndroidWindow ();
		~AndroidWindow ();

		void					Close () override;

		uint2					GetSurfaceSize () override;

		InputEventQueue const&	GetInputEventQueue () override;
		NativeWindow			GetNative () override;

		// desctop only
		void  SetSize (const uint2 &) override {}
		void  SetPosition (const int2 &) override {}
		void  SetPosition (Monitor::ID, const int2 &) override {}
		void  SetTitle (NtStringView) override {}


	private:
		void  _SetState (EState newState);
		void  _SetState2 (EState newState);
		void  _SetListener (UniquePtr<IWndListener> listener);


	// called from java
	public:
		static WinID JNICALL  native_OnCreate (JNIEnv*, jclass, jobject wnd);
		static void JNICALL  native_OnDestroy (JNIEnv*, jclass, WinID wndId);
		static void JNICALL  native_OnStart (JNIEnv*, jclass, WinID wndId);
		static void JNICALL  native_OnStop (JNIEnv*, jclass, WinID wndId);
		static void JNICALL  native_OnEnterForeground (JNIEnv*, jclass, WinID wndId);
		static void JNICALL  native_OnEnterBackground (JNIEnv*, jclass, WinID wndId);
		static void JNICALL  native_SurfaceChanged (JNIEnv*, jclass, WinID wndId, jobject surface);
		static void JNICALL  native_SurfaceDestroyed (JNIEnv*, jclass, WinID wndId);
		static void JNICALL  native_Update (JNIEnv*, jclass, WinID wndId);
		static void JNICALL  native_OnKey (JNIEnv*, jclass, WinID wndId, jint keycode, jint action);
		static void JNICALL  native_OnTouch (JNIEnv*, jclass, WinID wndId, jint action, jint count, jfloatArray data);
		static void JNICALL  native_OnOrientationChanged (JNIEnv*, jclass, WinID wndId, jint newOrientation);
	};


}	// AE::App

#endif	// PLATFORM_ANDROID
