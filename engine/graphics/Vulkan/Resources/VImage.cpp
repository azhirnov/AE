// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#ifdef AE_ENABLE_VULKAN

# include "graphics/Vulkan/Resources/VImage.h"
# include "graphics/Vulkan/VEnumCast.h"
# include "graphics/Private/EnumUtils.h"
# include "graphics/Vulkan/VResourceManager.h"

namespace AE::Graphics
{
namespace {
/*
=================================================
	_ChooseAspect
=================================================
*/
	ND_ static VkImageAspectFlagBits  ChooseAspect (EPixelFormat format)
	{
		VkImageAspectFlagBits	result = VkImageAspectFlagBits(0);

		if ( EPixelFormat_IsColor( format ))
			result |= VK_IMAGE_ASPECT_COLOR_BIT;
		else
		{
			if ( EPixelFormat_HasDepth( format ))
				result |= VK_IMAGE_ASPECT_DEPTH_BIT;

			if ( EPixelFormat_HasStencil( format ))
				result |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
		return result;
	}
	
/*
=================================================
	_ChooseDefaultLayout
=================================================
*/
	ND_ static VkImageLayout  ChooseDefaultLayout (EImageUsage usage, VkImageLayout defaultLayout)
	{
		VkImageLayout	result = VK_IMAGE_LAYOUT_GENERAL;

		if ( defaultLayout != VK_IMAGE_LAYOUT_MAX_ENUM and defaultLayout != VK_IMAGE_LAYOUT_UNDEFINED )
			result = defaultLayout;
		else
		// render target layouts has high priority to avoid unnecessary decompressions
		if ( EnumEq( usage, EImageUsage::ColorAttachment ))
			result = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		else
		if ( EnumEq( usage, EImageUsage::DepthStencilAttachment ))
			result = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		else
		if ( EnumEq( usage, EImageUsage::Sampled ))
			result = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		else
		if ( EnumEq( usage, EImageUsage::Storage ))
			result = VK_IMAGE_LAYOUT_GENERAL;

		return result;
	}
	
/*
=================================================
	GetAllImageAccessMasks
=================================================
*
	ND_ static VkAccessFlags  GetAllImageAccessMasks (VkImageUsageFlags usage)
	{
		VkAccessFlags	result = 0;

		for (VkImageUsageFlags t = 1; t <= usage; t <<= 1)
		{
			if ( not EnumEq( usage, t ) )
				continue;

			BEGIN_ENUM_CHECKS();
			switch ( VkImageUsageFlagBits(t) )
			{
				case VK_IMAGE_USAGE_TRANSFER_SRC_BIT :				result |= VK_ACCESS_TRANSFER_READ_BIT;					break;
				case VK_IMAGE_USAGE_TRANSFER_DST_BIT :				break;
				case VK_IMAGE_USAGE_SAMPLED_BIT :					result |= VK_ACCESS_SHADER_READ_BIT;					break;
				case VK_IMAGE_USAGE_STORAGE_BIT :					result |= VK_ACCESS_SHADER_READ_BIT;					break;
				case VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT :			result |= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;			break;
				case VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT :	result |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;	break;
				case VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT :		break;
				case VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT :			result |= VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;			break;
				case VK_IMAGE_USAGE_SHADING_RATE_IMAGE_BIT_NV :		result |= VK_ACCESS_SHADING_RATE_IMAGE_READ_BIT_NV;		break;
				case VK_IMAGE_USAGE_FRAGMENT_DENSITY_MAP_BIT_EXT :
				case VK_IMAGE_USAGE_FLAG_BITS_MAX_ENUM :			break;	// to shutup compiler warnings
			}
			END_ENUM_CHECKS();
		}
		return result;
	}*/
}
//-----------------------------------------------------------------------------



/*
=================================================
	destructor
=================================================
*/
	VImage::~VImage ()
	{
		ASSERT( _image == VK_NULL_HANDLE );
		ASSERT( _viewMap.empty() );
	}

/*
=================================================
	Create
=================================================
*/
	bool VImage::Create (VResourceManager &resMngr, const ImageDesc &desc, GfxMemAllocatorPtr allocator, EResourceState defaultState, StringView dbgName)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( _image == VK_NULL_HANDLE );
		CHECK_ERR( allocator );
		
		_desc = desc;
		_desc.Validate();

		auto&		dev			= resMngr.GetDevice();
		const bool	opt_tiling	= not EnumAny( _desc.memType, EMemoryType::_HostVisible );
		
		// create image
		VkImageCreateInfo	info = {};
		info.sType			= VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		info.pNext			= null;
		info.flags			= VEnumCast( _desc.flags );
		info.imageType		= VEnumCast( _desc.imageType );
		info.format			= VEnumCast( _desc.format );
		info.extent.width	= _desc.dimension.x;
		info.extent.height	= _desc.dimension.y;
		info.extent.depth	= _desc.dimension.z;
		info.mipLevels		= _desc.maxLevel.Get();
		info.arrayLayers	= _desc.arrayLayers.Get();
		info.samples		= VEnumCast( _desc.samples );
		info.tiling			= (opt_tiling ? VK_IMAGE_TILING_OPTIMAL : VK_IMAGE_TILING_LINEAR);
		info.usage			= VEnumCast( _desc.usage );
		info.initialLayout	= (opt_tiling ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_PREINITIALIZED);
		
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

		// TODO:
		// test usage with supported features (vkGetPhysicalDeviceFormatProperties2)

		VK_CHECK( dev.vkCreateImage( dev.GetVkDevice(), &info, null, OUT &_image ));

		CHECK_ERR( allocator->AllocForImage( BitCast<ImageVk_t>(_image), _desc, INOUT _memStorage ));
		
		if ( not dbgName.empty() )
		{
			dev.SetObjectName( BitCast<uint64_t>(_image), dbgName, VK_OBJECT_TYPE_IMAGE );
		}

		_memAllocator		= std::move(allocator);
		//_readAccessMask	= GetAllImageAccessMasks( info.usage );
		_aspectMask			= ChooseAspect( _desc.format );
		_defaultLayout		= ChooseDefaultLayout( _desc.usage, EResourceState_ToImageLayout( defaultState, _aspectMask ));
		_debugName			= dbgName;

		return true;
	}

/*
=================================================
	Create
=================================================
*
	bool VImage::Create (const VDevice &dev, const VulkanImageDesc &desc, StringView dbgName, OnRelease_t &&onRelease)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( _image == VK_NULL_HANDLE );
		
		_image				= BitCast<VkImage>( desc.image );
		_desc.imageType		= FGEnumCast( BitCast<VkImageType>( desc.imageType ), BitCast<ImageFlagsVk_t>( desc.flags ),
										  desc.arrayLayers, BitCast<VkSampleCountFlagBits>( desc.samples ) );
		_desc.dimension		= desc.dimension;
		_desc.format		= FGEnumCast( BitCast<VkFormat>( desc.format ));
		_desc.usage			= FGEnumCast( BitCast<VkImageUsageFlagBits>( desc.usage ));
		_desc.arrayLayers	= ImageLayer{ desc.arrayLayers };
		_desc.maxLevel		= MipmapLevel{ desc.maxLevels };
		_desc.samples		= FGEnumCast( BitCast<VkSampleCountFlagBits>( desc.samples ));
		_desc.isExternal	= true;

		if ( not dbgName.empty() )
		{
			dev.SetObjectName( BitCast<uint64_t>(_image), dbgName, VK_OBJECT_TYPE_IMAGE );
		}
		
		CHECK( desc.queueFamily == VK_QUEUE_FAMILY_IGNORED );	// not supported yet
		CHECK( desc.queueFamilyIndices.empty() or desc.queueFamilyIndices.size() >= 2 );

		_queueFamilyMask = Default;

		for (auto idx : desc.queueFamilyIndices) {
			_queueFamilyMask |= BitCast<EQueueFamily>(idx);
		}

		_aspectMask		= ChooseAspect( _desc.format );
		_defaultLayout	= ChooseDefaultLayout( _desc.usage, BitCast<VkImageLayout>(desc.defaultLayout) );
		_debugName		= dbgName;
		_onRelease		= std::move(onRelease);

		return true;
	}

/*
=================================================
	Destroy
=================================================
*/
	void VImage::Destroy (VResourceManager &resMngr)
	{
		EXLOCK( _drCheck );
		
		auto&	dev = resMngr.GetDevice();

		for (auto& view : _viewMap) {
			dev.vkDestroyImageView( dev.GetVkDevice(), view.second, null );
		}
		
		if ( _image ) {
			dev.vkDestroyImage( dev.GetVkDevice(), _image, null );
		}

		if ( _memAllocator ) {
			CHECK( _memAllocator->Dealloc( INOUT _memStorage ));
		}
		
		_viewMap.clear();
		_debugName.clear();

		_memAllocator	= null;
		_image			= VK_NULL_HANDLE;
		_desc			= Default;
		_aspectMask		= VkImageAspectFlagBits(0);
		_defaultLayout	= VK_IMAGE_LAYOUT_MAX_ENUM;
	}
	
/*
=================================================
	GetMemoryInfo
=================================================
*/
	bool VImage::GetMemoryInfo (OUT VResourceMemoryInfo &outInfo) const
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
	VkImageView  VImage::GetView (const VDevice &dev, const ImageViewDesc &desc) const
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
	GetView
=================================================
*/
	VkImageView  VImage::GetView (const VDevice &dev, bool isDefault, INOUT ImageViewDesc &viewDesc) const
	{
		SHAREDLOCK( _drCheck );

		if ( isDefault )
			viewDesc = ImageViewDesc{ _desc };
		else
			viewDesc.Validate( _desc );

		return GetView( dev, viewDesc );
	}

/*
=================================================
	_CreateView
=================================================
*/
	bool VImage::_CreateView (const VDevice &dev, const ImageViewDesc &viewDesc, OUT VkImageView &outView) const
	{
		const VkComponentSwizzle	components[] = {
			VK_COMPONENT_SWIZZLE_IDENTITY,	// unknown
			VK_COMPONENT_SWIZZLE_R,
			VK_COMPONENT_SWIZZLE_G,
			VK_COMPONENT_SWIZZLE_B,
			VK_COMPONENT_SWIZZLE_A,
			VK_COMPONENT_SWIZZLE_ZERO,
			VK_COMPONENT_SWIZZLE_ONE
		};

		const auto&				desc		= viewDesc;
		const uint4				swizzle		= Min( uint4(uint(CountOf(components)-1)), desc.swizzle.ToVec() );
		VkImageViewCreateInfo	view_info	= {};

		view_info.sType			= VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		view_info.pNext			= null;
		view_info.viewType		= VEnumCast( desc.viewType );
		view_info.flags			= 0;
		view_info.image			= Handle();
		view_info.format		= VEnumCast( desc.format );
		view_info.components	= { components[swizzle.x], components[swizzle.y], components[swizzle.z], components[swizzle.w] };

		view_info.subresourceRange.aspectMask		= VEnumCast( desc.aspectMask );
		view_info.subresourceRange.baseMipLevel		= desc.baseLevel.Get();
		view_info.subresourceRange.levelCount		= desc.levelCount;
		view_info.subresourceRange.baseArrayLayer	= desc.baseLayer.Get();
		view_info.subresourceRange.layerCount		= desc.layerCount;
		
		VK_CHECK( dev.vkCreateImageView( dev.GetVkDevice(), &view_info, null, OUT &outView ));
		return true;
	}
	
/*
=================================================
	IsReadOnly
=================================================
*/
	bool  VImage::IsReadOnly () const
	{
		SHAREDLOCK( _drCheck );
		return not EnumEq( _desc.usage, EImageUsage::TransferDst | EImageUsage::ColorAttachment | EImageUsage::Storage |
										EImageUsage::DepthStencilAttachment | EImageUsage::TransientAttachment );
	}
	
/*
=================================================
	GetNativeDescription
=================================================
*/
	VulkanImageDesc  VImage::GetNativeDescription () const
	{
		VulkanImageDesc		desc;
		desc.image			= BitCast<ImageVk_t>( _image );
		//desc.imageType		= BitCast<ImageTypeVk_t>( GetImageType( _desc.imageType ));	// TODO
		//desc.flags			= BitCast<ImageFlagsVk_t>( GetImageFlags( _desc.flags ));
		desc.usage			= BitCast<ImageUsageVk_t>( VEnumCast( _desc.usage ));
		desc.format			= BitCast<FormatVk_t>( VEnumCast( _desc.format ));
		desc.currentLayout	= BitCast<ImageLayoutVk_t>( _defaultLayout );	// TODO
		desc.defaultLayout	= desc.currentLayout;
		desc.samples		= BitCast<SampleCountFlagBitsVk_t>( VEnumCast( _desc.samples ));
		desc.dimension		= _desc.dimension;
		desc.arrayLayers	= _desc.arrayLayers.Get();
		desc.maxLevels		= _desc.maxLevel.Get();
		//desc.queueFamily	= VK_QUEUE_FAMILY_IGNORED;
		//desc.queueFamilyIndices	// TODO
		return desc;
	}
	
/*
=================================================
	IsSupported
=================================================
*/
	bool  VImage::IsSupported (const VDevice &dev, const ImageDesc &desc)
	{
		VkFormatProperties	props = {};
		vkGetPhysicalDeviceFormatProperties( dev.GetVkPhysicalDevice(), VEnumCast( desc.format ), OUT &props );
		
		const bool					opt_tiling	= not EnumAny( desc.memType, EMemoryType::_HostVisible );
		const VkFormatFeatureFlags	dev_flags	= opt_tiling ? props.optimalTilingFeatures : props.linearTilingFeatures;
		VkFormatFeatureFlags		img_flags	= 0;

		for (EImageUsage t = EImageUsage(1); t <= desc.usage; t = EImageUsage(uint(t) << 1))
		{
			if ( not EnumEq( desc.usage, t ))
				continue;

			BEGIN_ENUM_CHECKS();
			switch ( t )
			{
				case EImageUsage::TransferSrc :				img_flags |= VK_FORMAT_FEATURE_TRANSFER_SRC_BIT | VK_FORMAT_FEATURE_BLIT_SRC_BIT;	break;
				case EImageUsage::TransferDst :				img_flags |= VK_FORMAT_FEATURE_TRANSFER_DST_BIT | VK_FORMAT_FEATURE_BLIT_DST_BIT;	break;
				case EImageUsage::Sampled :					img_flags |= VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;	break;
				case EImageUsage::Storage :					img_flags |= VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT | VK_FORMAT_FEATURE_STORAGE_IMAGE_ATOMIC_BIT;			break;
				case EImageUsage::ColorAttachment :			img_flags |= VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT | VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT;		break;
				case EImageUsage::DepthStencilAttachment :	img_flags |= VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;	break;
				case EImageUsage::TransientAttachment :		break;
				case EImageUsage::InputAttachment :			break;
				case EImageUsage::ShadingRate :				break;
				
				#ifdef VK_EXT_fragment_density_map
				case EImageUsage::FragmentDensityMap :		img_flags |= VK_FORMAT_FEATURE_FRAGMENT_DENSITY_MAP_BIT_EXT;	break;
				#else
				case EImageUsage::FragmentDensityMap :		ASSERT(false);	break;
				#endif

				case EImageUsage::_Last :
				case EImageUsage::All :
				case EImageUsage::Transfer :
				case EImageUsage::Unknown :
				default :									ASSERT(false);	break;
			}
			END_ENUM_CHECKS();
		}

		return (dev_flags & img_flags);
	}
	
/*
=================================================
	IsSupported
=================================================
*/
	bool  VImage::IsSupported (const VDevice &, const ImageViewDesc &desc) const
	{
		SHAREDLOCK( _drCheck );

		// TODO: mutable format

		if ( desc.format != Default and desc.format != _desc.format )
			return false;

		return true;
	}


}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
