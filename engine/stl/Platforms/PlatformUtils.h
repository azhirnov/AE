// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Platforms/WindowsUtils.h"
#include "stl/Platforms/AndroidUtils.h"

namespace AE::STL
{
	#if defined(PLATFORM_WINDOWS)
		using PlatformUtils = WindowsUtils;
	
	#elif defined(PLATFORM_ANDROID)
		using PlatformUtils = AndroidUtils;
		
	#endif
	
}	// AE::STL
