// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "platform/Public/Monitor.h"
#include "platform/Public/IWindow.h"
#include "platform/Public/AppEnums.h"

namespace AE::App
{

	//
	// Application interface
	//

	class IApplication
	{
	// types
	public:
		class IAppListener
		{
		// interface
		public:
			virtual ~IAppListener () {}

			virtual void OnUpdate (IApplication &app) = 0;

			virtual void OnStart (IApplication &app) = 0;
			virtual void OnStop (IApplication &app) = 0;
		};

		using Monitors_t		= FixedArray< Monitor, 8 >;
		using WndListenerPtr	= UniquePtr< IWindow::IWndListener >;


	// interface
	public:
		ND_ virtual WindowPtr				CreateWindow (WndListenerPtr, const WindowDesc &) = 0;
		ND_ virtual VRDevicePtr				CreateVRDevice () = 0;
		ND_ virtual SharedPtr<RStream>		OpenResource () = 0;
		ND_ virtual Monitors_t				GetMonitors () = 0;
		ND_ virtual ArrayView<const char*>	GetVulkanInstanceExtensions () = 0;
		ND_ virtual EGraphicsApi			GetSupportedGAPI () = 0;
			virtual void					Terminate () = 0;

		// TODO:
		//	- vibration (android)
		//	- set orientation (android)
		//	- enum gapi
	};


}	// AE::App


extern std::unique_ptr<AE::App::IApplication::IAppListener>  AE_OnAppCreated ();
extern void													 AE_OnAppDestroyed ();
