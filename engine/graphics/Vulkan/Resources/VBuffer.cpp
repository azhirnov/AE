// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#ifdef AE_ENABLE_VULKAN

# include "graphics/Vulkan/Resources/VBuffer.h"
# include "graphics/Vulkan/VResourceManager.h"
# include "graphics/Vulkan/VEnumCast.h"

namespace AE::Graphics
{

/*
=================================================
	destructor
=================================================
*/
	VBuffer::~VBuffer ()
	{
		EXLOCK( _drCheck );
		ASSERT( _buffer == VK_NULL_HANDLE );
		ASSERT( _viewMap.empty() );
	}
	
/*
=================================================
	GetAllBufferReadAccessMasks
=================================================
*
	static VkAccessFlags  GetAllBufferReadAccessMasks (VkBufferUsageFlags usage)
	{
		VkAccessFlags	result = 0;

		for (VkBufferUsageFlags t = 1; t <= usage; t <<= 1)
		{
			if ( not EnumEq( usage, t ) )
				continue;

			BEGIN_ENUM_CHECKS();
			switch ( VkBufferUsageFlagBits(t) )
			{
				case VK_BUFFER_USAGE_TRANSFER_SRC_BIT :			result |= VK_ACCESS_TRANSFER_READ_BIT;			break;
				case VK_BUFFER_USAGE_TRANSFER_DST_BIT :			break;
				case VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT :	result |= VK_ACCESS_SHADER_READ_BIT;			break;
				case VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT :	result |= VK_ACCESS_SHADER_READ_BIT;			break;
				case VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT :		result |= VK_ACCESS_UNIFORM_READ_BIT;			break;
				case VK_BUFFER_USAGE_STORAGE_BUFFER_BIT :		result |= VK_ACCESS_SHADER_READ_BIT;			break;
				case VK_BUFFER_USAGE_INDEX_BUFFER_BIT :			result |= VK_ACCESS_INDEX_READ_BIT;				break;
				case VK_BUFFER_USAGE_VERTEX_BUFFER_BIT :		result |= VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;	break;
				case VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT :		result |= VK_ACCESS_INDIRECT_COMMAND_READ_BIT;	break;
				case VK_BUFFER_USAGE_RAY_TRACING_BIT_NV :		break;
				case VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_BUFFER_BIT_EXT :
				case VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_COUNTER_BUFFER_BIT_EXT :
				case VK_BUFFER_USAGE_CONDITIONAL_RENDERING_BIT_EXT :
				case VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_EXT :
				case VK_BUFFER_USAGE_FLAG_BITS_MAX_ENUM :		break;	// to shutup compiler warnings
			}
			END_ENUM_CHECKS();
		}
		return result;
	}

/*
=================================================
	Create
=================================================
*/
	bool VBuffer::Create (VResourceManager &resMngr, const BufferDesc &desc, GfxMemAllocatorPtr allocator, StringView dbgName)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( _buffer == VK_NULL_HANDLE );
		CHECK_ERR( allocator );

		auto&	dev = resMngr.GetDevice();

		_desc = desc;

		// create buffer
		VkBufferCreateInfo	info = {};
		info.sType	= VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		info.pNext	= null;
		info.flags	= 0;
		info.usage	= VEnumCast( _desc.usage );
		info.size	= VkDeviceSize( _desc.size );

		VQueueFamilyIndices_t	queue_family_indices;

		// setup sharing mode
		if ( _desc.queues != Default )
		{
			dev.GetQueueFamilies( _desc.queues, OUT queue_family_indices );

			info.sharingMode			= VK_SHARING_MODE_CONCURRENT;
			info.pQueueFamilyIndices	= queue_family_indices.data();
			info.queueFamilyIndexCount	= uint(queue_family_indices.size());
		}

		// reset to exclusive mode
		if ( info.queueFamilyIndexCount <= 1 )
		{
			info.sharingMode			= VK_SHARING_MODE_EXCLUSIVE;
			info.pQueueFamilyIndices	= null;
			info.queueFamilyIndexCount	= 0;
		}

		VK_CHECK( dev.vkCreateBuffer( dev.GetVkDevice(), &info, null, OUT &_buffer ));

		CHECK_ERR( allocator->AllocForBuffer( BitCast<BufferVk_t>(_buffer), _desc, INOUT _memStorage ));

		if ( not dbgName.empty() )
		{
			dev.SetObjectName( BitCast<uint64_t>(_buffer), dbgName, VK_OBJECT_TYPE_BUFFER );
		}
		
		_memAllocator		= std::move(allocator);
		//_readAccessMask	= GetAllBufferReadAccessMasks( info.usage );
		_debugName			= dbgName;

		return true;
	}
	
/*
=================================================
	Create
=================================================
*
	bool VBuffer::Create (const VDevice &dev, const VulkanBufferDesc &desc, StringView dbgName, OnRelease_t &&onRelease)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( _buffer == VK_NULL_HANDLE );

		_buffer		= BitCast<VkBuffer>( desc.buffer );
		_desc.size	= desc.size;
		_desc.usage	= FGEnumCast( BitCast<VkBufferUsageFlagBits>( desc.usage ));

		if ( not dbgName.empty() )
		{
			dev.SetObjectName( BitCast<uint64_t>(_buffer), dbgName, VK_OBJECT_TYPE_BUFFER );
		}
		
		CHECK( desc.queueFamily == VK_QUEUE_FAMILY_IGNORED );	// not supported yet
		CHECK( desc.queueFamilyIndices.empty() or desc.queueFamilyIndices.size() >= 2 );

		_queueFamilyMask = Default;

		for (auto idx : desc.queueFamilyIndices) {
			_queueFamilyMask |= BitCast<EQueueFamily>(idx);
		}

		_debugName	= dbgName;
		_onRelease	= std::move(onRelease);

		return true;
	}

/*
=================================================
	Destroy
=================================================
*/
	void VBuffer::Destroy (VResourceManager &resMngr)
	{
		EXLOCK( _drCheck );

		auto&	dev = resMngr.GetDevice();

		for (auto& view : _viewMap) {
			dev.vkDestroyBufferView( dev.GetVkDevice(), view.second, null );
		}

		if ( _buffer ) {
			dev.vkDestroyBuffer( dev.GetVkDevice(), _buffer, null );
		}

		if ( _memAllocator ) {
			CHECK( _memAllocator->Dealloc( INOUT _memStorage ));
		}

		_viewMap.clear();
		_debugName.clear();

		_memAllocator	= null;
		_buffer			= VK_NULL_HANDLE;
		_desc			= Default;
	}
	
/*
=================================================
	GetMemoryInfo
=================================================
*/
	bool VBuffer::GetMemoryInfo (OUT VResourceMemoryInfo &outInfo) const
	{
		SHAREDLOCK( _drCheck );
		CHECK_ERR( _memAllocator );

		IGfxMemAllocator::NativeMemInfo_t	info;
		CHECK_ERR( _memAllocator->GetInfo( _memStorage, OUT info ));

		auto*	vk_info = UnionGetIf<VulkanMemoryObjInfo>( &info );
		CHECK_ERR( vk_info );

		outInfo.memory		= BitCast<VkDeviceMemory>( vk_info->memory );
		outInfo.flags		= BitCast<VkMemoryPropertyFlagBits>( vk_info->flags );
		outInfo.offset		= vk_info->offset;
		outInfo.size		= vk_info->size;
		outInfo.mappedPtr	= vk_info->mappedPtr;
		return true;
	}

/*
=================================================
	GetView
=================================================
*/
	VkBufferView  VBuffer::GetView (const VDevice &dev, const BufferViewDesc &desc) const
	{
		SHAREDLOCK( _drCheck );

		// find already created image view
		{
			SHAREDLOCK( _viewMapLock );

			auto	iter = _viewMap.find( desc );

			if ( iter != _viewMap.end() )
				return iter->second;
		}

		// create new image view
		EXLOCK( _viewMapLock );

		auto[iter, inserted] = _viewMap.insert({ desc, VK_NULL_HANDLE });

		if ( not inserted )
			return iter->second;	// other thread create view before
		
		CHECK_ERR( _CreateView( dev, desc, OUT iter->second ));

		return iter->second;
	}
	
/*
=================================================
	_CreateView
=================================================
*/
	bool  VBuffer::_CreateView (const VDevice &dev, const BufferViewDesc &desc, OUT VkBufferView &outView) const
	{
		VkBufferViewCreateInfo	info = {};
		info.sType		= VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
		info.flags		= 0;
		info.buffer		= _buffer;
		info.format		= VEnumCast( desc.format );
		info.offset		= VkDeviceSize(desc.offset);
		info.range		= VkDeviceSize(desc.size);

		VK_CHECK( dev.vkCreateBufferView( dev.GetVkDevice(), &info, null, OUT &outView ));
		return true;
	}

/*
=================================================
	IsReadOnly
=================================================
*/
	bool  VBuffer::IsReadOnly () const
	{
		SHAREDLOCK( _drCheck );
		constexpr auto	mask = EBufferUsage::TransferDst | EBufferUsage::StorageTexel | EBufferUsage::Storage | EBufferUsage::RayTracing;

		return not EnumAny( _desc.usage, mask );
	}
	
/*
=================================================
	IsOwnsHandle
=================================================
*/
	bool  VBuffer::IsOwnsHandle () const
	{
		return false;
		//return bool(_onRelease);
	}

/*
=================================================
	GetNativeDescription
=================================================
*/
	VulkanBufferDesc  VBuffer::GetNativeDescription () const
	{
		VulkanBufferDesc	desc;
		desc.buffer			= BitCast<BufferVk_t>( _buffer );
		desc.usage			= BitCast<BufferUsageFlagsVk_t>( VEnumCast( _desc.usage ));
		desc.size			= _desc.size;
		//desc.queueFamily	= VK_QUEUE_FAMILY_IGNORED;
		//desc.queueFamilyIndices	// TODO
		return desc;
	}
	
/*
=================================================
	IsSupported
=================================================
*/
	bool  VBuffer::IsSupported (const VDevice &dev, const BufferDesc &desc)
	{
		Unused( dev, desc );
		return true;
	}
	
/*
=================================================
	IsSupported
=================================================
*/
	bool  VBuffer::IsSupported (const VDevice &dev, const BufferViewDesc &desc) const
	{
		SHAREDLOCK( _drCheck );

		VkFormatProperties	props = {};
		vkGetPhysicalDeviceFormatProperties( dev.GetVkPhysicalDevice(), VEnumCast( desc.format ), OUT &props );
		
		const VkFormatFeatureFlags	dev_flags	= props.bufferFeatures;
		VkFormatFeatureFlags		buf_flags	= 0;
		
		for (EBufferUsage t = EBufferUsage(1); t <= _desc.usage; t = EBufferUsage(uint(t) << 1))
		{
			if ( not EnumEq( _desc.usage, t ))
				continue;

			BEGIN_ENUM_CHECKS();
			switch ( t )
			{
				case EBufferUsage::UniformTexel :		buf_flags |= VK_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER_BIT;	break;
				case EBufferUsage::StorageTexel :		buf_flags |= VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_BIT | VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_ATOMIC_BIT;	break;
				case EBufferUsage::TransferSrc :		break;
				case EBufferUsage::TransferDst :		break;
				case EBufferUsage::Uniform :			break;
				case EBufferUsage::Storage :			break;
				case EBufferUsage::Index :				break;
				case EBufferUsage::Vertex :				break;
				case EBufferUsage::Indirect :			break;
				case EBufferUsage::RayTracing :			break;
				case EBufferUsage::ShaderAddress :		break;
				case EBufferUsage::_Last :
				case EBufferUsage::All :
				case EBufferUsage::Transfer :
				case EBufferUsage::Unknown :
				default :								ASSERT(false);	break;
			}
			END_ENUM_CHECKS();
		}

		return (dev_flags & buf_flags);
	}


}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
