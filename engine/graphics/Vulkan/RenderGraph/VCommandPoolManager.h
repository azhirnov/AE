// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef AE_ENABLE_VULKAN
# include "graphics/Public/CommandBufferTypes.h"
# include "graphics/Vulkan/VDevice.h"
# include "threading/Common.h"

namespace AE::Graphics
{
	using AE::Threading::RecursiveMutex;
	using AE::Threading::Atomic;



	//
	// Vulkan Command Buffer
	//

	class VCommandBuffer
	{
		friend class VCommandPoolManager;

	// variables
	private:
		VkCommandBuffer		_cmdbuf		= VK_NULL_HANDLE;
		EQueueType			_queueType	= Default;
		ECommandBufferType	_cmdType	= Default;
		RecursiveMutex *	_lock		= null;


	// methods
	protected:
		VCommandBuffer (VkCommandBuffer cmdbuf, EQueueType queueType, ECommandBufferType cmdType, RecursiveMutex& lock);

	public:
		VCommandBuffer () {}
		VCommandBuffer (VCommandBuffer &&);
		~VCommandBuffer ();

		void  Release ();

		ND_ EQueueType		GetQueueType ()	const	{ return _queueType; }
		ND_ VkCommandBuffer	Get ()			const	{ ASSERT(IsValid());  return _cmdbuf; }
		ND_ bool			IsValid ()		const	{ return _cmdbuf != VK_NULL_HANDLE; }
		ND_ VQueuePtr		GetQueue ()		const;
	};



	//
	// Vulkan Command Pool Manager
	//

	class VCommandPoolManager final : private VulkanDeviceFn
	{
	// types
	private:
		static constexpr uint	CMD_COUNT	= GraphicsConfig::MaxCmdBuffersPerPool;
		static constexpr uint	POOL_COUNT	= GraphicsConfig::MaxCmdPoolsPerQueue;

		using CmdBuffers_t = StaticArray< VkCommandBuffer, CMD_COUNT >;

		struct CmdPool
		{
			RecursiveMutex		lock;	// access to command pool and command buffers must be synchronized
			VkCommandPool		handle	= VK_NULL_HANDLE;
			Atomic<uint>		count	{0};
			CmdBuffers_t		buffers;
		};
		using CmdPools_t = StaticArray< CmdPool, POOL_COUNT >;

		struct CmdPoolPerQueue
		{
			Atomic<uint>		poolCount	{0};
			CmdPools_t			pools;
			VQueuePtr			queue;
		};

		using Queues_t	= StaticArray< CmdPoolPerQueue, uint(EQueueType::_Count) >;
		using Frames_t	= StaticArray< Queues_t, GraphicsConfig::MaxFrames >;


	// variables
	private:
		Frames_t			_perFrame;
		uint				_frameId	= 0;
		VDevice const&		_device;


	// methods
	public:
		explicit VCommandPoolManager (const VDevice &dev);
		~VCommandPoolManager ();

		bool  NextFrame (uint frameId);
		bool  ReleaseResources (uint frameId);

		ND_ VCommandBuffer	GetCommandBuffer (EQueueType queue, ECommandBufferType type);

		ND_ VDevice const&	GetDevice () const	{ return _device; }
	};
	

}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
