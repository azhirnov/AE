// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "graphics/Public/Common.h"

namespace AE::Graphics
{
	
	using CommandBufferVk_t			= struct __VkCommandBufferType *;
	enum BufferVk_t					: uint64_t {};
	enum FramebufferVk_t			: uint64_t {};
	enum ImageVk_t					: uint64_t {};
	enum RenderPassVk_t				: uint64_t {};
	enum AccelerationStructureVk_t	: uint64_t {};
	enum DeviceMemoryVk_t			: uint64_t {};
	
	enum FormatVk_t					: uint {};
	enum BufferUsageFlagsVk_t		: uint {};
	enum ImageUsageVk_t				: uint {};
	enum ImageLayoutVk_t			: uint {};
	enum ImageTypeVk_t				: uint {};
	enum ImageFlagsVk_t				: uint {};
	enum SampleCountFlagBitsVk_t	: uint {};
	enum MemoryPropertyFlagsVk_t	: uint {};
	//using QueueFamilyIndicesVk_t = FixedArray< uint8_t, 8 >;



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
		SampleCountFlagBitsVk_t	samples			= {};
		uint3					dimension;
		uint					arrayLayers		= 0;
		uint					maxLevels		= 0;
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
		BufferVk_t				buffer		= {};
		BufferUsageFlagsVk_t	usage		= {};
		BytesU					size;
		
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
		CommandBufferVk_t	cmdBuffer		= null;
		RenderPassVk_t		renderPass		= {};
		FramebufferVk_t		framebuffer		= {};
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
		MemoryPropertyFlagsVk_t	flags		= {};
		BytesU					offset;
		BytesU					size;
		void *					mappedPtr	= null;
	};


}	// AE::Graphics
