// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef AE_ENABLE_VULKAN

# include "graphics/Vulkan/VQueue.h"
# include "threading/Containers/LfStaticPool.h"

namespace AE::Graphics
{

	//
	// Vulkan Command Pool
	//

	class VCommandPool final
	{
	// types
	private:
		using CmdBufStorage_t	= Threading::LfStaticPool< VkCommandBuffer, 64 >;


	// variables
	private:
		VkCommandPool		_pool		= VK_NULL_HANDLE;

		CmdBufStorage_t		_freePrimaries;
		CmdBufStorage_t		_freeSecondaries;
		
		RWDataRaceCheck		_drCheck;	// command pool must be externally synchronized, so check it


	// methods
	public:
		VCommandPool () {}
		~VCommandPool ();

		bool  Create (const VDevice &dev, VQueuePtr queue, VkCommandPoolCreateFlags flags = 0, StringView dbgName = Default);
		void  Destroy (const VDevice &dev);
		
		ND_ VkCommandBuffer  AllocPrimary (const VDevice &dev);
		ND_ VkCommandBuffer  AllocSecondary (const VDevice &dev);

		void  Deallocate (const VDevice &dev, VkCommandBuffer cmd);

		void  ResetAll (const VDevice &dev, VkCommandPoolResetFlags flags);
		void  TrimAll (const VDevice &dev, VkCommandPoolTrimFlags flags);

		void  Reset (const VDevice &dev, VkCommandBuffer cmd, VkCommandBufferResetFlags flags);

		void  RecyclePrimary (const VDevice &dev, VkCommandBuffer cmd);
		void  RecycleSecondary (const VDevice &dev, VkCommandBuffer cmd);

		ND_ bool  IsCreated ()	const	{ SHAREDLOCK( _drCheck );  return _pool; }
	};


}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
