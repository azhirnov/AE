// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef AE_ENABLE_VULKAN

# include "graphics/Vulkan/VCommon.h"

namespace AE::Graphics
{

	//
	// Vulkan Virtual Image
	//

	class VVirtualImage
	{
	// variables
	private:
		DebugName_t		_debugName;


	// methods
	public:
		VVirtualImage () {}
		~VVirtualImage () {}


		bool Create (StringView dbgName)
		{
			_debugName = dbgName;
			return true;
		}


		void Destroy (const VResourceManager &)
		{}
	};

}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
