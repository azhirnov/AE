// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Platforms/AndroidUtils.h"
#include "stl/Algorithms/ArrayUtils.h"

#ifdef PLATFORM_ANDROID
# include <sys/prctl.h>

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
		int	res = prctl( PR_SET_NAME, (unsigned long) name.c_str(), 0, 0, 0 );
		ASSERT( res );
	}
	
/*
=================================================
	GetThreadName
=================================================
*/
	String  AndroidUtils::GetThreadName ()
	{
		char	buf [16];
		int		res = prctl( PR_GET_NAME, buf, 0, 0, 0 );
		ASSERT( res );
		return String{buf};
	}

}	// AE::STL

#endif	// PLATFORM_ANDROID
