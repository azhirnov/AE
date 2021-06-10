// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Containers/NtStringView.h"

#ifdef PLATFORM_WINDOWS
# include <thread>

namespace AE::STL
{

	//
	// WinAPI Utils
	//

	struct WindowsUtils final
	{
	// types
		struct MemoryPageInfo
		{
			Bytes	pageSize;
			Bytes	allocationGranularity;
		};


	// functions
			static void		SetThreadName (NtStringView name);
		ND_ static String	GetThreadName ();

			static bool		SetThreadAffinity (const std::thread::native_handle_type &handle, uint cpuCore);
			static bool		SetThreadPriority (const std::thread::native_handle_type &handle, float priority);

			static bool		CheckErrors (StringView file = __FILE__, int line = __LINE__);

			static bool		GetMemoryPageInfo (OUT MemoryPageInfo &info);
	};

}	// AE::STL

#endif	// PLATFORM_WINDOWS
