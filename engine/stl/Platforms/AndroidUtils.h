// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Containers/NtStringView.h"

#ifdef PLATFORM_ANDROID

namespace AE::STL
{

	//
	// Android Utils
	//

	struct AndroidUtils final
	{
			static void		SetThreadName (NtStringView name);
		ND_ static String	GetThreadName ();
	};

}	// AE::STL

#endif	// PLATFORM_ANDROID
