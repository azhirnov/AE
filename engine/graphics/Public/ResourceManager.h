// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef AE_ENABLE_VULKAN
#include "graphics/Public/VulkanResourceManager.h"

namespace AE::Graphics
{
	using IResourceManager = VResourceManager;
}
#endif
