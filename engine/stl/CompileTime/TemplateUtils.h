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

	
	//
	// Deferred Template Type
	//

	template <template <typename ...> class Templ, typename ...Types>
	struct DeferredTemplate
	{
		using type	= Templ< Types... >;
	};


}	// AE::STL
