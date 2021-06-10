// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Platforms/AndroidUtils.h"
#include "stl/Algorithms/ArrayUtils.h"

#ifdef PLATFORM_ANDROID
# include <sys/prctl.h>
# include <sched.h>

namespace AE::STL
{
	
/*
=================================================
	SetCurrentThreadName
=================================================
*/
	void AndroidUtils::SetThreadName (NtStringView name)
	{
		ASSERT( name.length() <= 16 );
		int	res = ::prctl( PR_SET_NAME, (unsigned long) name.c_str(), 0, 0, 0 );
		ASSERT( res == 0 );
	}
	
/*
=================================================
	GetThreadName
=================================================
*/
	String  AndroidUtils::GetThreadName ()
	{
		char	buf [16];
		int		res = ::prctl( PR_GET_NAME, buf, 0, 0, 0 );
		ASSERT( res == 0 );
		return String{buf};
	}
	
/*
=================================================
	SetThreadAffinity
=================================================
*/
	bool  AndroidUtils::SetThreadAffinity (const std::thread::native_handle_type &handle, uint cpuCore)
	{
		ASSERT( cpuCore < std::thread::hardware_concurrency() );

		::cpu_set_t  cpuset;
		CPU_ZERO( &cpuset );
		CPU_SET( cpuCore, &cpuset );

		return ::sched_setaffinity( 0, sizeof(cpu_set_t), &cpuset ) == 0;
	}


}	// AE::STL

#endif	// PLATFORM_ANDROID
