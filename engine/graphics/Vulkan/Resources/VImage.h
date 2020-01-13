// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef AE_ENABLE_VULKAN

# include "graphics/Public/ImageDesc.h"
# include "graphics/Public/EResourceState.h"
# include "graphics/Public/IDs.h"
# include "graphics/Vulkan/VQueue.h"

namespace AE::Graphics
{

	//
	// Vulkan Image immutable data
	//

	class VImage final
	{
		friend class VImageUnitTest;

	// types
	public:
		using ImageViewMap_t	= HashMap< ImageViewDesc, VkImageView >;
		//using OnRelease_t		= IFrameGraph::OnExternalImageReleased_t;


	// variables
	private:
		VkImage						_image				= VK_NULL_HANDLE;
		ImageDesc					_desc;

		mutable SharedMutex			_viewMapLock;
		mutable ImageViewMap_t		_viewMap;

		UniqueID<MemoryID>			_memoryId;
		VkImageAspectFlags			_aspectMask			= 0;
		VkImageLayout				_defaultLayout		= VK_IMAGE_LAYOUT_MAX_ENUM;
		VkAccessFlags				_readAccessMask		= 0;

		DebugName_t					_debugName;

		RWDataRaceCheck				_drCheck;


	// methods
	public:
		VImage () {}
		~VImage ();

		bool Create (VResourceManager &, const ImageDesc &desc, MemoryID memId, VMemoryObj &memObj,
					 EResourceState defaultState, StringView dbgName);

		//bool Create (const VDevice &dev, const VulkanImageDesc &desc, StringView dbgName, OnRelease_t &&onRelease);

		void Destroy (VResourceManager &);

		//ND_ VulkanImageDesc		GetApiSpecificDescription () const;

		ND_ VkImageView			GetView (const VDevice &, const ImageViewDesc &) const;
		ND_ VkImageView			GetView (const VDevice &, bool isDefault, INOUT ImageViewDesc &) const;
		
		ND_ bool				IsReadOnly ()			const;
		ND_ bool				IsOwnsHandle ()			const;

		ND_ VkImage				Handle ()				const	{ SHAREDLOCK( _drCheck );  return _image; }
		ND_ MemoryID			GetMemoryID ()			const	{ SHAREDLOCK( _drCheck );  return _memoryId; }

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
		
		ND_ VkAccessFlags		GetAllReadAccessMask ()	const	{ SHAREDLOCK( _drCheck );  return _readAccessMask; }

		ND_ bool				IsExclusiveSharing ()	const	{ SHAREDLOCK( _drCheck );  return _desc.queues == Default; }
		ND_ StringView			GetDebugName ()			const	{ SHAREDLOCK( _drCheck );  return _debugName; }


	private:
		bool _CreateView (const VDevice &, const ImageViewDesc &, OUT VkImageView &) const;
	};
	

}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
