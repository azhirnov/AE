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
		ASSERT( not _memoryId );
	}
	
/*
=================================================
	GetAllBufferReadAccessMasks
=================================================
*/
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
	bool VBuffer::Create (VResourceManager &resMngr, const BufferDesc &desc, MemoryID memId, VMemoryObj &memObj, StringView dbgName)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( _buffer == VK_NULL_HANDLE );
		CHECK_ERR( not _memoryId );

		auto&	dev = resMngr.GetDevice();

		_desc		= desc;
		_memoryId	= UniqueID<MemoryID>{ memId };

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
		if ( info.queueFamilyIndexCount < 2 )
		{
			info.sharingMode			= VK_SHARING_MODE_EXCLUSIVE;
			info.pQueueFamilyIndices	= null;
			info.queueFamilyIndexCount	= 0;
		}

		VK_CHECK( dev.vkCreateBuffer( dev.GetVkDevice(), &info, null, OUT &_buffer ));

		CHECK_ERR( memObj.AllocateForBuffer( resMngr.GetMemoryManager(), _buffer ));

		if ( not dbgName.empty() )
		{
			dev.SetObjectName( BitCast<uint64_t>(_buffer), dbgName, VK_OBJECT_TYPE_BUFFER );
		}

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

		if ( _buffer ) {
			dev.vkDestroyBuffer( dev.GetVkDevice(), _buffer, null );
		}

		if ( _memoryId ) {
			resMngr.ReleaseResource( _memoryId );
		}

		_buffer		= VK_NULL_HANDLE;
		_memoryId	= Default;
		_desc		= Default;

		_debugName.clear();
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
	GetApiSpecificDescription
=================================================
*
	VulkanBufferDesc  VBuffer::GetApiSpecificDescription () const
	{
		VulkanBufferDesc	desc;
		desc.buffer			= BitCast<BufferVk_t>( _buffer );
		desc.usage			= BitCast<BufferUsageFlagsVk_t>( VEnumCast( _desc.usage ));
		desc.size			= _desc.size;
		desc.queueFamily	= VK_QUEUE_FAMILY_IGNORED;
		//desc.queueFamilyIndices	// TODO
		return desc;
	}
	*/

}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
