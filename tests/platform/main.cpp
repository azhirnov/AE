// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#include "platform/Public/IApplication.h"

using namespace AE::App;


#ifdef PLATFORM_ANDROID
extern int Test_Platform (IApplication &app, IWindow &wnd)
{
	// TODO

	AE_LOGI( "Tests.Platform finished" );
	return 0;
}
#endif


#if defined(PLATFORM_WINDOWS) or defined(PLATFORM_LINUX)

	extern void Test_GLFW ();

	UniquePtr<IApplication::IAppListener>  AE_OnAppCreated ()
	{
		Test_GLFW();

		CHECK_FATAL( AE_DUMP_MEMLEAKS() );

		AE_LOGI( "Tests.Platform finished" );
		std::exit(0);
	}
	
	void  AE_OnAppDestroyed ()
	{
		// do nothing
	}

#endif
