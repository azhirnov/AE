// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef AE_ENABLE_VULKAN

# include "graphics/Public/GfxMemAllocator.h"
# include "graphics/Vulkan/VCommon.h"

namespace AE::Graphics
{

	//
	// Vulkan Small Memory Allocator
	//

	class VSmallMemAllocator final : public IGfxMemAllocator
	{
	// types
	private:
		struct Data
		{
		};


	// variables
	private:
		VDevice const&		_device;


	// methods
	public:
		explicit VSmallMemAllocator (const VDevice &dev);
		~VSmallMemAllocator () override;
		
		bool AllocForImage (const NativeImageHandle_t &image, const ImageDesc &desc, OUT Storage_t &data) override;
		bool AllocForBuffer (const NativeBufferHandle_t &buffer, const BufferDesc &desc, OUT Storage_t &data) override;
		//bool AllocateForAccelStruct (const NativeAccelStructHandle_t &as, EMemoryType memType, OUT Storage_t &data) override;

		bool Dealloc (INOUT Storage_t &data) override;
			
		bool GetInfo (const Storage_t &data, OUT NativeMemInfo_t &info) const override;


	private:
		ND_ static Data *		_CastStorage (Storage_t &data);
		ND_ static Data const*	_CastStorage (const Storage_t &data);
	};


}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
