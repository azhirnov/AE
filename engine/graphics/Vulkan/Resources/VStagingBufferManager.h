// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef AE_ENABLE_VULKAN

# include "graphics/Public/CommandBufferTypes.h"
# include "graphics/Vulkan/VDevice.h"

namespace AE::Graphics
{

	//
	// Vulkan Staging Buffer Manager
	//

	class VStagingBufferManager final
	{
	// types
	public:
		struct StagingBufferResult
		{
			//BufferID					buffer;
			VkBuffer					buffer		= VK_NULL_HANDLE;
			VkDeviceMemory				memory		= VK_NULL_HANDLE;
			Bytes						bufferOffset;
			Bytes						memoryOffset;
			Bytes						size;
			void *						mapped		= null;
			VkMemoryPropertyFlagBits	memFlags	= Zero;
		};

	private:
		class StaticMemAllocator;
		class DynamicMemAllocator;

		struct StaticBuffer
		{
			UniqueID<BufferID>	buffer;
			VkBuffer			bufferHandle	= VK_NULL_HANDLE;
			Atomic<uint>		size			{0};
			uint				capacity		= 0;
			uint				memOffset		= 0;
			void*				mapped			= null;
		};
		using PerFrame_t		= FixedArray< StaticBuffer, GraphicsConfig::MaxFrames * uint(EQueueType::_Count) >;
		using SizePerQueue_t	= GraphicsCreateInfo::SizePerQueue_t;

		static constexpr uint	QueueCount = uint(EQueueType::_Count);


	// variables
	private:
		PerFrame_t					_staticBuffersForWrite;
		PerFrame_t					_staticBuffersForRead;
		VkDeviceMemory				_staticMemoryForWrite		= VK_NULL_HANDLE;
		VkDeviceMemory				_staticMemoryForRead		= VK_NULL_HANDLE;
		VkMemoryPropertyFlagBits	_staticMemoryFlagsForWrite	= Zero;
		VkMemoryPropertyFlagBits	_staticMemoryFlagsForRead	= Zero;

		SizePerQueue_t				_writeStaticSize;
		SizePerQueue_t				_readStaticSize;
		Bytes						_maxWriteDynamicSize;
		Bytes						_maxReadDynamicSize;
		Bytes						_maxHostMemory;

		VResourceManagerImpl&		_resMngr;


	// methods
	public:
		explicit VStagingBufferManager (VResourceManagerImpl &resMngr);
		~VStagingBufferManager ();

		bool  Initialize (const GraphicsCreateInfo &info);
		void  Deinitialize ();
		
		void  OnNextFrame (uint frameId);

		bool  GetStagingBuffer (OUT StagingBufferResult& res, Bytes reqSize, Bytes blockSize, Bytes offsetAlign,
								uint frameId, EStagingHeapType heap, EQueueType queue, bool upload);

	private:
		bool  _CheckHostVisibleMemory (OUT Bytes& totalHostMem) const;
		bool  _CreateStaticBuffers (const GraphicsCreateInfo &info);
	};
	

}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
