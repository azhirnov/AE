// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef AE_ENABLE_VULKAN

# include "graphics/Public/Common.h"

# include "threading/Primitives/DataRaceCheck.h"

# include "stl/Algorithms/Cast.h"
# include "stl/Containers/FixedArray.h"
# include "stl/Containers/FixedString.h"
# include "stl/Containers/Ptr.h"

# include "graphics/Vulkan/VulkanLoader.h"
# include "graphics/Public/IDs.h"

# include "graphics/Vulkan/VulkanCheckError.h"

namespace AE::Graphics
{
	using AE::Threading::Atomic;
	using AE::Threading::Mutex;
	using AE::Threading::SharedMutex;
	using AE::Threading::RWDataRaceCheck;
	using AE::Threading::EMemoryOrder;
	using AE::Threading::ThreadFence;

	class VDevice;
	class VResourceManager;
	class VLogicalRenderPass;
	class VRenderPass;
	class VFramebuffer;
	
	using DebugName_t		= FixedString<64>;

	
	using VDependencyID					= HandleTmpl< uint16_t, uint16_t, __COUNTER__ >;
	using VImageID						= HandleTmpl< uint16_t, uint16_t, __COUNTER__ >;
	using VBufferID						= HandleTmpl< uint16_t, uint16_t, __COUNTER__ >;
	using VVirtualImageID				= HandleTmpl< uint16_t, uint16_t, __COUNTER__ >;
	using VVirtualBufferID				= HandleTmpl< uint16_t, uint16_t, __COUNTER__ >;
	using VSamplerID					= HandleTmpl< uint16_t, uint16_t, __COUNTER__ >;
	using VSamplerPackID				= HandleTmpl< uint16_t, uint16_t, __COUNTER__ >;

	using VDescriptorSetLayoutID		= HandleTmpl< uint16_t, uint16_t, __COUNTER__ >;
	using VPipelineLayoutID				= HandleTmpl< uint16_t, uint16_t, __COUNTER__ >;

	using VGraphicsPipelineTemplateID	= HandleTmpl< uint16_t, uint16_t, __COUNTER__ >;
	using VMeshPipelineTemplateID		= HandleTmpl< uint16_t, uint16_t, __COUNTER__ >;
	using VComputePipelineTemplateID	= HandleTmpl< uint16_t, uint16_t, __COUNTER__ >;
	//using VRayTracingPipelineTemplateID	= HandleTmpl< uint16_t, uint16_t, __COUNTER__ >;
	
	using VFramebufferID				= HandleTmpl< uint16_t, uint16_t, __COUNTER__ >;
	using VRenderPassOutputID			= HandleTmpl< uint16_t, uint16_t, __COUNTER__ >;


	struct VConfig
	{
		static constexpr uint	MaxQueues	= 16;
	};

	
	struct VDispatchIndirectCommand
	{
		uint3		groudCount;
	};


}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
