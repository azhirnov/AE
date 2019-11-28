// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "platform/App/Public/Common.h"

#ifdef PLATFORM_ANDROID

# include "platform/App/Public/IApplication.h"
# include "platform/App/Android/AndroidWindow.h"
# include <android/asset_manager_jni.h>

namespace AE::App
{

	//
	// Android Application
	//

	class AndroidApplication final : public IApplication
	{
	// types
	private:
		using Window		= SharedPtr< AndroidWindow >;
		using WinID			= AndroidWindow::WinID;
		using AWindows_t	= FixedArray<Pair< WinID, Window >, 8 >;


	// variables
	private:
		UniquePtr< IAppListener >	_listener;
		AWindows_t					_windows;
		Monitor						_displayInfo;
		WinID						_windowCounter	= 0;
		bool 						_started		= false;
		
		struct {
			JavaObj						application;
			JavaObj						assetManager;
			AAssetManager *				jniAssetMngr	= null;
		}							_java;
		struct {
			JavaMethod< jboolean () >				isNetworkConnected;
			JavaMethod< void (jstring, jboolean) >	showToast;
			JavaMethod< void () >					createWindow;
		}							_methods;

		DataRaceCheck				_drCheck;


	// methods
	private:
		AndroidApplication (UniquePtr<IAppListener>);

		void _OnDestroy ();

		ND_ WinID  _AddWindow (SharedPtr<AndroidWindow> wnd);

	public:
		~AndroidApplication ();

		WindowPtr				CreateWindow (WndListenerPtr, const WindowDesc &) override;
		VRDevicePtr				CreateVRDevice () override;

		SharedPtr<RStream>		OpenResource () override;

		Monitors_t				GetMonitors () override;
		ArrayView<const char*>	GetVulkanInstanceExtensions () override;
		
		void					Terminate () override;

		void					Update ();
		
		ND_ static AndroidApplication*&		_GetAppInstance ();
		ND_ static SharedPtr<AndroidWindow>	_GetAppWindow (WinID id);
		ND_ static Pair< SharedPtr<AndroidWindow>, WinID >	_GetNewWindow ();


	// called from java
	public:
		static void JNICALL  native_OnCreate (JNIEnv*, jclass, jobject app,  jobject assetMngr);
		static void JNICALL  native_SetDirectories (JNIEnv*, jclass, jstring, jstring, jstring, jstring, jstring);

		static jint OnJniLoad (JavaVM* vm);
		static void OnJniUnload (JavaVM* vm);
	};


}	// AE::App

#endif	// PLATFORM_ANDROID
