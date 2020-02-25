// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef AE_ENABLE_VULKAN

# include "graphics/Vulkan/VQueue.h"
# include "threading/Containers/LfStaticPool.h"

namespace AE::Graphics
{
	class VCommandPool;
	class VCommandPoolManager;
	
	using VCommandPoolPtr = RC<VCommandPool>;
	

	enum class ECommandPoolType : uint
	{
		Primary_OneTimeSubmit,
		Secondary_CachedRenderCommands,
		_Count,
		Unknown	= ~0u,
	};



	//
	// Vulkan Command Pool
	//

	class VCommandPool final : public EnableRC< VCommandPool >
	{
	// types
	private:
		using CmdBufStorage_t	= Threading::LfStaticPool< VkCommandBuffer, 64 >;


	// variables
	private:
		VkCommandPool		_pool		= VK_NULL_HANDLE;
		ECommandPoolType	_type		= Default;

		CmdBufStorage_t		_available;

		RecursiveMutex		_lock;
		
		RWDataRaceCheck		_drCheck;	// command pool must be externally synchronized, so check it


	// methods
	public:
		VCommandPool () {}
		~VCommandPool ();

		bool  Create (const VDevice &dev, VQueuePtr queue, ECommandPoolType type, StringView dbgName = Default);
		void  Destroy (const VDevice &dev);
		
		// use command pool only in single thread, lock when start recording command buffer, unlock at end.
		ND_ bool  Lock ();
			void  Unlock ();

		ND_ VkCommandBuffer  Allocate (const VDevice &dev);

		void  Deallocate (const VDevice &dev, VkCommandBuffer cmd);

		void  ResetAll (const VDevice &dev, VkCommandPoolResetFlags flags);
		void  TrimAll (const VDevice &dev, VkCommandPoolTrimFlags flags);

		void  Reset (const VDevice &dev, VkCommandBuffer cmd, VkCommandBufferResetFlags flags);

		void  Recycle (const VDevice &dev, VkCommandBuffer cmd);

		ND_ bool				IsCreated ()	const	{ SHAREDLOCK( _drCheck );  return _pool; }
		ND_ ECommandPoolType	GetType ()		const	{ SHAREDLOCK( _drCheck );  return _type; }
	};



	//
	// Vulkan Command Pool Manager
	//

	class VCommandPoolManager final : public EnableRC< VCommandPoolManager >
	{
	// types
	private:
		using Storage_t			= StaticArray< Atomic<VCommandPool *>, 64 >;
		using CmdPoolTypes_t	= StaticArray< Storage_t, uint(ECommandPoolType::_Count) >;
		using PerQueue_t		= StaticArray< CmdPoolTypes_t, 6 >;

		STATIC_ASSERT( Storage_t::value_type::is_always_lock_free );


	// variables
	private:
		VDevice const&	_device;
		PerQueue_t		_cmdPools;


	// methods
	public:
		VCommandPoolManager (const VDevice &dev);
		~VCommandPoolManager ();

		ND_ VCommandPoolPtr  Acquire (VQueuePtr queue, ECommandPoolType type);
	};
//-----------------------------------------------------------------------------

	
	
	inline bool  VCommandPool::Lock ()
	{
		return _lock.try_lock();
	}
	
	inline void  VCommandPool::Unlock ()
	{
		_lock.unlock();
	}


}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
