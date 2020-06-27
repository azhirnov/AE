// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Common.h"

namespace AE::STL
{

	//
	// On Destroy
	//

	struct OnDestroy
	{
	private:
		Function<void ()>	_fn;

	public:
		explicit OnDestroy (Function<void ()> &&fn) : _fn{ std::move(fn) } {}
		~OnDestroy ()	{ _fn(); }
	};

}	// AE::STL
