// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef AE_ENABLE_VULKAN

# include "graphics/Vulkan/VDevice.h"
# include "graphics/Public/ResourceManager.h"

namespace AE::App
{
	struct NativeWindow;
}

namespace AE::Graphics
{

	//
	// Vulkan Swapchain
	//

	class VSwapchain : public VulkanDeviceFn, public Noncopyable
	{
	// types
	public:
		static constexpr VkImageUsageFlagBits	DefaultImageUsage	=
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
			VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;

	private:
		static constexpr uint	MaxSwapchainLength = 8;

		using Images_t		= FixedArray< VkImage, MaxSwapchainLength >;
		using ImageIDs_t	= FixedArray< UniqueID<GfxResourceID>, MaxSwapchainLength >;


	// variables
	protected:
		VDevice const&					_device;

		uint2							_surfaceSize;
		VkSwapchainKHR					_vkSwapchain		= VK_NULL_HANDLE;
		VkSurfaceKHR					_vkSurface			= VK_NULL_HANDLE;

		uint							_currImageIndex		= UMax;
		Images_t						_images;
		ImageIDs_t						_imageIDs;

		VkFormat						_colorFormat		= VK_FORMAT_UNDEFINED;
		VkColorSpaceKHR					_colorSpace			= VK_COLOR_SPACE_MAX_ENUM_KHR;
		uint							_minImageCount		= 2;
		VkSurfaceTransformFlagBitsKHR	_preTransform		= VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
		VkPresentModeKHR				_presentMode		= VK_PRESENT_MODE_FIFO_KHR;
		VkCompositeAlphaFlagBitsKHR		_compositeAlpha		= VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		VkImageUsageFlagBits			_colorImageUsage	= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		

	// methods
	protected:
		explicit VSwapchain (const VDevice &dev);

	public:
		~VSwapchain ();

		ND_ bool IsSupported (VQueuePtr queue) const;

		ND_ VkResult  AcquireNextImage (VkSemaphore imageAvailable);
		ND_ VkResult  Present (VQueuePtr queue, ArrayView<VkSemaphore> renderFinished);
		
		ND_ VkSurfaceKHR				GetVkSurface ()					const	{ return _vkSurface; }
		ND_ VkSwapchainKHR				GetVkSwapchain ()				const	{ return _vkSwapchain; }

		ND_ uint2 const&				GetSurfaceSize ()				const	{ return _surfaceSize; }
		ND_ VkFormat					GetColorFormat ()				const	{ return _colorFormat; }
		ND_ VkColorSpaceKHR				GetColorSpace ()				const	{ return _colorSpace; }
		ND_ VkSurfaceTransformFlagBitsKHR	GetPreTransformFlags ()		const	{ return _preTransform; }
		ND_ VkPresentModeKHR			GetPresentMode ()				const	{ return _presentMode; }
		ND_ VkCompositeAlphaFlagBitsKHR	GetCompositeAlphaMode ()		const	{ return _compositeAlpha; }

		ND_ uint						GetSwapchainLength ()			const	{ return uint(_images.size()); }
		ND_ uint						GetCurretImageIndex ()			const	{ return _currImageIndex; }
		ND_ bool						IsImageAcquired ()				const	{ return GetCurretImageIndex() < GetSwapchainLength(); }

		ND_ VkImageUsageFlagBits		GetImageUsage ()				const	{ return _colorImageUsage; }
		ND_ VkImage						GetCurrentImage ()				const;
		ND_ GfxResourceID				GetCurrentImageID ()			const;
	};

	

	//
	// Vulkan Swapchain Initializer
	//

	class VSwapchainInitializer final : public VSwapchain
	{
	// methods
	public:
		explicit VSwapchainInitializer (const VDevice &dev);

		bool CreateSurface (const App::NativeWindow &, StringView dbgName = {});
		void DestroySurface ();

		ND_ bool IsSupported (VkSampleCountFlagBits samples, VkPresentModeKHR presentMode,
							  VkFormat colorFormat, VkImageUsageFlagBits colorImageUsage) const;
		
		bool ChooseColorFormat (INOUT VkFormat &colorFormat, INOUT VkColorSpaceKHR &colorSpace) const;

		bool Create (IResourceManager *						resMngr,
					 const uint2							&viewSize,
					 const VkFormat							colorFormat			= VK_FORMAT_B8G8R8A8_UNORM,
					 const VkColorSpaceKHR					colorSpace			= VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
					 const uint								minImageCount		= 2,
					 const VkPresentModeKHR					presentMode			= VK_PRESENT_MODE_FIFO_KHR,
					 const VkSurfaceTransformFlagBitsKHR	transform			= VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
					 const VkCompositeAlphaFlagBitsKHR		compositeAlpha		= VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
					 const VkImageUsageFlagBits				colorImageUsage		= DefaultImageUsage,
					 StringView								dbgName				= {});

		void Destroy (IResourceManager* resMngr);

		bool Recreate (IResourceManager* resMngr, const uint2 &size);
		
		ND_ static ArrayView<const char*>  GetInstanceExtensions ();

	private:
		bool _CreateColorAttachment (IResourceManager* resMngr);
		void _PrintInfo () const;

		bool _GetImageUsage (OUT VkImageUsageFlags &imageUsage,	VkPresentModeKHR presentMode, VkFormat colorFormat, const VkSurfaceCapabilitiesKHR &surfaceCaps) const;
		bool _GetCompositeAlpha (INOUT VkCompositeAlphaFlagBitsKHR &compositeAlpha, const VkSurfaceCapabilitiesKHR &surfaceCaps) const;
		void _GetPresentMode (INOUT VkPresentModeKHR &presentMode) const;
		void _GetSwapChainExtent (INOUT VkExtent2D &extent, const VkSurfaceCapabilitiesKHR &surfaceCaps) const;
		void _GetSurfaceTransform (INOUT VkSurfaceTransformFlagBitsKHR &transform, const VkSurfaceCapabilitiesKHR &surfaceCaps) const;
		void _GetSurfaceImageCount (INOUT uint &minImageCount, const VkSurfaceCapabilitiesKHR &surfaceCaps) const;
	};

	
/*
=================================================
	GetCurrentImage
=================================================
*/
	inline VkImage  VSwapchain::GetCurrentImage () const
	{
		return _currImageIndex < _images.size() ? _images[_currImageIndex] : VK_NULL_HANDLE;
	}
	
/*
=================================================
	GetCurrentImageID
=================================================
*/
	inline GfxResourceID  VSwapchain::GetCurrentImageID () const
	{
		return _currImageIndex < _imageIDs.size() ? _imageIDs[_currImageIndex] : Default;
	}

}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
