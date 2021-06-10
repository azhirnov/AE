// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

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
	ChooseAspect
=================================================
*/
	ND_ VkImageAspectFlagBits  ChooseAspect (EPixelFormat format)
	{
		VkImageAspectFlagBits	result = Zero;

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
	ChooseDefaultLayout
=================================================
*/
	ND_ VkImageLayout  ChooseDefaultLayout (EImageUsage usage, VkImageLayout defaultLayout)
	{
		VkImageLayout	result = VK_IMAGE_LAYOUT_GENERAL;

		if ( defaultLayout != VK_IMAGE_LAYOUT_MAX_ENUM and defaultLayout != VK_IMAGE_LAYOUT_UNDEFINED )
			result = defaultLayout;
		else
		// render target layouts has high priority to avoid unnecessary decompressions
		if ( AllBits( usage, EImageUsage::ColorAttachment ))
			result = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		else
		if ( AllBits( usage, EImageUsage::DepthStencilAttachment ))
			result = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		else
		if ( AllBits( usage, EImageUsage::Sampled ))
			result = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		else
		if ( AllBits( usage, EImageUsage::Storage ))
			result = VK_IMAGE_LAYOUT_GENERAL;

		return result;
	}

/*
=================================================
	GetAllImageAccessMasks
=================================================
*
	ND_ static VkAccessFlags  GetAllImageAccessMasks (VkImageUsageFlagBits usage)
	{
		VkAccessFlags	result = Zero;
		
		while ( usage != Zero )
		{
			VkImageUsageFlagBits	t = ExtractBit( INOUT usage );

			BEGIN_ENUM_CHECKS();
			switch ( t )
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
	bool  VImage::Create (VResourceManagerImpl &resMngr, const ImageDesc &desc, VMemAllocatorPtr allocator, EResourceState defaultState, StringView dbgName)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( _image == VK_NULL_HANDLE );
		CHECK_ERR( allocator );
		CHECK_ERR( desc.format != Default );
		CHECK_ERR( desc.usage != Default );
		
		_desc = desc;
		_desc.Validate();

		auto&		dev			= resMngr.GetDevice();
		const bool	opt_tiling	= not AnyBits( _desc.memType, EMemoryType::_HostVisible );
		
		ASSERT( IsSupported( dev, _desc ));

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

		VK_CHECK( dev.vkCreateImage( dev.GetVkDevice(), &info, null, OUT &_image ));

		CHECK_ERR( allocator->AllocForImage( _image, _desc, INOUT _memStorage ));
		
		if ( dbgName.size() )
			dev.SetObjectName( BitCast<uint64_t>(_image), dbgName, VK_OBJECT_TYPE_IMAGE );

		_memAllocator	= RVRef(allocator);
		_aspectMask		= ChooseAspect( _desc.format );
		_defaultLayout	= ChooseDefaultLayout( _desc.usage, EResourceState_ToImageLayout( defaultState, _aspectMask ));
		_debugName		= dbgName;
		_canBeDestroyed	= true;

		return true;
	}

/*
=================================================
	Create
=================================================
*/
	bool  VImage::Create (const VDevice &dev, const VulkanImageDesc &desc, StringView dbgName)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( _image == VK_NULL_HANDLE );
		
		_image				= desc.image;
		_desc.imageType		= AEEnumCast( desc.imageType );
		_desc.flags			= AEEnumCast( desc.flags );
		_desc.dimension		= desc.dimension;
		_desc.format		= AEEnumCast( desc.format );
		_desc.usage			= AEEnumCast( desc.usage );
		_desc.arrayLayers	= ImageLayer{ desc.arrayLayers };
		_desc.maxLevel		= MipmapLevel{ desc.maxLevels };
		_desc.samples		= AEEnumCast( desc.samples );
		_desc.memType		= Default; // TODO

		ASSERT( IsSupported( dev, _desc ));

		if ( dbgName.size() )
			dev.SetObjectName( BitCast<uint64_t>(_image), dbgName, VK_OBJECT_TYPE_IMAGE );

		_aspectMask		= ChooseAspect( _desc.format );
		_defaultLayout	= ChooseDefaultLayout( _desc.usage, desc.defaultLayout );
		_debugName		= dbgName;
		_canBeDestroyed	= desc.canBeDestroyed;

		return true;
	}

/*
=================================================
	Destroy
=================================================
*/
	void  VImage::Destroy (VResourceManagerImpl &resMngr)
	{
		EXLOCK( _drCheck );
		
		auto&	dev = resMngr.GetDevice();
		
		{
			SHAREDLOCK( _viewMapLock );
			for (auto& view : _viewMap) {
				dev.vkDestroyImageView( dev.GetVkDevice(), view.second, null );
			}
			_viewMap.clear();
		}

		if ( _canBeDestroyed and _image ) {
			dev.vkDestroyImage( dev.GetVkDevice(), _image, null );
		}

		if ( _memAllocator ) {
			CHECK( _memAllocator->Dealloc( INOUT _memStorage ));
		}
		
		_debugName.clear();

		_memAllocator	= null;
		_image			= VK_NULL_HANDLE;
		_desc			= Default;
		_aspectMask		= Zero;
		_defaultLayout	= VK_IMAGE_LAYOUT_MAX_ENUM;
	}
	
/*
=================================================
	GetMemoryInfo
=================================================
*/
	bool  VImage::GetMemoryInfo (OUT VulkanMemoryObjInfo &info) const
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
	VkImageView  VImage::GetView (const VDevice &dev, Bool isDefault, INOUT ImageViewDesc &viewDesc) const
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
		return not AllBits( _desc.usage, EImageUsage::TransferDst | EImageUsage::ColorAttachment | EImageUsage::Storage |
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
		desc.image			= _image;
		desc.imageType		= VEnumCast( _desc.imageType );
		desc.flags			= VEnumCast( _desc.flags );
		desc.usage			= VEnumCast( _desc.usage );
		desc.format			= VEnumCast( _desc.format );
		desc.currentLayout	= _defaultLayout;
		desc.defaultLayout	= desc.currentLayout;
		desc.samples		= VEnumCast( _desc.samples );
		desc.dimension		= _desc.dimension;
		desc.arrayLayers	= _desc.arrayLayers.Get();
		desc.maxLevels		= _desc.maxLevel.Get();
		desc.canBeDestroyed	= _canBeDestroyed;
		return desc;
	}
	
/*
=================================================
	IsSupported
=================================================
*/
	bool  VImage::IsSupported (const VDevice &dev, const ImageDesc &desc)
	{
		const bool		opt_tiling	= not AnyBits( desc.memType, EMemoryType::_HostVisible );
		const VkFormat	format		= VEnumCast( desc.format );

		// check available creation flags
		{
			VkImageCreateFlagBits	required	= VEnumCast( desc.flags );
			VkImageCreateFlagBits	available	= dev.GetFlags().imageCreateFlags;

			if ( not AllBits( available, required ))
				return false;
		}

		// check format features
		{
			VkFormatProperties	props = {};
			vkGetPhysicalDeviceFormatProperties( dev.GetVkPhysicalDevice(), format, OUT &props );
		
			const VkFormatFeatureFlags	available	= opt_tiling ? props.optimalTilingFeatures : props.linearTilingFeatures;
			VkFormatFeatureFlags		required	= 0;
			
			for (EImageUsage usage = desc.usage; usage != Zero;)
			{
				EImageUsage	t = ExtractBit( INOUT usage );

				BEGIN_ENUM_CHECKS();
				switch ( t )
				{
					case EImageUsage::TransferSrc :				required |= VK_FORMAT_FEATURE_TRANSFER_SRC_BIT | VK_FORMAT_FEATURE_BLIT_SRC_BIT;	break;
					case EImageUsage::TransferDst :				required |= VK_FORMAT_FEATURE_TRANSFER_DST_BIT | VK_FORMAT_FEATURE_BLIT_DST_BIT;	break;
					case EImageUsage::Sampled :					required |= VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;break;
					case EImageUsage::Storage :					required |= VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT;					break;
					case EImageUsage::SampledMinMax :			required |= VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_MINMAX_BIT_EXT;	break;
					case EImageUsage::StorageAtomic :			required |= VK_FORMAT_FEATURE_STORAGE_IMAGE_ATOMIC_BIT;				break;
					case EImageUsage::ColorAttachment :			required |= VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT;					break;
					case EImageUsage::ColorAttachmentBlend :	required |= VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT;			break;
					case EImageUsage::DepthStencilAttachment :	required |= VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;			break;
					case EImageUsage::TransientAttachment :		break;	// TODO
					case EImageUsage::InputAttachment :			break;
					case EImageUsage::ShadingRate :				if ( not dev.GetFeatures().shadingRateImageNV ) return false;		break;
					//case EImageUsage::FragmentDensityMap :	return false;	// not supported yet
					case EImageUsage::_Last :
					case EImageUsage::All :
					case EImageUsage::Transfer :
					case EImageUsage::Unknown :
					default :									ASSERT(false);	break;
				}
				END_ENUM_CHECKS();
			}

			if ( not AllBits( available, required ))
				return false;
		}

		// check image properties
		{
			VkImageFormatProperties	props = {};
			VK_CHECK( vkGetPhysicalDeviceImageFormatProperties( dev.GetVkPhysicalDevice(), format, VEnumCast( desc.imageType ),
																opt_tiling ? VK_IMAGE_TILING_OPTIMAL : VK_IMAGE_TILING_LINEAR,
																VEnumCast( desc.usage ), VEnumCast( desc.flags ), OUT &props ));

			if ( desc.dimension.x > props.maxExtent.width  or
			 	 desc.dimension.y > props.maxExtent.height or
				 desc.dimension.z > props.maxExtent.depth )
				return false;

			if ( desc.maxLevel.Get() > props.maxMipLevels )
				return false;

			if ( desc.arrayLayers.Get() > props.maxArrayLayers )
				return false;

			if ( not AllBits( props.sampleCounts, desc.samples.Get() ))
				return false;
		}

		return true;
	}
	
/*
=================================================
	IsSupported
=================================================
*/
	bool  VImage::IsSupported (const VDevice &dev, const ImageViewDesc &desc) const
	{
		SHAREDLOCK( _drCheck );
		
		if ( desc.viewType == EImage_CubeArray )
		{
			if ( not dev.GetProperties().features.imageCubeArray )
				return false;

			if ( _desc.imageType != EImageDim_2D or (_desc.imageType == EImageDim_3D and AllBits( _desc.flags, EImageFlags::Array2DCompatible)) )
				return false;

			if ( not AllBits( _desc.flags, EImageFlags::CubeCompatible ))
				return false;

			if ( desc.layerCount % 6 != 0 )
				return false;
		}

		if ( desc.viewType == EImage_Cube )
		{
			if ( not AllBits( _desc.flags, EImageFlags::CubeCompatible ))
				return false;

			if ( desc.layerCount != 6 )
				return false;
		}

		if ( _desc.imageType == EImageDim_3D and desc.viewType != EImage_3D )
		{
			if ( not AllBits( _desc.flags, EImageFlags::Array2DCompatible ))
				return false;
		}

		if ( desc.format != Default and desc.format != _desc.format )
		{
			auto&	required	= EPixelFormat_GetInfo( _desc.format );
			auto&	origin		= EPixelFormat_GetInfo( desc.format );
			bool	req_comp	= Any( required.blockSize > 1u );
			bool	orig_comp	= Any( origin.blockSize > 1u );
			
			if ( not AllBits( _desc.flags, EImageFlags::MutableFormat ))
				return false;

			// compressed to uncompressed
			if ( AllBits( _desc.flags, EImageFlags::BlockTexelViewCompatible ) and orig_comp and not req_comp )
			{
				if ( required.bitsPerBlock != origin.bitsPerBlock )
					return false;
			}
			else
			{
				if ( req_comp != orig_comp )
					return false;

				if ( Any( required.blockSize != origin.blockSize ))
					return false;

				if ( desc.aspectMask == EImageAspect::Stencil )
				{
					if ( required.bitsPerBlock2 != origin.bitsPerBlock2 )
						return false;
				}
				else
				{
					if ( required.bitsPerBlock != origin.bitsPerBlock )
						return false;
				}
			}
		}

		return true;
	}


}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
