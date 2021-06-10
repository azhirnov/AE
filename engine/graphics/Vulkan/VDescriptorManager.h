// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef AE_ENABLE_VULKAN

# include "graphics/Vulkan/Resources/VDescriptorSetLayout.h"
# include "threading/Containers/LfIndexedPool2.h"
# include "threading/Containers/LfStaticPool.h"

namespace AE::Graphics
{

	//
	// Vulkan Descriptor Manager
	//

	class VDescriptorManager final
	{
	// types
	public:
		struct DescSetRef
		{
			VkDescriptorSet		handle	= VK_NULL_HANDLE;
			uint8_t				poolIdx	= UMax;
		};

	private:
		static constexpr uint	MaxDescriptorPoolSize	= 1u << 11;
		static constexpr uint	MaxDescriptorSets		= 1u << 10;

		struct DSPool
		{
			VkDescriptorPool	pool	= VK_NULL_HANDLE;
		};

		using DescriptorPoolArray_t		= FixedArray< DSPool, 8 >;
		using DSCache_t					= Threading::LfStaticPool< DescSetRef, 64 >;
		//using DSCachePool_t			= Threading::LfIndexedPool2< DSCache_t, DescriptorSetLayoutID::Index_t, (1u << 9), 8 >;


	// variables
	private:
		VResourceManagerImpl &		_resMngr;

		Mutex						_dsPoolGuard;
		DescriptorPoolArray_t		_dsPools;

		//DSCachePool_t				_dsCache;


	// methods
	public:
		explicit VDescriptorManager (VResourceManagerImpl &);
		~VDescriptorManager ();
		
		bool  Initialize ();
		void  Deinitialize ();

		bool  AllocDescriptorSet (DescriptorSetLayoutID layoutId, OUT DescSetRef &ds);
		bool  DeallocDescriptorSet (DescriptorSetLayoutID layoutId, const DescSetRef &ds);

	private:
		bool  _CreateDescriptorPool ();
	};


}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
