// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Common.h"

namespace AE::STL
{

	//
	// Value to Type
	//

	template <auto Value>
	struct ValueToType
	{
		static constexpr auto	value = Value;
	};


}	// AE::STL
