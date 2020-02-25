// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "graphics/Public/Common.h"

namespace AE::Graphics
{
	
# ifndef VULKAN_CORE_H_
	using CommandBufferVk_t			= struct __VkCommandBufferType *;
	enum BufferVk_t					: uint64_t {};
	enum FramebufferVk_t			: uint64_t {};
	enum ImageVk_t					: uint64_t {};
	enum RenderPassVk_t				: uint64_t {};
	enum AccelerationStructureVk_t	: uint64_t {};
	enum DeviceMemoryVk_t			: uint64_t {};
	
	enum FormatVk_t					: uint {};
	enum BufferUsageVk_t			: uint {};
	enum ImageUsageVk_t				: uint {};
	enum ImageLayoutVk_t			: uint {};
	enum ImageTypeVk_t				: uint {};
	enum ImageFlagsVk_t				: uint {};
	enum SampleCountVk_t			: uint {};
	enum MemoryPropertyVk_t			: uint {};

# else
	using CommandBufferVk_t			= VkCommandBuffer;
	using BufferVk_t				= VkBuffer;
	using FramebufferVk_t			= VkFramebuffer;
	using ImageVk_t					= VkImage;
	using RenderPassVk_t			= VkRenderPass;
	#ifdef VK_NV_ray_tracing
	using AccelerationStructureVk_t	= VkAccelerationStructureNV;
	#else
	enum AccelerationStructureVk_t	: uint64_t {};
	#endif
	using DeviceMemoryVk_t			= VkDeviceMemory;
	
	using FormatVk_t				= VkFormat;
	using BufferUsageVk_t			= VkBufferUsageFlags;
	using ImageUsageVk_t			= VkImageUsageFlags;
	using ImageLayoutVk_t			= VkImageLayout;
	using ImageTypeVk_t				= VkImageType;
	using ImageFlagsVk_t			= VkImageCreateFlags;
	using SampleCountVk_t			= VkSampleCountFlagBits;
	using MemoryPropertyVk_t		= VkMemoryPropertyFlags;

# endif	// VULKAN_CORE_H_



	//
	// Vulkan Image Description
	//
	struct VulkanImageDesc
	{
		ImageVk_t				image			= {};
		ImageTypeVk_t			imageType		= {};
		ImageFlagsVk_t			flags			= {};
		ImageUsageVk_t			usage			= {};
		FormatVk_t				format			= {};
		ImageLayoutVk_t			currentLayout	= {};
		ImageLayoutVk_t			defaultLayout	= ImageLayoutVk_t(0x7FFFFFFF);
		SampleCountVk_t			samples			= {};
		uint3					dimension;
		uint					arrayLayers		= 0;
		uint					maxLevels		= 0;
		bool					canBeDestroyed	= true;
		//uint					queueFamily		= UMax;	// queue family that owns image, you must specify this correctly
														// if image created with exclusive sharing mode and you need to
														// keep current content of the image, otherwise keep default value.

		//QueueFamilyIndicesVk_t	queueFamilyIndices;		// required if sharing mode is concurent.
	};
	


	//
	// Vulkan Buffer Description
	//
	struct VulkanBufferDesc
	{
		BufferVk_t				buffer			= {};
		BufferUsageVk_t			usage			= {};
		BytesU					size;
		bool					canBeDestroyed	= true;
		
		//uint					queueFamily	= UMax;		// queue family that owns buffer, you must specify this correctly
														// if buffer created with exclusive sharing mode and you need to
														// keep current content of the buffer, otherwise keep default value.

		//QueueFamilyIndicesVk_t	queueFamilyIndices;		// required if sharing mode is concurent.
	};

	

	//
	// Vulkan Render Context
	//
	struct VulkanRenderContext
	{
		CommandBufferVk_t	cmdBuffer		= null;		// primary for sync, secondary for async context
		RenderPassVk_t		renderPass		= {};
		FramebufferVk_t		framebuffer		= {};		// non-null for sync, null for async context
		uint				subpassIndex	= 0;
	};

	

	//
	// Vulkan Transfer/Compute/Graphics Context
	//
	struct VulkanContext
	{
		CommandBufferVk_t	cmdBuffer		= null;
	};



	//
	// Vulkan Memory Object Info
	//
	struct VulkanMemoryObjInfo
	{
		DeviceMemoryVk_t		memory		= {};
		MemoryPropertyVk_t		flags		= {};
		BytesU					offset;
		BytesU					size;
		void *					mappedPtr	= null;
	};


}	// AE::Graphics
