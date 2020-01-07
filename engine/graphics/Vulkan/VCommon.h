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

namespace AE::Graphics
{
	using AE::Threading::Atomic;
	using AE::Threading::Mutex;
	using AE::Threading::SharedMutex;
	using AE::Threading::RWDataRaceCheck;
	using AE::Threading::EMemoryOrder;

	class VDevice;
	class VMemoryObj;
	class VResourceManager;
	
	using DebugName_t		= FixedString<64>;

	
	using VImageID						= HandleTmpl< uint16_t, uint16_t, __COUNTER__ >;
	using VBufferID						= HandleTmpl< uint16_t, uint16_t, __COUNTER__ >;
	using VSamplerID					= HandleTmpl< uint16_t, uint16_t, __COUNTER__ >;
	using VSamplerPackID				= HandleTmpl< uint16_t, uint16_t, __COUNTER__ >;

	using VDescriptorSetLayoutID		= HandleTmpl< uint16_t, uint16_t, __COUNTER__ >;
	using VPipelineLayoutID				= HandleTmpl< uint16_t, uint16_t, __COUNTER__ >;

	using VGraphicsPipelineTemplateID	= HandleTmpl< uint16_t, uint16_t, __COUNTER__ >;
	using VMeshPipelineTemplateID		= HandleTmpl< uint16_t, uint16_t, __COUNTER__ >;
	using VComputePipelineTemplateID	= HandleTmpl< uint16_t, uint16_t, __COUNTER__ >;
	//using VRayTracingPipelineTemplateID	= HandleTmpl< uint16_t, uint16_t, __COUNTER__ >;


}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
