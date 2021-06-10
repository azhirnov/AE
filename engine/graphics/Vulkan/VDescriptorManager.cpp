// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#ifdef AE_ENABLE_VULKAN

# include "graphics/Vulkan/VDescriptorManager.h"
# include "graphics/Vulkan/VResourceManager.h"

namespace AE::Graphics
{

/*
=================================================
	constructor
=================================================
*/
	VDescriptorManager::VDescriptorManager (VResourceManagerImpl &resMngr) :
		_resMngr{ resMngr }
	{
		//STATIC_ASSERT( DSCachePool_t::capacity() == VResourceManagerImpl::DSLayoutPool_t::capacity() );
	}
	
/*
=================================================
	destructor
=================================================
*/
	VDescriptorManager::~VDescriptorManager ()
	{
		CHECK( _dsPools.empty() );
	}
	
/*
=================================================
	Initialize
=================================================
*/
	bool  VDescriptorManager::Initialize ()
	{
		EXLOCK( _dsPoolGuard );

		CHECK_ERR( _CreateDescriptorPool() );
		return true;
	}
	
/*
=================================================
	Deinitialize
=================================================
*/
	void  VDescriptorManager::Deinitialize ()
	{
		EXLOCK( _dsPoolGuard );

		auto&	dev = _resMngr.GetDevice();

		for (auto& item : _dsPools)
		{
			if ( item.pool )
			{
				dev.vkDestroyDescriptorPool( dev.GetVkDevice(), item.pool, null );
				item.pool = VK_NULL_HANDLE;
			}
		}
		_dsPools.clear();
	}
	
/*
=================================================
	AllocDescriptorSet
=================================================
*/
	bool  VDescriptorManager::AllocDescriptorSet (DescriptorSetLayoutID layoutId, OUT DescSetRef &ds)
	{
		// add to cache
		/*DSCache_t*	cache_ptr = null;

		if ( _dsCache.AssignAt( layoutId.Index(), OUT cache_ptr ))
		{
			if ( cache_ptr->Extract( OUT ds ))
				return true;
		}*/


		auto&	dev		= _resMngr.GetDevice();
		auto*	layout	= _resMngr.GetResource( layoutId );
		CHECK_ERR( layout );

		EXLOCK( _dsPoolGuard );

		VkDescriptorSetLayout			ds_layout	= layout->Handle();
		VkDescriptorSetAllocateInfo		info		= {};

		info.sType				= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		info.descriptorSetCount	= 1;
		info.pSetLayouts		= &ds_layout;

		for (auto& item : _dsPools)
		{
			info.descriptorPool = item.pool;
			
			if ( dev.vkAllocateDescriptorSets( dev.GetVkDevice(), &info, OUT &ds.handle ) == VK_SUCCESS )
			{
				ds.poolIdx = uint8_t(Distance( _dsPools.data(), &item ));
				return true;
			}
		}

		CHECK_ERR( _CreateDescriptorPool() );
		
		info.descriptorPool = _dsPools.back().pool;
		VK_CHECK( dev.vkAllocateDescriptorSets( dev.GetVkDevice(), &info, OUT &ds.handle ));
		ds.poolIdx = uint8_t(_dsPools.size() - 1);

		return true;
	}
	
/*
=================================================
	DeallocDescriptorSet
=================================================
*/
	bool  VDescriptorManager::DeallocDescriptorSet (DescriptorSetLayoutID, const DescSetRef &ds)
	{
		EXLOCK( _dsPoolGuard );
		CHECK_ERR( ds.poolIdx < _dsPools.size() );
		
		auto&	dev = _resMngr.GetDevice();

		VK_CALL( dev.vkFreeDescriptorSets( dev.GetVkDevice(), _dsPools[ds.poolIdx].pool, 1, &ds.handle ));
		return true;
	}

/*
=================================================
	_CreateDescriptorPool
=================================================
*/
	bool  VDescriptorManager::_CreateDescriptorPool ()
	{
		CHECK_ERR( _dsPools.size() < _dsPools.capacity() );
		
		auto&	dev = _resMngr.GetDevice();

		FixedArray< VkDescriptorPoolSize, 32 >	pool_sizes;

		pool_sizes.push_back({ VK_DESCRIPTOR_TYPE_SAMPLER,						MaxDescriptorPoolSize });
		pool_sizes.push_back({ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,		MaxDescriptorPoolSize * 4 });
		pool_sizes.push_back({ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,				MaxDescriptorPoolSize });
		pool_sizes.push_back({ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,				MaxDescriptorPoolSize / 2 });
		pool_sizes.push_back({ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,				MaxDescriptorPoolSize * 2 });
		
		pool_sizes.push_back({ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,				MaxDescriptorPoolSize * 2 });
		pool_sizes.push_back({ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,				MaxDescriptorPoolSize * 2 });
		pool_sizes.push_back({ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,		MaxDescriptorPoolSize * 2 });
		pool_sizes.push_back({ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,		MaxDescriptorPoolSize });

		pool_sizes.push_back({ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,			MaxDescriptorPoolSize });
		pool_sizes.push_back({ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,			MaxDescriptorPoolSize });
		
	#ifdef VK_NV_ray_tracing
		if ( dev.GetFeatures().rayTracingNV ) {
			pool_sizes.push_back({ VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV, MaxDescriptorPoolSize / 2 });
		}
	#endif

		
		VkDescriptorPoolCreateInfo	info = {};
		info.sType			= VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		info.poolSizeCount	= uint(pool_sizes.size());
		info.pPoolSizes		= pool_sizes.data();
		info.maxSets		= MaxDescriptorSets;
		info.flags			= VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

		VkDescriptorPool	ds_pool;
		VK_CHECK( dev.vkCreateDescriptorPool( dev.GetVkDevice(), &info, null, OUT &ds_pool ));

		_dsPools.push_back({ ds_pool });
		return true;
	}


}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
