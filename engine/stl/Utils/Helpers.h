// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

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
		explicit OnDestroy (Function<void ()> &&fn) : _fn{ RVRef(fn) } {}
		~OnDestroy ()	{ _fn(); }
	};

}	// AE::STL
