// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#ifdef AE_ENABLE_VULKAN

# include "graphics/Vulkan/Allocators/VUniMemAllocator.h"
# include "graphics/Vulkan/VDevice.h"

# define VMA_USE_STL_CONTAINERS				0
# define VMA_STATIC_VULKAN_FUNCTIONS		0
# define VMA_RECORDING_ENABLED				0
# define VMA_DEDICATED_ALLOCATION			0	// TODO: set 0 to avoid crash on Intel
# define VMA_DEBUG_INITIALIZE_ALLOCATIONS	0
# define VMA_DEBUG_ALWAYS_DEDICATED_MEMORY	0
# define VMA_DEBUG_DETECT_CORRUPTION		0	// TODO: use for debugging ?
# define VMA_DEBUG_GLOBAL_MUTEX				0	// will be externally synchronized

# define VMA_IMPLEMENTATION		1
# define VMA_ASSERT(expr)		{}

#ifdef COMPILER_MSVC
#	pragma warning (push, 0)
#	pragma warning (disable: 4701)
#	pragma warning (disable: 4703)
#endif

# include "vk_mem_alloc.h"

#ifdef COMPILER_MSVC
#	pragma warning (pop)
#endif

namespace AE::Graphics
{
namespace {
/*
=================================================
	ConvertToMemoryFlags
=================================================
*/
	VmaAllocationCreateFlags  ConvertToMemoryFlags (EMemoryType memType)
	{
		VmaAllocationCreateFlags	result = 0;

		if ( EnumEq( memType, EMemoryType::Dedicated ))
			result |= VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
		
		if ( EnumAny( memType, EMemoryType::_HostVisible ))
			result |= VMA_ALLOCATION_CREATE_MAPPED_BIT;

		// TODO: VMA_ALLOCATION_CREATE_NEVER_ALLOCATE_BIT

		return result;
	}

/*
=================================================
	ConvertToMemoryUsage
=================================================
*/
	VmaMemoryUsage  ConvertToMemoryUsage (EMemoryType memType)
	{
		if ( EnumEq( memType, EMemoryType::HostCocherent ))
			return VmaMemoryUsage::VMA_MEMORY_USAGE_GPU_TO_CPU;

		if ( EnumEq( memType, EMemoryType::HostCached ))
			return VmaMemoryUsage::VMA_MEMORY_USAGE_CPU_TO_GPU;

		return VmaMemoryUsage::VMA_MEMORY_USAGE_GPU_ONLY;
	}
	
/*
=================================================
	ConvertToMemoryProperties
=================================================
*/
	VkMemoryPropertyFlags  ConvertToMemoryProperties (EMemoryType memType)
	{
		const EMemoryType		values	= EMemoryType(memType);
		VkMemoryPropertyFlags	flags	= 0;

		for (EMemoryType t = EMemoryType(1); t <= memType; t = EMemoryType(uint(t) << 1)) 
		{
			if ( not EnumEq( values, t ) )
				continue;

			BEGIN_ENUM_CHECKS();
			switch ( t )
			{
				case EMemoryType::DeviceLocal :		flags |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;		break;
				case EMemoryType::Transient :		flags |= VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT;	break;
				case EMemoryType::HostCocherent :	flags |= VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;	break;
				case EMemoryType::HostCached :		flags |= VK_MEMORY_PROPERTY_HOST_CACHED_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;		break;
				case EMemoryType::Dedicated :
				case EMemoryType::_HostVisible :
				case EMemoryType::Shared :
				case EMemoryType::Unknown :
				default :							break;
			}
			END_ENUM_CHECKS();
		}
		return flags;
	}
}
//-----------------------------------------------------------------------------



/*
=================================================
	constructor
=================================================
*/
	VUniMemAllocator::VUniMemAllocator (const VDevice &dev) :
		_device{ dev },		_allocator{ null }
	{
		CHECK( _CreateAllocator( OUT _allocator ) );
	}
	
/*
=================================================
	destructor
=================================================
*/
	VUniMemAllocator::~VUniMemAllocator ()
	{
		if ( _allocator ) {
			vmaDestroyAllocator( _allocator );
		}
	}
	
/*
=================================================
	_CastStorage
=================================================
*/
	VUniMemAllocator::Data*  VUniMemAllocator::_CastStorage (Storage_t &data)
	{
		return data.Cast<Data>( SizeOf<uint> );
	}

	VUniMemAllocator::Data const*  VUniMemAllocator::_CastStorage (const Storage_t &data)
	{
		return data.Cast<Data>( SizeOf<uint> );
	}

/*
=================================================
	AllocForImage
=================================================
*/
	bool VUniMemAllocator::AllocForImage (const NativeImageHandle_t &imageHandle, const ImageDesc &desc, OUT Storage_t &data)
	{
		auto*	image_ptr = UnionGetIf<ImageVk_t>( &imageHandle );
		CHECK_ERR( image_ptr and *image_ptr );

		VmaAllocationCreateInfo		info = {};
		info.flags			= ConvertToMemoryFlags( desc.memType );
		info.usage			= ConvertToMemoryUsage( desc.memType );
		info.requiredFlags	= ConvertToMemoryProperties( desc.memType );
		info.preferredFlags	= 0;
		info.memoryTypeBits	= 0;
		info.pool			= VK_NULL_HANDLE;
		info.pUserData		= null;

		VkImage			image	= BitCast<VkImage>( *image_ptr );
		VmaAllocation	mem		= null;
		VK_CHECK( vmaAllocateMemoryForImage( _allocator, image, &info, OUT &mem, null ));

		VK_CHECK( vmaBindImageMemory( _allocator, mem, image ));
		
		_CastStorage( data )->allocation = mem;
		return true;
	}
	
/*
=================================================
	AllocForBuffer
=================================================
*/
	bool VUniMemAllocator::AllocForBuffer (const NativeBufferHandle_t &bufferHandle, const BufferDesc &desc, OUT Storage_t &data)
	{
		auto*	buffer_ptr = UnionGetIf<BufferVk_t>( &bufferHandle );
		CHECK_ERR( buffer_ptr and *buffer_ptr );

		VmaAllocationCreateInfo		info = {};
		info.flags			= ConvertToMemoryFlags( desc.memType );
		info.usage			= ConvertToMemoryUsage( desc.memType );
		info.requiredFlags	= ConvertToMemoryProperties( desc.memType );
		info.preferredFlags	= 0;
		info.memoryTypeBits	= 0;
		info.pool			= VK_NULL_HANDLE;
		info.pUserData		= null;

		VkBuffer		buffer	= BitCast<VkBuffer>( *buffer_ptr );
		VmaAllocation	mem		= null;
		VK_CHECK( vmaAllocateMemoryForBuffer( _allocator, buffer, &info, OUT &mem, null ));

		VK_CHECK( vmaBindBufferMemory( _allocator, mem, buffer ));
		
		_CastStorage( data )->allocation = mem;
		return true;
	}
	
/*
=================================================
	AllocateForAccelStruct
=================================================
*
	bool VUniMemAllocator::AllocateForAccelStruct (VkAccelerationStructureNV accelStruct, const MemoryDesc &desc, OUT Storage_t &data)
	{
		VkAccelerationStructureMemoryRequirementsInfoNV	mem_info = {};
		mem_info.sType					= VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
		mem_info.type					= VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_OBJECT_NV;
		mem_info.accelerationStructure	= accelStruct;

		VkMemoryRequirements2	mem_req = {};
		_device.vkGetAccelerationStructureMemoryRequirementsNV( _device.GetVkDevice(), &mem_info, OUT &mem_req );
		
		VmaAllocationCreateInfo		info = {};
		info.flags			= ConvertToMemoryFlags( desc.type );
		info.usage			= ConvertToMemoryUsage( desc.type );
		info.requiredFlags	= ConvertToMemoryProperties( desc.type );
		info.preferredFlags	= 0;
		info.memoryTypeBits	= 0;
		info.pool			= VK_NULL_HANDLE;
		info.pUserData		= null;
		
		VmaAllocation	mem = null;
		VK_CHECK( _allocator->AllocateMemory( mem_req.memoryRequirements, false, false, VK_NULL_HANDLE, VK_NULL_HANDLE, info, VMA_SUBALLOCATION_TYPE_UNKNOWN, 1, OUT &mem ));
		
		VmaAllocationInfo	alloc_info	= {};
		vmaGetAllocationInfo( _allocator, mem, OUT &alloc_info );
		
		VkBindAccelerationStructureMemoryInfoNV		bind_info = {};
		bind_info.sType					= VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_NV;
		bind_info.accelerationStructure	= accelStruct;
		bind_info.memory				= alloc_info.deviceMemory;
		bind_info.memoryOffset			= alloc_info.offset;
		VK_CHECK( _device.vkBindAccelerationStructureMemoryNV( _device.GetVkDevice(), 1, &bind_info ));

		_CastStorage( data )->allocation = mem;
		return true;
	}

/*
=================================================
	Dealloc
=================================================
*/
	bool VUniMemAllocator::Dealloc (INOUT Storage_t &data)
	{
		VmaAllocation&	mem = _CastStorage( data )->allocation;

		vmaFreeMemory( _allocator, mem );

		mem = null;
		return true;
	}
	
/*
=================================================
	GetMemoryInfo
=================================================
*/
	bool VUniMemAllocator::GetInfo (const Storage_t &data, OUT NativeMemInfo_t &outInfo) const
	{
		VmaAllocation		mem			= _CastStorage( data )->allocation;
		VmaAllocationInfo	alloc_info	= {};
		vmaGetAllocationInfo( _allocator, mem, OUT &alloc_info );
		
		const auto&		mem_props = _device.GetProperties().memoryProperties;
		ASSERT( alloc_info.memoryType < mem_props.memoryTypeCount );

		auto&	info = outInfo.emplace<VulkanMemoryObjInfo>();

		info.memory		= BitCast<DeviceMemoryVk_t>( alloc_info.deviceMemory );
		info.flags		= BitCast<MemoryPropertyVk_t>( mem_props.memoryTypes[ alloc_info.memoryType ].propertyFlags );
		info.offset		= BytesU(alloc_info.offset);
		info.size		= BytesU(alloc_info.size);
		info.mappedPtr	= alloc_info.pMappedData;
		return true;
	}

/*
=================================================
	_CreateAllocator
=================================================
*/
	bool VUniMemAllocator::_CreateAllocator (OUT VmaAllocator &alloc) const
	{
		VkDevice				dev = _device.GetVkDevice();
		VmaVulkanFunctions		funcs = {};

		funcs.vkGetPhysicalDeviceProperties			= _var_vkGetPhysicalDeviceProperties;
		funcs.vkGetPhysicalDeviceMemoryProperties	= _var_vkGetPhysicalDeviceMemoryProperties;
		funcs.vkAllocateMemory						= BitCast<PFN_vkAllocateMemory>(vkGetDeviceProcAddr( dev, "vkAllocateMemory" ));
		funcs.vkFreeMemory							= BitCast<PFN_vkFreeMemory>(vkGetDeviceProcAddr( dev, "vkFreeMemory" ));
		funcs.vkMapMemory							= BitCast<PFN_vkMapMemory>(vkGetDeviceProcAddr( dev, "vkMapMemory" ));
		funcs.vkUnmapMemory							= BitCast<PFN_vkUnmapMemory>(vkGetDeviceProcAddr( dev, "vkUnmapMemory" ));
		funcs.vkBindBufferMemory					= BitCast<PFN_vkBindBufferMemory>(vkGetDeviceProcAddr( dev, "vkBindBufferMemory" ));
		funcs.vkBindImageMemory						= BitCast<PFN_vkBindImageMemory>(vkGetDeviceProcAddr( dev, "vkBindImageMemory" ));
		funcs.vkGetBufferMemoryRequirements			= BitCast<PFN_vkGetBufferMemoryRequirements>(vkGetDeviceProcAddr( dev, "vkGetBufferMemoryRequirements" ));
		funcs.vkGetImageMemoryRequirements			= BitCast<PFN_vkGetImageMemoryRequirements>(vkGetDeviceProcAddr( dev, "vkGetImageMemoryRequirements" ));
		funcs.vkCreateBuffer						= null;
		funcs.vkDestroyBuffer						= null;
		funcs.vkCreateImage							= null;
		funcs.vkDestroyImage						= null;
		funcs.vkFlushMappedMemoryRanges				= BitCast<PFN_vkFlushMappedMemoryRanges>(vkGetDeviceProcAddr( dev, "vkFlushMappedMemoryRanges" ));
		funcs.vkInvalidateMappedMemoryRanges		= BitCast<PFN_vkInvalidateMappedMemoryRanges>(vkGetDeviceProcAddr( dev, "vkInvalidateMappedMemoryRanges" ));

	#if VMA_DEDICATED_ALLOCATION
		if ( _device.GetFeatures().bindMemory2 )
		{
			funcs.vkGetBufferMemoryRequirements2KHR		= BitCast<PFN_vkGetBufferMemoryRequirements2KHR>(vkGetDeviceProcAddr( dev, "vkGetBufferMemoryRequirements2" ));
			funcs.vkGetImageMemoryRequirements2KHR		= BitCast<PFN_vkGetImageMemoryRequirements2KHR>(vkGetDeviceProcAddr( dev, "vkGetImageMemoryRequirements2" ));
		}
		else
		{
			funcs.vkGetBufferMemoryRequirements2KHR		= BitCast<PFN_vkGetBufferMemoryRequirements2KHR>(vkGetDeviceProcAddr( dev, "vkGetBufferMemoryRequirements2KHR" ));
			funcs.vkGetImageMemoryRequirements2KHR		= BitCast<PFN_vkGetImageMemoryRequirements2KHR>(vkGetDeviceProcAddr( dev, "vkGetImageMemoryRequirements2KHR" ));
		}
	#endif

		VmaAllocatorCreateInfo	info = {};
		info.flags			= VMA_ALLOCATOR_CREATE_EXTERNALLY_SYNCHRONIZED_BIT;
		info.physicalDevice	= _device.GetVkPhysicalDevice();
		info.device			= dev;

		info.preferredLargeHeapBlockSize	= VkDeviceSize(256) << 20;
		info.pAllocationCallbacks			= null;
		info.pDeviceMemoryCallbacks			= null;
		//info.frameInUseCount	// ignore
		info.pHeapSizeLimit					= null;		// TODO
		info.pVulkanFunctions				= &funcs;

		VK_CHECK( vmaCreateAllocator( &info, OUT &alloc ));
		return true;
	}

}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
