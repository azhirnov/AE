// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Platforms/LinuxUtils.h"
#include "stl/Algorithms/ArrayUtils.h"

#ifdef PLATFORM_LINUX
# undef  _GNU_SOURCE
# define _GNU_SOURCE 1
# include <sys/prctl.h>
# include <pthread.h>

namespace AE::STL
{
	
/*
=================================================
	SetCurrentThreadName
=================================================
*/
	void LinuxUtils::SetThreadName (NtStringView name)
	{
		ASSERT( name.length() <= 16 );
		int	res = prctl( PR_SET_NAME, (unsigned long) name.c_str(), 0, 0, 0 );
		ASSERT( res == 0 );
	}
	
/*
=================================================
	GetThreadName
=================================================
*/
	String  LinuxUtils::GetThreadName ()
	{
		char	buf [16];
		int		res = prctl( PR_GET_NAME, buf, 0, 0, 0 );
		ASSERT( res == 0 );
		return String{buf};
	}
	
/*
=================================================
	SetThreadAffinity
=================================================
*/
	bool  LinuxUtils::SetThreadAffinity (const std::thread::native_handle_type &handle, uint cpuCore)
	{
		cpu_set_t cpuset;
		CPU_ZERO( &cpuset );
		CPU_SET( cpuCore, &cpuset );
		return pthread_setaffinity_np(handle, sizeof(cpu_set_t), &cpuset) == 0;
	}


}	// AE::STL

#endif	// PLATFORM_LINUX
