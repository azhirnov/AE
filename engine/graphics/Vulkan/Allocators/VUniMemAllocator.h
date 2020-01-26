// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef AE_ENABLE_VULKAN

# include "graphics/Public/GfxMemAllocator.h"
# include "graphics/Vulkan/VCommon.h"

VK_DEFINE_HANDLE(VmaAllocation)
VK_DEFINE_HANDLE(VmaAllocator)

namespace AE::Graphics
{

	//
	// Vulkan Universal Memory Allocator
	//

	class VUniMemAllocator final : public IGfxMemAllocator
	{
	// types
	private:
		struct Data
		{
			VmaAllocation	allocation;
		};


	// variables
	private:
		VDevice const&		_device;
		VmaAllocator		_allocator;


	// methods
	public:
		explicit VUniMemAllocator (const VDevice &dev);
		~VUniMemAllocator () override;
		
		bool AllocForImage (const NativeImageHandle_t &image, const ImageDesc &desc, OUT Storage_t &data) override;
		bool AllocForBuffer (const NativeBufferHandle_t &buffer, const BufferDesc &desc, OUT Storage_t &data) override;
		//bool AllocateForAccelStruct (const NativeAccelStructHandle_t &as, EMemoryType memType, OUT Storage_t &data) override;

		bool Dealloc (INOUT Storage_t &data) override;
			
		bool GetInfo (const Storage_t &data, OUT NativeMemInfo_t &info) const override;


	private:
		bool _CreateAllocator (OUT VmaAllocator &alloc) const;

		ND_ static Data *		_CastStorage (Storage_t &data);
		ND_ static Data const*	_CastStorage (const Storage_t &data);
	};


}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
