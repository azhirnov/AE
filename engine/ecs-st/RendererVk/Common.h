// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef AE_ENABLE_VULKAN

# include "ecs-st/Renderer/Common.h"

namespace AE::ECS::RendererVk
{

	struct Config
	{
		// descriptor sets
		static constexpr uint	RenderPassDSIndex	= 0;
		static constexpr uint	DrawCallDSIndex		= 1;	// 1 and 2 for material and object DS
	};

}	// AE::ECS::RendererVk

#endif	// AE_ENABLE_VULKAN
