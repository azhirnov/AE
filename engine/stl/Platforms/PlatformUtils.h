// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Platforms/WindowsUtils.h"

namespace AE::STL
{
	#ifdef PLATFORM_WINDOWS
		using PlatformUtils = WindowsUtils;
	#endif

}	// AE::STL
