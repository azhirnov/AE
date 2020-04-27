// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef AE_ENABLE_GLFW
# include "platform/Public/IApplication.h"
# include "platform/GLFW/WindowGLFW.h"

extern int main ();

typedef struct GLFWmonitor GLFWmonitor;

namespace AE::App
{

	//
	// GLFW Application
	//

	class ApplicationGLFW final : public IApplication, public Noncopyable
	{
	// types
	private:
		using Window		= WeakPtr< WindowGLFW >;
		using WindowArray_t	= FixedArray< Window, 8 >;


	// variables
	private:
		Atomic< bool >				_isRunning;
		UniquePtr< IAppListener >	_listener;
		WindowArray_t				_windows;
		const ThreadID				_mainThread;

		DataRaceCheck				_drCheck;


	// methods
	public:
		WindowPtr				CreateWindow (WndListenerPtr, const WindowDesc &) override;
		VRDevicePtr				CreateVRDevice () override;

		SharedPtr<RStream>		OpenResource () override;

		Monitors_t				GetMonitors () override;
		ArrayView<const char*>	GetVulkanInstanceExtensions () override;
		
		void					Terminate () override;
		
		EGraphicsApi			GetSupportedGAPI () override;

		static int				Run (UniquePtr<IAppListener>);

		static bool				GetMonitorInfo (GLFWmonitor*, OUT Monitor &);

	private:
		ApplicationGLFW (UniquePtr<IAppListener>);
		~ApplicationGLFW ();

		void _MainLoop ();
	};


}	// AE::App

#endif	// AE_ENABLE_GLFW
