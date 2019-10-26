// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Containers/NtStringView.h"
#include <thread>

namespace AE::STL
{

	//void SetThreadName (const std::thread &, StringView name);
	void SetCurrentThreadName (NtStringView name);

}	// AE::STL
