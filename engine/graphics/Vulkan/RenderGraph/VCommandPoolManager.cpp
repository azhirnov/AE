// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#ifdef AE_ENABLE_VULKAN
# include "graphics/Vulkan/RenderGraph/VCommandPoolManager.h"
# include "graphics/Vulkan/VRenderGraph.h"

namespace AE::Graphics
{
	
/*
=================================================
	constructor
=================================================
*/
	VCommandBuffer::VCommandBuffer (VkCommandBuffer cmdbuf, EQueueType queueType, ECommandBufferType cmdType, RecursiveMutex& lock) :
		_cmdbuf{ cmdbuf },
		_queueType{ queueType },
		_cmdType{ cmdType },
		_lock{ &lock }
	{
		ASSERT( _cmdbuf != VK_NULL_HANDLE );
		_lock->lock();
	}
	
	VCommandBuffer::VCommandBuffer (VCommandBuffer && other) :
		_cmdbuf{ other._cmdbuf },
		_queueType{ other._queueType },
		_cmdType{ other._cmdType },
		_lock{ other._lock }
	{
		other._cmdbuf	= VK_NULL_HANDLE;
		other._lock		= null;
	}

/*
=================================================
	destructor
=================================================
*/
	VCommandBuffer::~VCommandBuffer ()
	{
		Release();
	}
	
/*
=================================================
	Release
=================================================
*/
	void  VCommandBuffer::Release ()
	{
		if ( _lock )
		{
			ASSERT( _cmdbuf != VK_NULL_HANDLE );
			_cmdbuf = VK_NULL_HANDLE;
			_lock->unlock();
			_lock = null;
		}
	}

/*
=================================================
	GetQueue
=================================================
*/
	VQueuePtr  VCommandBuffer::GetQueue () const
	{
		return RenderGraph().GetDevice().GetQueue( _queueType );
	}
//-----------------------------------------------------------------------------



/*
=================================================
	constructor
=================================================
*/
	VCommandPoolManager::VCommandPoolManager (const VDevice &dev) :
		_device{ dev }
	{
		VulkanDeviceFn_Init( _device );

		for (auto& frame : _perFrame)
		{
			for (uint q = 0; q < uint(EQueueType::_Count); ++q)
			{
				frame[q].queue = _device.GetQueue( EQueueType(q) );
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
		for (uint i = 0; i < GraphicsConfig::MaxFrames; ++i)
		{
			CHECK( ReleaseResources( i ));
		}
	}
	
/*
=================================================
	NextFrame
=================================================
*/
	bool  VCommandPoolManager::NextFrame (uint frameId)
	{
		_frameId = frameId;

		auto&	per_frame_queues = _perFrame[ _frameId ];

		for (auto& queue : per_frame_queues)
		{
			const uint	pool_count = Min( POOL_COUNT, queue.poolCount.load( EMemoryOrder::Relaxed ));
				
			for (uint i = 0; i < pool_count; ++i)
			{
				// reset command pool and free command buffers
				auto&	pool = queue.pools[i];

				CHECK_ERR( pool.lock.try_lock() );

				const uint	cmd_count = Min( CMD_COUNT, pool.count.exchange( 0, EMemoryOrder::Relaxed ));

				vkFreeCommandBuffers( _device.GetVkDevice(), pool.handle, cmd_count, pool.buffers.data() );	// TODO: free or reset ?
					
				VK_CHECK( vkResetCommandPool( _device.GetVkDevice(), pool.handle, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT ));	// TODO: check flags

				pool.lock.unlock();
			}

			ASSERT( queue.poolCount.load( EMemoryOrder::Relaxed ) == pool_count );
		}
		return true;
	}
	
/*
=================================================
	ReleaseResources
=================================================
*/
	bool  VCommandPoolManager::ReleaseResources (uint frameId)
	{
		auto&	per_frame_queues = _perFrame[ frameId ];

		for (auto& queue : per_frame_queues)
		{
			const uint	pool_count = Min( POOL_COUNT, queue.poolCount.exchange( 0, EMemoryOrder::Relaxed ));
				
			for (uint i = 0; i < pool_count; ++i)
			{
				// free command buffers and destroy pool
				auto&	pool = queue.pools[i];

				CHECK_ERR( pool.lock.try_lock() );

				const uint	cmd_count = Min( CMD_COUNT, pool.count.exchange( 0, EMemoryOrder::Relaxed ));

				vkFreeCommandBuffers( _device.GetVkDevice(), pool.handle, cmd_count, pool.buffers.data() );

				vkDestroyCommandPool( _device.GetVkDevice(), pool.handle, null );
				pool.handle = VK_NULL_HANDLE;

				pool.lock.unlock();
			}

			ASSERT( queue.poolCount.load( EMemoryOrder::Relaxed ) == 0 );
		}
		return true;
	}

/*
=================================================
	GetCommandBuffer
=================================================
*/
	VCommandBuffer  VCommandPoolManager::GetCommandBuffer (EQueueType queueType, ECommandBufferType cmdType)
	{
		auto&	per_frame_queues	= _perFrame[ _frameId ];
		auto&	queue				= per_frame_queues[ uint(queueType) ];
		uint	pool_count			= queue.poolCount.load( EMemoryOrder::Relaxed );
		uint	pool_idx			= 0;

		CHECK_ERR( queue.queue );

		const auto	CreateCmdBuf = [this, queueType, cmdType] (auto& pool, uint index) -> VCommandBuffer
		{
			VkCommandBufferAllocateInfo	info = {};
			info.sType				= VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			info.commandPool		= pool.handle;
			info.commandBufferCount	= 1;

			BEGIN_ENUM_CHECKS();
			switch ( cmdType )
			{
				case ECommandBufferType::Primary_OneTimeSubmit :
					info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
					break;
				case ECommandBufferType::Secondary_RenderCommands :
					info.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
					// TODO: inheritance info
					break;
				case ECommandBufferType::_Count :
				case ECommandBufferType::Unknown :
					ASSERT( !"unknown command buffer type" );
					break;
			}
			END_ENUM_CHECKS();

			VkCommandBuffer	cmd = VK_NULL_HANDLE;
			VK_CHECK( vkAllocateCommandBuffers( _device.GetVkDevice(), &info, OUT &cmd ));

			VkCommandBufferBeginInfo	begin = {};
			begin.sType	= VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			begin.flags	= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

			VK_CHECK( vkBeginCommandBuffer( cmd, &begin ));

			pool.buffers[index] = cmd;
			return VCommandBuffer{ cmd, queueType, cmdType, pool.lock };
		};

		for (uint i = 0; i < 2; ++i)
		{
			if ( i == 1 )
				pool_count = queue.poolCount.load( EMemoryOrder::Relaxed );

			for (; pool_idx < pool_count; ++pool_idx)
			{
				auto&	pool = queue.pools[pool_idx];

				if ( not pool.lock.try_lock() )
					continue;

				std::unique_lock lock{ pool.lock, std::adopt_lock };

				if ( pool.handle == VK_NULL_HANDLE )
					continue; // not created yet

				const uint	index = pool.count.fetch_add( 1, EMemoryOrder::Relaxed );

				if ( index < CMD_COUNT )
					return CreateCmdBuf( pool, index );
			}
		}

		// create new pool
		{
			pool_idx = queue.poolCount.fetch_add( 1, EMemoryOrder::Relaxed );
			CHECK_ERR( pool_idx < POOL_COUNT );
			
			auto&	pool = queue.pools[pool_idx];
			EXLOCK( pool.lock );
			ASSERT( pool.handle == VK_NULL_HANDLE );

			VkCommandPoolCreateInfo	info = {};
			info.sType				= VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			info.queueFamilyIndex	= uint(queue.queue->familyIndex);
			info.flags				= 0;

			VK_CHECK( vkCreateCommandPool( _device.GetVkDevice(), &info, null, OUT &pool.handle ));

			// TODO
			//if ( dbgName.size() )
			//	_device.SetObjectName( uint64_t(pool.handle), dbgName, VK_OBJECT_TYPE_COMMAND_POOL );

			const uint	index = pool.count.fetch_add( 1, EMemoryOrder::Relaxed );

			if ( index < CMD_COUNT )
				return CreateCmdBuf( pool, index );
		}

		RETURN_ERR( "failed to allocate command buffer" );
	}
	

}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
