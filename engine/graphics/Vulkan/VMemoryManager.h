// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef AE_ENABLE_VULKAN

# include "graphics/Vulkan/VCommon.h"

namespace AE::Graphics
{

	//
	// Vulkan Memory Manager
	//

	class VMemoryManager final
	{
	// variables
	private:


	// methods
	public:
		VMemoryManager ();
		~VMemoryManager ();
	};


}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
