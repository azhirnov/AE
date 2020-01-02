// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "threading/Common.h"

namespace AE::Threading
{

	//
	// Dummy Lock
	//

	struct DummyLock
	{
		void lock ()	{}
		void unlock ()	{}
	};



	//
	// Dummy Shared Lock
	//

	struct DummySharedLock
	{
		void lock ()			{}
		void unlock ()			{}

		void lock_shared ()		{}
		void unlock_shared () 	{}
	};


}	// AE::Threading
