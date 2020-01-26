// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#ifdef AE_ENABLE_VULKAN

# include "graphics/Vulkan/RenderGraph/VCommandPool.h"
# include "graphics/Vulkan/VDevice.h"

namespace AE::Graphics
{

/*
=================================================
	destructor
=================================================
*/
	VCommandPool::~VCommandPool ()
	{
		CHECK( not _pool );
	}

/*
=================================================
	Create
=================================================
*/
	bool  VCommandPool::Create (const VDevice &dev, VQueuePtr queue, VkCommandPoolCreateFlags flags, StringView dbgName)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( queue );
		CHECK_ERR( not IsCreated() );

		VkCommandPoolCreateInfo	info = {};
		info.sType				= VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		info.flags				= VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | flags;
		info.queueFamilyIndex	= uint(queue->familyIndex);

		VK_CHECK( dev.vkCreateCommandPool( dev.GetVkDevice(), &info, null, OUT &_pool ));

		if ( dbgName.size() )
			dev.SetObjectName( uint64_t(_pool), dbgName, VK_OBJECT_TYPE_COMMAND_POOL );

		return true;
	}
	
/*
=================================================
	Destroy
=================================================
*/
	void  VCommandPool::Destroy (const VDevice &dev)
	{
		EXLOCK( _drCheck );

		_freePrimaries.Clear();
		_freeSecondaries.Clear();

		if ( _pool )
		{
			dev.vkDestroyCommandPool( dev.GetVkDevice(), _pool, null );
			_pool = VK_NULL_HANDLE;
		}
	}
	
/*
=================================================
	ResetAll
=================================================
*/
	void  VCommandPool::ResetAll (const VDevice &dev, VkCommandPoolResetFlags flags)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( IsCreated(), void());

		// TODO: clear cache if flag 'VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT' is not used

		VK_CALL( dev.vkResetCommandPool( dev.GetVkDevice(), _pool, flags ));
	}
	
/*
=================================================
	TrimAll
=================================================
*/
	void  VCommandPool::TrimAll (const VDevice &dev, VkCommandPoolTrimFlags flags)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( IsCreated(), void());
		CHECK_ERR( dev.GetFeatures().commandPoolTrim, void());

		dev.vkTrimCommandPoolKHR( dev.GetVkDevice(), _pool, flags );
	}

/*
=================================================
	Reset
=================================================
*/
	void  VCommandPool::Reset (const VDevice &dev, VkCommandBuffer cmd, VkCommandBufferResetFlags flags)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( IsCreated(), void());

		VK_CALL( dev.vkResetCommandBuffer( cmd, flags ));
	}
	
/*
=================================================
	RecyclePrimary / RecycleSecondary
=================================================
*/
	void  VCommandPool::RecyclePrimary (const VDevice &dev, VkCommandBuffer cmd)
	{
		if ( not _freePrimaries.Put( cmd ))
		{
			Deallocate( dev, cmd );
		}
	}
	
	void  VCommandPool::RecycleSecondary (const VDevice &dev, VkCommandBuffer cmd)
	{
		if ( not _freeSecondaries.Put( cmd ))
		{
			Deallocate( dev, cmd );
		}
	}

/*
=================================================
	AllocPrimary
=================================================
*/
	VkCommandBuffer  VCommandPool::AllocPrimary (const VDevice &dev)
	{
		CHECK_ERR( IsCreated(), VK_NULL_HANDLE );
		
		// use cache
		{
			VkCommandBuffer  cmd;
			if ( _freePrimaries.Extract( OUT cmd ))
				return cmd;
		}

		EXLOCK( _drCheck );

		VkCommandBufferAllocateInfo	info = {};
		info.sType				= VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		info.commandPool		= _pool;
		info.level				= VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		info.commandBufferCount	= 1;
		
		VkCommandBuffer	cmd = VK_NULL_HANDLE;
		VK_CHECK( dev.vkAllocateCommandBuffers( dev.GetVkDevice(), &info, OUT &cmd ));

		return cmd;
	}
	
/*
=================================================
	AllocSecondary
=================================================
*
	VkCommandBuffer  VCommandPool::AllocSecondary (const VDevice &dev)
	{
		CHECK_ERR( IsCreated(), VK_NULL_HANDLE );

		// use cache
		{
			VkCommandBuffer  cmd;
			if ( _freeSecondaries.Extract( OUT cmd ))
				return cmd;
		}
		
		EXLOCK( _drCheck );

		VkCommandBufferAllocateInfo	info = {};
		info.sType				= VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		info.commandPool		= _pool;
		info.level				= VK_COMMAND_BUFFER_LEVEL_SECONDARY;
		info.commandBufferCount	= 1;
		
		VkCommandBuffer  cmd = VK_NULL_HANDLE;
		VK_CHECK( dev.vkAllocateCommandBuffers( dev.GetVkDevice(), &info, OUT &cmd ));

		return cmd;
	}
	
/*
=================================================
	Deallocate
=================================================
*/
	void  VCommandPool::Deallocate (const VDevice &dev, VkCommandBuffer cmd)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( IsCreated(), void());

		dev.vkFreeCommandBuffers( dev.GetVkDevice(), _pool, 1, &cmd );
	}


}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
