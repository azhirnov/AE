// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef AE_ENABLE_VULKAN

# include "graphics/Public/ImageDesc.h"
# include "graphics/Public/GfxMemAllocator.h"
# include "graphics/Public/EResourceState.h"
# include "graphics/Public/IDs.h"
# include "graphics/Vulkan/VQueue.h"
# include "graphics/Public/NativeTypes.Vulkan.h"

namespace AE::Graphics
{

	//
	// Vulkan Image immutable data
	//

	class VImage final
	{
	// types
	public:
		using ImageViewMap_t	= HashMap< ImageViewDesc, VkImageView >;
		using MemStorage_t		= IGfxMemAllocator::Storage_t;


	// variables
	private:
		VkImage						_image				= VK_NULL_HANDLE;
		ImageDesc					_desc;

		mutable SharedMutex			_viewMapLock;
		mutable ImageViewMap_t		_viewMap;
		
		VkImageAspectFlagBits		_aspectMask			= Zero;
		VkImageLayout				_defaultLayout		= VK_IMAGE_LAYOUT_MAX_ENUM;
		//VkAccessFlags				_readAccessMask		= 0;
		
		GfxMemAllocatorPtr			_memAllocator;
		MemStorage_t				_memStorage;

		DebugName_t					_debugName;
		bool						_canBeDestroyed		= true;

		RWDataRaceCheck				_drCheck;


	// methods
	public:
		VImage () {}
		~VImage ();

		bool Create (VResourceManager &, const ImageDesc &desc, GfxMemAllocatorPtr allocator, EResourceState defaultState, StringView dbgName);
		bool Create (const VDevice &dev, const VulkanImageDesc &desc, StringView dbgName);

		void Destroy (VResourceManager &);
		
		bool GetMemoryInfo (OUT VResourceMemoryInfo &) const;
		bool GetMemoryInfo (OUT IGfxMemAllocator::NativeMemInfo_t &) const;

		ND_ VkImageView			GetView (const VDevice &, const ImageViewDesc &) const;
		ND_ VkImageView			GetView (const VDevice &, bool isDefault, INOUT ImageViewDesc &) const;

		ND_ VulkanImageDesc		GetNativeDescription () const;
		
		ND_ bool				IsReadOnly ()			const;
		ND_ bool				IsOwnsHandle ()			const;

		ND_ VkImage				Handle ()				const	{ SHAREDLOCK( _drCheck );  return _image; }
		ND_ ImageDesc const&	Description ()			const	{ SHAREDLOCK( _drCheck );  return _desc; }
		ND_ VkImageAspectFlags	AspectMask ()			const	{ SHAREDLOCK( _drCheck );  return _aspectMask; }
		ND_ uint3 const			Dimension ()			const	{ SHAREDLOCK( _drCheck );  return _desc.dimension; }
		ND_ VkImageLayout		DefaultLayout ()		const	{ SHAREDLOCK( _drCheck );  return _defaultLayout; }

		ND_ uint const			Width ()				const	{ SHAREDLOCK( _drCheck );  return _desc.dimension.x; }
		ND_ uint const			Height ()				const	{ SHAREDLOCK( _drCheck );  return _desc.dimension.y; }
		ND_ uint const			Depth ()				const	{ SHAREDLOCK( _drCheck );  return _desc.dimension.z; }
		ND_ uint const			ArrayLayers ()			const	{ SHAREDLOCK( _drCheck );  return _desc.arrayLayers.Get(); }
		ND_ uint const			MipmapLevels ()			const	{ SHAREDLOCK( _drCheck );  return _desc.maxLevel.Get(); }
		ND_ EPixelFormat		PixelFormat ()			const	{ SHAREDLOCK( _drCheck );  return _desc.format; }
		ND_ EImage				ImageType ()			const	{ SHAREDLOCK( _drCheck );  return _desc.imageType; }
		ND_ uint const			Samples ()				const	{ SHAREDLOCK( _drCheck );  return _desc.samples.Get(); }
		
		//ND_ VkAccessFlags		GetAllReadAccessMask ()	const	{ SHAREDLOCK( _drCheck );  return _readAccessMask; }

		ND_ bool				IsExclusiveSharing ()	const	{ SHAREDLOCK( _drCheck );  return _desc.queues == Default; }
		ND_ StringView			GetDebugName ()			const	{ SHAREDLOCK( _drCheck );  return _debugName; }


		ND_ static bool	IsSupported (const VDevice &dev, const ImageDesc &desc);
		ND_ bool		IsSupported (const VDevice &dev, const ImageViewDesc &desc) const;


	private:
		bool _CreateView (const VDevice &, const ImageViewDesc &, OUT VkImageView &) const;
	};
	

}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
