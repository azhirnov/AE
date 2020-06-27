// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Platforms/WindowsUtils.h"
#include "stl/Platforms/AndroidUtils.h"
#include "stl/Platforms/LinuxUtils.h"

#include "stl/Platforms/WindowsLibrary.h"
#include "stl/Platforms/PosixLibrary.h"

#include "stl/Platforms/WindowsProcess.h"

namespace AE::STL
{
	#if defined(PLATFORM_WINDOWS)
		using PlatformUtils = WindowsUtils;
		using Process		= WindowsProcess;
	
	#elif defined(PLATFORM_ANDROID)
		using PlatformUtils = AndroidUtils;

	#elif defined(PLATFORM_LINUX)
		using PlatformUtils = LinuxUtils;
		
	#endif


	#if defined(PLATFORM_WINDOWS)
		using Library = WindowsLibrary;
		
	#elif defined(PLATFORM_LINUX) || defined(PLATFORM_ANDROID)
		using Library = PosixLibrary;

	#endif
	
}	// AE::STL
