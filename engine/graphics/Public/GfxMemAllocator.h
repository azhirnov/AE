// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "graphics/Public/Common.h"
#include "graphics/Public/BufferDesc.h"
#include "graphics/Public/ImageDesc.h"
#include "stl/Containers/UntypedStorage.h"
#include "graphics/Public/NativeTypes.OpenGL.h"
#include "graphics/Public/NativeTypes.Vulkan.h"

namespace AE::Graphics
{

	//
	// Graphics Memory Allocator interface
	//

	class IGfxMemAllocator : public std::enable_shared_from_this<IGfxMemAllocator>
	{
	// types
	public:
		using Storage_t					= UntypedStorage< sizeof(uint64_t) * 4, alignof(uint64_t) >;
		using NativeBufferHandle_t		= Union< NullUnion, BufferVk_t >;
		using NativeImageHandle_t		= Union< NullUnion, ImageVk_t >;
		using NativeAccelStructHandle_t	= Union< NullUnion, AccelerationStructureVk_t >;
		using NativeMemInfo_t			= Union< NullUnion, VulkanMemoryObjInfo >;


	// interface
	public:
		virtual ~IGfxMemAllocator () {}

		virtual bool AllocForImage (const NativeImageHandle_t &image, const ImageDesc &desc, OUT Storage_t &data) = 0;
		virtual bool AllocForBuffer (const NativeBufferHandle_t &buffer, const BufferDesc &desc, OUT Storage_t &data) = 0;
		//virtual bool AllocateForAccelStruct (const NativeAccelStructHandle_t &as, EMemoryType memType, OUT Storage_t &data) = 0;

		virtual bool Dealloc (INOUT Storage_t &data) = 0;
			
		virtual bool GetInfo (const Storage_t &data, OUT NativeMemInfo_t &info) const = 0;
	};

	using GfxMemAllocatorPtr = SharedPtr< IGfxMemAllocator >;


}	// AE::Graphics
