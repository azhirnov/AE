// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#ifdef AE_ENABLE_VULKAN

# include "graphics/Vulkan/Resources/VCommandPool.h"
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
	bool  VCommandPool::Create (const VDevice &dev, VQueuePtr queue, ECommandPoolType type, StringView dbgName)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( queue );
		CHECK_ERR( not IsCreated() );

		VkCommandPoolCreateInfo	info = {};
		info.sType				= VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		info.queueFamilyIndex	= uint(queue->familyIndex);
		
		BEGIN_ENUM_CHECKS();
		switch ( type )
		{
			case ECommandPoolType::Primary_OneTimeSubmit :
				info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
				break;

			case ECommandPoolType::Secondary_CachedRenderCommands :
				info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
				break;

			case ECommandPoolType::_Count :
			case ECommandPoolType::Unknown :
			default :
				RETURN_ERR( "unknown command pool type" );
		}
		END_ENUM_CHECKS();

		VK_CHECK( dev.vkCreateCommandPool( dev.GetVkDevice(), &info, null, OUT &_pool ));

		if ( dbgName.size() )
			dev.SetObjectName( uint64_t(_pool), dbgName, VK_OBJECT_TYPE_COMMAND_POOL );

		_type = type;
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

		_available.Clear();

		if ( _pool )
			dev.vkDestroyCommandPool( dev.GetVkDevice(), _pool, null );
		
		_pool = VK_NULL_HANDLE;
		_type = Default;
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
	Recycle
=================================================
*/
	void  VCommandPool::Recycle (const VDevice &dev, VkCommandBuffer cmd)
	{
		if ( not _available.Put( cmd ))
		{
			Dealloc( dev, cmd );
		}
	}

/*
=================================================
	Allocate
=================================================
*/
	VkCommandBuffer  VCommandPool::Alloc (const VDevice &dev)
	{
		CHECK_ERR( IsCreated(), VK_NULL_HANDLE );
		
		// use cache
		{
			VkCommandBuffer  cmd;
			if ( _available.Extract( OUT cmd ))
				return cmd;
		}

		EXLOCK( _drCheck );

		VkCommandBufferAllocateInfo	info = {};
		info.sType				= VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		info.commandPool		= _pool;
		info.commandBufferCount	= 1;
		
		BEGIN_ENUM_CHECKS();
		switch ( _type )
		{
			case ECommandPoolType::Primary_OneTimeSubmit :
				info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
				break;

			case ECommandPoolType::Secondary_CachedRenderCommands :
				info.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
				break;

			case ECommandPoolType::_Count :
			case ECommandPoolType::Unknown :
			default :
				RETURN_ERR( "unknown command pool type" );
		}
		END_ENUM_CHECKS();

		VkCommandBuffer	cmd = VK_NULL_HANDLE;
		VK_CHECK( dev.vkAllocateCommandBuffers( dev.GetVkDevice(), &info, OUT &cmd ));

		return cmd;
	}
	
/*
=================================================
	Deallocate
=================================================
*/
	void  VCommandPool::Dealloc (const VDevice &dev, VkCommandBuffer cmd)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( IsCreated(), void());

		dev.vkFreeCommandBuffers( dev.GetVkDevice(), _pool, 1, &cmd );
	}
//-----------------------------------------------------------------------------

	
	
/*
=================================================
	constructor
=================================================
*/
	VCommandPoolManager::VCommandPoolManager (const VDevice &dev) : _device{dev}
	{
		for (auto& queue : _cmdPools)
		{
			for (auto& storage : queue)
			{
				for (auto& pool : storage)
				{
					pool.store( null, EMemoryOrder::Relaxed );
				}
			}
		}
	}

/*
=================================================
	destructor
=================================================
*/
	VCommandPoolManager::~VCommandPoolManager ()
	{
		// destroy all command pools
		for (auto& queue : _cmdPools)
		{
			for (auto& storage : queue)
			{
				for (auto& pool : storage)
				{
					auto*	ptr = pool.exchange( null, EMemoryOrder::Acquire );
					if ( ptr )
					{
						DEBUG_ONLY(
							CHECK( ptr->Lock() );
							ptr->Unlock();
						)

						ptr->Destroy( _device );
						ptr->ReleaseRef();
					}
				}
			}
		}
	}

/*
=================================================
	Acquire
----
	TODO: optimize
	map where key = thread_id ?
=================================================
*/
	VCommandPoolPtr  VCommandPoolManager::Acquire (VQueuePtr queue, ECommandPoolType type)
	{
		CHECK_ERR( queue );
		CHECK_ERR( uint(queue->familyIndex) < _cmdPools.size() );

		auto&			storage = _cmdPools [uint(queue->familyIndex)] [uint(type)];
		VCommandPoolPtr	new_pool;

		for (uint j = 0; j < 2; ++j)
		{
			for (size_t i = 0; i < storage.size(); ++i)
			{
				VCommandPool*	cmd_pool = storage[i].load( EMemoryOrder::Relaxed );	// cache will be invalidated on 'Lock()' call

				if ( cmd_pool )
				{
					if ( cmd_pool->Lock() )
						return VCommandPoolPtr{ cmd_pool };

					continue;
				}

				// create new
				if ( not new_pool )
				{
					new_pool = Threading::MakeRC<VCommandPool>();
					CHECK_ERR( new_pool->Create( _device, queue, type ));
				}

				// if another thread inserted value to this cell then keep 'new_pool' for next cell
				VCommandPool*	expected = null;
				if ( storage[i].compare_exchange_strong( INOUT expected, new_pool.get(), EMemoryOrder::Release ))	// just flush cache
					Unused( new_pool.release() );

				--i;
			}
			
			// some cmd pools may be unlocked after first check, so wait and try again
			std::this_thread::yield();
		}

		RETURN_ERR( "failed to acquire command pool" );
	}


}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
