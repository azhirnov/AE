// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Containers/NtStringView.h"

#ifdef PLATFORM_LINUX
# include <thread>

namespace AE::STL
{

	//
	// Linux Utils
	//

	struct LinuxUtils final
	{
			static void		SetThreadName (NtStringView name);
		ND_ static String	GetThreadName ();
		
			static bool		SetThreadAffinity (const std::thread::native_handle_type &handle, uint cpuCore);
	};

}	// AE::STL

#endif	// PLATFORM_LINUX
