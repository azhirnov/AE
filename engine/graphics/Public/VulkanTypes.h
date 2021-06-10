// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef AE_ENABLE_VULKAN

# include "graphics/Public/IDs.h"
# include "graphics/Vulkan/VulkanLoader.h"

namespace AE::Graphics
{
	#if not (defined(VK_NV_ray_tracing) or defined(VK_KHR_ray_tracing))
	enum vKAccelerationStructureKHR	: ulong {};
	#endif



	//
	// Vulkan Image Description
	//
	struct VulkanImageDesc
	{
		VkImage					image			= VK_NULL_HANDLE;
		VkImageType				imageType		= VK_IMAGE_TYPE_MAX_ENUM;
		VkImageCreateFlagBits	flags			= VkImageCreateFlagBits(0);
		VkImageUsageFlagBits	usage			= VkImageUsageFlagBits(0);
		VkFormat				format			= VK_FORMAT_UNDEFINED;
		VkImageLayout			currentLayout	= VK_IMAGE_LAYOUT_UNDEFINED;
		VkImageLayout			defaultLayout	= VK_IMAGE_LAYOUT_MAX_ENUM;
		VkSampleCountFlagBits	samples			= VK_SAMPLE_COUNT_1_BIT;
		uint3					dimension;
		uint					arrayLayers		= 0;
		uint					maxLevels		= 0;
		bool					canBeDestroyed	= true;
	};
	


	//
	// Vulkan Buffer Description
	//
	struct VulkanBufferDesc
	{
		VkBuffer				buffer			= VK_NULL_HANDLE;
		VkBufferUsageFlagBits	usage			= VkBufferUsageFlagBits(0);
		Bytes					size;
		bool					canBeDestroyed	= true;
	};

	

	//
	// Vulkan Render Context
	//
	struct VulkanRenderContext
	{
		VkCommandBuffer		cmdBuffer		= VK_NULL_HANDLE;		// primary for sync, secondary for async context
		VkRenderPass		renderPass		= VK_NULL_HANDLE;
		VkFramebuffer		framebuffer		= VK_NULL_HANDLE;		// non-null for sync, null for async context
		uint				subpassIndex	= 0;
	};

	

	//
	// Vulkan Transfer/Compute/Graphics Context
	//
	struct VulkanContext
	{
		VkCommandBuffer		cmdBuffer		= VK_NULL_HANDLE;
	};



	//
	// Vulkan Memory Object Info
	//
	struct VulkanMemoryObjInfo
	{
		VkDeviceMemory				memory		= VK_NULL_HANDLE;
		VkMemoryPropertyFlagBits	flags		= Zero;
		Bytes						offset;
		Bytes						size;
		void *						mappedPtr	= null;
	};



	//
	// Vulkan Pipeline Info 
	//
	struct VulkanPipelineInfo
	{
		using DSLayouts_t = FixedMap< DescriptorSetName, Tuple<uint, VkDescriptorSetLayout>, 8 >;

		VkPipeline				pipeline	= VK_NULL_HANDLE;
		VkPipelineLayout		layout		= VK_NULL_HANDLE;
		DSLayouts_t				dsLayouts;
	};


}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
