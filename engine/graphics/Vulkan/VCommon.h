// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

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

# include "threading/TaskSystem/TaskScheduler.h"
# include "threading/TaskSystem/Promise.h"

namespace AE::Graphics
{
	using AE::Threading::Atomic;
	using AE::Threading::Mutex;
	using AE::Threading::SharedMutex;
	using AE::Threading::RecursiveMutex;
	using AE::Threading::RWDataRaceCheck;
	using AE::Threading::EMemoryOrder;
	using AE::Threading::ThreadFence;
	using AE::Threading::AsyncTask;

	class VDevice;
	class VResourceManagerImpl;
	class VLogicalRenderPass;
	class VRenderPass;
	class VFramebuffer;
	class VCommandPoolManager;
	class VCommandBatch;

	using DebugName_t = FixedString<64>;
	
	enum class StagingBufferIdx : uint {};

	
	using VSamplerID					= HandleTmpl< uint16_t, uint16_t, Graphics::_hidden_::VulkanIDs_Start + 6 >;
	using VSamplerPackID				= HandleTmpl< uint16_t, uint16_t, Graphics::_hidden_::VulkanIDs_Start + 7 >;

	using VPipelineLayoutID				= HandleTmpl< uint16_t, uint16_t, Graphics::_hidden_::VulkanIDs_Start + 8 >;

	using VGraphicsPipelineTemplateID	= HandleTmpl< uint16_t, uint16_t, Graphics::_hidden_::VulkanIDs_Start + 9 >;
	using VMeshPipelineTemplateID		= HandleTmpl< uint16_t, uint16_t, Graphics::_hidden_::VulkanIDs_Start + 10 >;
	using VComputePipelineTemplateID	= HandleTmpl< uint16_t, uint16_t, Graphics::_hidden_::VulkanIDs_Start + 11 >;
	//using VRayTracingPipelineTemplateID= HandleTmpl< uint16_t, uint16_t, Graphics::_hidden_::VulkanIDs_Start + 12 >;
	
	using VFramebufferID				= HandleTmpl< uint16_t, uint16_t, Graphics::_hidden_::VulkanIDs_Start + 13 >;
	using VRenderPassOutputID			= HandleTmpl< uint16_t, uint16_t, Graphics::_hidden_::VulkanIDs_Start + 14 >;


	struct VConfig
	{
		static constexpr uint	MaxQueues	= 8;
	};

	
	struct VDispatchIndirectCommand
	{
		uint3		groudCount;
	};


	// TODO: remove
	static constexpr VkAccessFlagBits	VReadAccessMask = 
		VK_ACCESS_INDIRECT_COMMAND_READ_BIT |
		VK_ACCESS_INDEX_READ_BIT |
		VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT |
		VK_ACCESS_UNIFORM_READ_BIT |
		VK_ACCESS_INPUT_ATTACHMENT_READ_BIT |
		VK_ACCESS_SHADER_READ_BIT |
		VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
		VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
		VK_ACCESS_TRANSFER_READ_BIT |
		VK_ACCESS_HOST_READ_BIT |
		VK_ACCESS_MEMORY_READ_BIT |
	#ifdef VK_EXT_transform_feedback
		VK_ACCESS_TRANSFORM_FEEDBACK_COUNTER_READ_BIT_EXT |
	#endif
	#ifdef VK_EXT_conditional_rendering
		VK_ACCESS_CONDITIONAL_RENDERING_READ_BIT_EXT |
	#endif
	#ifdef VK_EXT_blend_operation_advanced
		VK_ACCESS_COLOR_ATTACHMENT_READ_NONCOHERENT_BIT_EXT |
	#endif
	#ifdef VK_NV_shading_rate_image
		VK_ACCESS_SHADING_RATE_IMAGE_READ_BIT_NV |
	#endif
	#ifdef VK_NV_ray_tracing
		VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV |
	#endif
	#ifdef VK_EXT_fragment_density_map
		VK_ACCESS_FRAGMENT_DENSITY_MAP_READ_BIT_EXT |
	#endif
		Zero;

	static constexpr VkAccessFlagBits	VWriteAccessMask =
		VK_ACCESS_SHADER_WRITE_BIT |
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
		VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
		VK_ACCESS_TRANSFER_WRITE_BIT |
		VK_ACCESS_HOST_WRITE_BIT |
		VK_ACCESS_MEMORY_WRITE_BIT |
	#ifdef VK_EXT_transform_feedback
		VK_ACCESS_TRANSFORM_FEEDBACK_WRITE_BIT_EXT |
		VK_ACCESS_TRANSFORM_FEEDBACK_COUNTER_WRITE_BIT_EXT |
	#endif
	#ifdef VK_NV_ray_tracing
		VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV |
	#endif
		Zero;


}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
