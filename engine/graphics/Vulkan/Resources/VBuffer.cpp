// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

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
	static VkAccessFlags  GetAllBufferReadAccessMasks (VkBufferUsageFlagBits usage)
	{
		VkAccessFlags	result = Zero;

		while ( usage != Zero )
		{
			VkBufferUsageFlagBits	t = ExtractBit( INOUT usage );

			BEGIN_ENUM_CHECKS();
			switch ( t )
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
	bool  VBuffer::Create (VResourceManagerImpl &resMngr, const BufferDesc &desc, VMemAllocatorPtr allocator, StringView dbgName)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( _buffer == VK_NULL_HANDLE );
		CHECK_ERR( allocator );
		CHECK_ERR( desc.size > 0 );
		CHECK_ERR( desc.usage != Default );

		auto&	dev = resMngr.GetDevice();
		ASSERT( IsSupported( dev, desc ));

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

		CHECK_ERR( allocator->AllocForBuffer( _buffer, _desc, INOUT _memStorage ));

		if ( not dbgName.empty() )
		{
			dev.SetObjectName( BitCast<uint64_t>(_buffer), dbgName, VK_OBJECT_TYPE_BUFFER );
		}
		
		_memAllocator	= RVRef(allocator);
		_debugName		= dbgName;
		_canBeDestroyed	= true;

		return true;
	}
	
/*
=================================================
	Create
=================================================
*/
	bool  VBuffer::Create (const VDevice &dev, const VulkanBufferDesc &desc, StringView dbgName)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( _buffer == VK_NULL_HANDLE );
		CHECK_ERR( desc.buffer != VK_NULL_HANDLE );

		_buffer			= desc.buffer;
		_desc.size		= desc.size;
		_desc.usage		= AEEnumCast( desc.usage );
		_desc.memType	= Default;
		
		ASSERT( IsSupported( dev, _desc ));

		if ( dbgName.size() )
			dev.SetObjectName( BitCast<uint64_t>(_buffer), dbgName, VK_OBJECT_TYPE_BUFFER );

		_debugName		= dbgName;
		_canBeDestroyed	= desc.canBeDestroyed;

		return true;
	}

/*
=================================================
	Destroy
=================================================
*/
	void  VBuffer::Destroy (VResourceManagerImpl &resMngr)
	{
		EXLOCK( _drCheck );

		auto&	dev = resMngr.GetDevice();

		{
			SHAREDLOCK( _viewMapLock );
			for (auto& view : _viewMap) {
				dev.vkDestroyBufferView( dev.GetVkDevice(), view.second, null );
			}
			_viewMap.clear();
		}

		if ( _canBeDestroyed and _buffer ) {
			dev.vkDestroyBuffer( dev.GetVkDevice(), _buffer, null );
		}

		if ( _memAllocator ) {
			CHECK( _memAllocator->Dealloc( INOUT _memStorage ));
		}

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
	bool  VBuffer::GetMemoryInfo (OUT VulkanMemoryObjInfo &info) const
	{
		SHAREDLOCK( _drCheck );
		CHECK_ERR( _memAllocator );
		return _memAllocator->GetInfo( _memStorage, OUT info );
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
		ASSERT( IsSupported( dev, desc ));

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

		return not AnyBits( _desc.usage, mask );
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
		desc.buffer			= _buffer;
		desc.usage			= VEnumCast( _desc.usage );
		desc.size			= _desc.size;
		desc.canBeDestroyed	= _canBeDestroyed;
		return desc;
	}
	
/*
=================================================
	IsSupported
=================================================
*/
	bool  VBuffer::IsSupported (const VDevice &dev, const BufferDesc &desc)
	{
		for (EBufferUsage usage = desc.usage; usage != Zero;)
		{
			EBufferUsage	t = ExtractBit( INOUT usage );

			BEGIN_ENUM_CHECKS();
			switch ( t )
			{
				case EBufferUsage::UniformTexel :		break;
				case EBufferUsage::StorageTexel :		break;
				case EBufferUsage::StorageTexelAtomic:	break;
				case EBufferUsage::TransferSrc :		break;
				case EBufferUsage::TransferDst :		break;
				case EBufferUsage::Uniform :			break;
				case EBufferUsage::Storage :			break;
				case EBufferUsage::Index :				break;
				case EBufferUsage::Vertex :				break;
				case EBufferUsage::Indirect :			break;
				case EBufferUsage::RayTracing :			if ( not dev.GetFeatures().rayTracingNV ) return false;									break;
				case EBufferUsage::ShaderAddress :		if ( not dev.GetFeatures().bufferAddress ) return false;								break;
				case EBufferUsage::VertexPplnStore :	if ( not dev.GetProperties().features.vertexPipelineStoresAndAtomics ) return false;	break;
				case EBufferUsage::FragmentPplnStore :	if ( not dev.GetProperties().features.fragmentStoresAndAtomics ) return false;			break;
				case EBufferUsage::_Last :
				case EBufferUsage::All :
				case EBufferUsage::Transfer :
				case EBufferUsage::Unknown :
				default :								ASSERT(false);	break;
			}
			END_ENUM_CHECKS();
		}
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
		
		const VkFormatFeatureFlags	available_flags	= props.bufferFeatures;
		VkFormatFeatureFlags		required_flags	= 0;
		
		for (EBufferUsage usage = _desc.usage; usage != Zero;)
		{
			EBufferUsage	t = ExtractBit( INOUT usage );

			BEGIN_ENUM_CHECKS();
			switch ( t )
			{
				case EBufferUsage::UniformTexel :		required_flags |= VK_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER_BIT;			break;
				case EBufferUsage::StorageTexel :		required_flags |= VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_BIT;			break;
				case EBufferUsage::StorageTexelAtomic:	required_flags |= VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_ATOMIC_BIT;	break;
				case EBufferUsage::TransferSrc :		break;
				case EBufferUsage::TransferDst :		break;
				case EBufferUsage::Uniform :			break;
				case EBufferUsage::Storage :			break;
				case EBufferUsage::Index :				break;
				case EBufferUsage::Vertex :				break;
				case EBufferUsage::Indirect :			break;
				case EBufferUsage::RayTracing :			break;
				case EBufferUsage::ShaderAddress :		break;
				case EBufferUsage::VertexPplnStore :	break;
				case EBufferUsage::FragmentPplnStore :	break;
				case EBufferUsage::_Last :
				case EBufferUsage::All :
				case EBufferUsage::Transfer :
				case EBufferUsage::Unknown :
				default :								ASSERT(false);	break;
			}
			END_ENUM_CHECKS();
		}
		
		return AllBits( available_flags, required_flags );
	}


}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
