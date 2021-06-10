// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#ifdef AE_ENABLE_VULKAN

# include "stl/Algorithms/StringUtils.h"

# include "graphics/Vulkan/VSwapchain.h"
# include "graphics/Vulkan/VResourceManager.h"
# include "graphics/Vulkan/VEnumCast.h"
# include "graphics/Vulkan/VEnumToString.h"
# include "platform/Public/IWindow.h"

# ifdef PLATFORM_WINDOWS
#	include "stl/Platforms/WindowsHeader.h"
#	include <vulkan/vulkan_win32.h>
# endif

# ifdef PLATFORM_ANDROID
#	include <vulkan/vulkan_android.h>
# endif

namespace AE::Graphics
{
	
/*
=================================================
	constructor
=================================================
*/
	VSwapchain::VSwapchain (const VDevice &dev) :
		_device{ dev },
		_semaphoreId{ 0 },
		_currImageIndex{ UMax }
	{
		VulkanDeviceFn_Init( _device );
	}
	
/*
=================================================
	destructor
=================================================
*/
	VSwapchain::~VSwapchain ()
	{
		CHECK( not _vkSwapchain );
		CHECK( not _vkSurface );
	}
	
/*
=================================================
	IsSupported
=================================================
*/
	bool  VSwapchain::IsSupported (VQueuePtr queue) const
	{
		CHECK_ERR( queue );
		CHECK_ERR( _device.GetVkPhysicalDevice() and _vkSurface );

		VkBool32	supported = 0;
		VK_CALL( vkGetPhysicalDeviceSurfaceSupportKHR( _device.GetVkPhysicalDevice(), uint(queue->familyIndex), _vkSurface, OUT &supported ));

		return supported;
	}

/*
=================================================
	AcquireNextImage
=================================================
*/
	VkResult  VSwapchain::AcquireNextImage (OUT VkSemaphore &imageAvailable)
	{
		CHECK_ERR( _vkSwapchain, VK_RESULT_MAX_ENUM );
		CHECK_ERR( not IsImageAcquired(), VK_RESULT_MAX_ENUM );

		_currImageIndex = UMax;
		imageAvailable  = _semaphores[_semaphoreId];

		uint		index = UMax;
		VkResult	res	  = vkAcquireNextImageKHR( _device.GetVkDevice(), _vkSwapchain, UMax, imageAvailable, VK_NULL_HANDLE, OUT &index );

		if ( res == VK_SUCCESS )
		{
			ASSERT( index < _images.size() );
			_currImageIndex = index;
		}

		return res;
	}
	
/*
=================================================
	Present
=================================================
*/
	VkResult  VSwapchain::Present (VQueuePtr queue, ArrayView<VkSemaphore> renderFinished)
	{
		CHECK_ERR( _vkSwapchain and queue, VK_RESULT_MAX_ENUM );
		CHECK_ERR( IsImageAcquired(), VK_RESULT_MAX_ENUM );

		EXLOCK( queue->guard );

		const VkSwapchainKHR	swap_chains[]	= { _vkSwapchain };
		const uint				image_indices[]	= { _currImageIndex };

		STATIC_ASSERT( CountOf(swap_chains) == CountOf(image_indices) );

		VkPresentInfoKHR	present_info = {};
		present_info.sType				= VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		present_info.swapchainCount		= uint(CountOf( swap_chains ));
		present_info.pSwapchains		= swap_chains;
		present_info.pImageIndices		= image_indices;
		present_info.waitSemaphoreCount	= uint(renderFinished.size());
		present_info.pWaitSemaphores	= renderFinished.data();

		_currImageIndex	= UMax;
		_semaphoreId++;

		VkResult	res = vkQueuePresentKHR( queue->handle, &present_info );

		return res;
	}
//-----------------------------------------------------------------------------

	
/*
=================================================
	constructor
=================================================
*/
	VSwapchainInitializer::VSwapchainInitializer (const VDevice &dev) : VSwapchain{ dev }
	{}

/*
=================================================
	CreateSurface
=================================================
*/
	bool  VSwapchainInitializer::CreateSurface (const App::NativeWindow &window, StringView dbgName)
	{
		CHECK_ERR( _device.GetVkInstance() );
		CHECK_ERR( _device.GetFeatures().surface );
		CHECK_ERR( not _vkSurface );

		#if defined(PLATFORM_WINDOWS)
		{
			CHECK_ERR( window.hinstance and window.hwnd );

			VkWin32SurfaceCreateInfoKHR		surface_info = {};

			surface_info.sType		= VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
			surface_info.hinstance	= HINSTANCE(window.hinstance);
			surface_info.hwnd		= HWND(window.hwnd);
		
			auto  fpCreateWin32SurfaceKHR = BitCast<PFN_vkCreateWin32SurfaceKHR>( vkGetInstanceProcAddr( _device.GetVkInstance(), "vkCreateWin32SurfaceKHR" ));
			CHECK_ERR( fpCreateWin32SurfaceKHR );

			VK_CHECK( fpCreateWin32SurfaceKHR( _device.GetVkInstance(), &surface_info, null, OUT &_vkSurface ));
			
			if ( dbgName.size() )
				_device.SetObjectName( BitCast<uint64_t>(_vkSurface), dbgName, VK_OBJECT_TYPE_SURFACE_KHR );

			AE_LOGD( "Created WinAPI Vulkan surface" );
		}
		#elif defined(PLATFORM_ANDROID)
		{
			CHECK_ERR( window.nativeWindow );

			VkAndroidSurfaceCreateInfoKHR	surface_info = {};

			surface_info.sType	= VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
			surface_info.flags	= 0;
			surface_info.window	= static_cast<ANativeWindow *>( window.nativeWindow );

			auto  fpCreateAndroidSurfaceKHR = BitCast<PFN_vkCreateAndroidSurfaceKHR>( vkGetInstanceProcAddr( _device.GetVkInstance(), "vkCreateAndroidSurfaceKHR" ));
			CHECK_ERR( fpCreateAndroidSurfaceKHR );

			VK_CHECK( fpCreateAndroidSurfaceKHR( _device.GetVkInstance(), &surface_info, null, OUT &_vkSurface ));
			
			if ( dbgName.size() )
				_device.SetObjectName( BitCast<uint64_t>(_vkSurface), dbgName, VK_OBJECT_TYPE_SURFACE_KHR );
			
			AE_LOGD( "Created Android Vulkan surface" );
		}
		#else
			return false;
		#endif

		// check that surface supported with current device
		bool	present_supported = false;

		for (auto& q : _device.GetVkQueues())
		{
			VkBool32	supported = 0;
			VK_CALL( vkGetPhysicalDeviceSurfaceSupportKHR( _device.GetVkPhysicalDevice(), uint(q.familyIndex), _vkSurface, OUT &supported ));

			if ( supported ) {
				present_supported = true;
				break;
			}
		}

		CHECK_ERR( present_supported );	
		return true;
	}
	
/*
=================================================
	GetInstanceExtensions
=================================================
*/
	ArrayView<const char*>  VSwapchainInitializer::GetInstanceExtensions ()
	{
		static const char*	extensions[] = {
			#ifdef PLATFORM_WINDOWS
				VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
			#endif
			#ifdef PLATFORM_ANDROID
				VK_KHR_ANDROID_SURFACE_EXTENSION_NAME,
			#endif
			#ifdef PLATFORM_LINUX
				VK_KHR_XLIB_SURFACE_EXTENSION_NAME,
			#endif
		};
		return extensions;
	}

/*
=================================================
	DestroySurface
=================================================
*/
	void  VSwapchainInitializer::DestroySurface ()
	{
		CHECK( not _vkSwapchain );

		if ( _vkSurface )
		{
			vkDestroySurfaceKHR( _device.GetVkInstance(), _vkSurface, null );
			_vkSurface = VK_NULL_HANDLE;
			
			AE_LOGD( "Destroyed Vulkan surface" );
		}
	}

/*
=================================================
	ChooseColorFormat
=================================================
*/
	bool  VSwapchainInitializer::ChooseColorFormat (INOUT VkFormat &colorFormat, INOUT VkColorSpaceKHR &colorSpace) const
	{
		CHECK_ERR( _device.GetVkPhysicalDevice() and _vkSurface );

		uint						count				= 0;
		Array< VkSurfaceFormatKHR >	surf_formats;
		const VkFormat				def_format			= VK_FORMAT_B8G8R8A8_UNORM;
		const VkColorSpaceKHR		def_space			= VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
		const VkFormat				required_format		= colorFormat;
		const VkColorSpaceKHR		required_colorspace	= colorSpace;

		VK_CHECK( vkGetPhysicalDeviceSurfaceFormatsKHR( _device.GetVkPhysicalDevice(), _vkSurface, OUT &count, null ));
		CHECK_ERR( count > 0 );

		surf_formats.resize( count );
		VK_CHECK( vkGetPhysicalDeviceSurfaceFormatsKHR( _device.GetVkPhysicalDevice(), _vkSurface, OUT &count, OUT surf_formats.data() ));
		
		if ( count == 1		and
			 surf_formats[0].format == VK_FORMAT_UNDEFINED )
		{
			colorFormat = required_format;
			colorSpace  = surf_formats[0].colorSpace;
		}
		else
		{
			const usize	max_idx				= UMax;
			usize		both_match_idx		= max_idx;
			usize		format_match_idx	= max_idx;
			usize		space_match_idx		= max_idx;
			usize		def_format_idx		= 0;
			usize		def_space_idx		= 0;

			for (usize i = 0; i < surf_formats.size(); ++i)
			{
				const auto&	surf_fmt = surf_formats[i];

				if ( surf_fmt.format	 == required_format		and
					 surf_fmt.colorSpace == required_colorspace )
				{
					both_match_idx = i;
				}
				else
				// separate check
				if ( surf_fmt.format	 == required_format )
					format_match_idx = i;
				else
				if ( surf_fmt.colorSpace == required_colorspace )
					space_match_idx = i;

				// check with default
				if ( surf_fmt.format	 == def_format )
					def_format_idx = i;

				if ( surf_fmt.colorSpace == def_space )
					def_space_idx = i;
			}

			usize	idx = 0;

			if ( both_match_idx != max_idx )
				idx = both_match_idx;
			else
			if ( format_match_idx != max_idx )
				idx = format_match_idx;
			else
			if ( def_format_idx != max_idx )
				idx = def_format_idx;

			// TODO: space_match_idx and def_space_idx are unused yet

			colorFormat	= surf_formats[ idx ].format;
			colorSpace	= surf_formats[ idx ].colorSpace;
		}

		return true;
	}
	
/*
=================================================
	IsSupported
=================================================
*/
	bool  VSwapchainInitializer::IsSupported (const VkSampleCountFlagBits samples, const VkPresentModeKHR presentMode,
											  const VkFormat colorFormat, const VkImageUsageFlagBits colorImageUsage) const
	{
		CHECK_ERR( _device.GetVkPhysicalDevice() and _vkSurface );

		VkSurfaceCapabilitiesKHR	surf_caps;
		VK_CHECK( vkGetPhysicalDeviceSurfaceCapabilitiesKHR( _device.GetVkPhysicalDevice(), _vkSurface, OUT &surf_caps ));
		
		VkImageUsageFlags	image_usage;
		_GetImageUsage( OUT image_usage, presentMode, colorFormat, surf_caps );

		if ( not AllBits( image_usage, colorImageUsage ))
			return false;

		VkImageFormatProperties	image_props = {};
		VK_CALL( vkGetPhysicalDeviceImageFormatProperties( _device.GetVkPhysicalDevice(), colorFormat, VK_IMAGE_TYPE_2D,
														   VK_IMAGE_TILING_OPTIMAL, colorImageUsage, 0, OUT &image_props ));

		if ( not AllBits( image_props.sampleCounts, samples ))
			return false;

		//if ( imageArrayLayers < image_props.maxArrayLayers )
		//	return false;

		return true;
	}
	
/*
=================================================
	Create
=================================================
*/
	bool  VSwapchainInitializer::Create (VResourceManager *						resMngr,
										 const uint2							&viewSize,
										 const VkFormat							colorFormat,
										 const VkColorSpaceKHR					colorSpace,
										 const uint								minImageCount,
										 const VkPresentModeKHR					presentMode,
										 const VkSurfaceTransformFlagBitsKHR	transform,
										 const VkCompositeAlphaFlagBitsKHR		compositeAlpha,
										 const VkImageUsageFlagBits				colorImageUsage,
										 StringView								dbgName)
	{
		CHECK_ERR( _device.GetVkPhysicalDevice() and _device.GetVkDevice() and _vkSurface );
		CHECK_ERR( _device.GetFeatures().swapchain );
		CHECK_ERR( not IsImageAcquired() );

		VkSurfaceCapabilitiesKHR	surf_caps;
		VK_CHECK( vkGetPhysicalDeviceSurfaceCapabilitiesKHR( _device.GetVkPhysicalDevice(), _vkSurface, OUT &surf_caps ));

		VkSwapchainKHR				old_swapchain	= _vkSwapchain;
		VkSwapchainCreateInfoKHR	swapchain_info	= {};
		
		swapchain_info.sType			= VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapchain_info.pNext			= null;
		swapchain_info.surface			= _vkSurface;
		swapchain_info.imageFormat		= colorFormat;
		swapchain_info.imageColorSpace	= colorSpace;
		swapchain_info.imageExtent		= { viewSize.x, viewSize.y };
		swapchain_info.imageArrayLayers	= 1;
		swapchain_info.minImageCount	= minImageCount;
		swapchain_info.oldSwapchain		= old_swapchain;
		swapchain_info.clipped			= VK_TRUE;
		swapchain_info.preTransform		= transform;
		swapchain_info.presentMode		= presentMode;
		swapchain_info.compositeAlpha	= compositeAlpha;
		swapchain_info.imageSharingMode	= VK_SHARING_MODE_EXCLUSIVE;

		_GetSurfaceImageCount( INOUT swapchain_info.minImageCount, surf_caps );
		_GetSurfaceTransform( INOUT swapchain_info.preTransform, surf_caps );
		_GetSwapChainExtent( INOUT swapchain_info.imageExtent, surf_caps );
		_GetPresentMode( INOUT swapchain_info.presentMode );
		CHECK_ERR( _GetImageUsage( OUT swapchain_info.imageUsage, swapchain_info.presentMode, colorFormat, surf_caps ));
		CHECK_ERR( _GetCompositeAlpha( INOUT swapchain_info.compositeAlpha, surf_caps ));
		
		swapchain_info.imageUsage &= colorImageUsage;

		VK_CHECK( vkCreateSwapchainKHR( _device.GetVkDevice(), &swapchain_info, null, OUT &_vkSwapchain ));


		// destroy obsolete resources
		if ( resMngr )
		{
			for (auto& id : _imageIDs) {
				CHECK( resMngr->ReleaseResource( id ));
			}
		}

		_images.clear();
		_imageIDs.clear();

		if ( old_swapchain != VK_NULL_HANDLE )
			vkDestroySwapchainKHR( _device.GetVkDevice(), old_swapchain, null );


		_colorFormat		= colorFormat;
		_colorSpace			= colorSpace;
		_minImageCount		= swapchain_info.minImageCount;
		_preTransform		= swapchain_info.preTransform;
		_presentMode		= swapchain_info.presentMode;
		_compositeAlpha		= swapchain_info.compositeAlpha;
		_colorImageUsage	= BitCast<VkImageUsageFlagBits>( swapchain_info.imageUsage );
		_surfaceSize.x		= swapchain_info.imageExtent.width;
		_surfaceSize.y		= swapchain_info.imageExtent.height;
		
		if ( dbgName.size() )
			_device.SetObjectName( BitCast<uint64_t>(_vkSwapchain), dbgName, VK_OBJECT_TYPE_SWAPCHAIN_KHR );

		CHECK_ERR( _CreateColorAttachment( resMngr ));

		if ( not old_swapchain )
			_PrintInfo();

		CHECK_ERR( _CreateSemaphores() );

		return true;
	}
	
/*
=================================================
	Destroy
=================================================
*/
	void  VSwapchainInitializer::Destroy (VResourceManager *resMngr)
	{
		CHECK( not IsImageAcquired() );

		if ( _device.GetVkDevice() )
		{
			if ( _vkSwapchain != VK_NULL_HANDLE )
			{
				vkDestroySwapchainKHR( _device.GetVkDevice(), _vkSwapchain, null );
				
				AE_LOGD( "Destroyed Vulkan swapchain" );
			}

			_DestroySemaphores();
		}
		
		if ( resMngr )
		{
			for (auto& id : _imageIDs) {
				CHECK( resMngr->ReleaseResource( id ));
			}
		}

		_imageIDs.clear();
		_images.clear();

		_vkSwapchain		= VK_NULL_HANDLE;
		_surfaceSize		= uint2();

		_currImageIndex		= UMax;
		
		_colorFormat		= VK_FORMAT_UNDEFINED;
		_colorSpace			= VK_COLOR_SPACE_MAX_ENUM_KHR;
		_minImageCount		= 0;
		_preTransform		= VK_SURFACE_TRANSFORM_FLAG_BITS_MAX_ENUM_KHR;
		_presentMode		= VK_PRESENT_MODE_MAX_ENUM_KHR;
		_compositeAlpha		= VK_COMPOSITE_ALPHA_FLAG_BITS_MAX_ENUM_KHR;
		_colorImageUsage	= Zero;
	}

/*
=================================================
	Recreate
----
	warning: you must delete all command buffers
	that used swapchain images!
=================================================
*/
	bool  VSwapchainInitializer::Recreate (VResourceManager *resMngr, const uint2 &size)
	{
		CHECK_ERR( Create( resMngr, size, _colorFormat, _colorSpace, _minImageCount, _presentMode,
						   _preTransform, _compositeAlpha, _colorImageUsage ));

		return true;
	}
	
/*
=================================================
	_CreateColorAttachment
=================================================
*/
	bool  VSwapchainInitializer::_CreateColorAttachment (VResourceManager *resMngr)
	{
		CHECK_ERR( _images.empty() );
		
		uint	count = 0;
		VK_CHECK( vkGetSwapchainImagesKHR( _device.GetVkDevice(), _vkSwapchain, OUT &count, null ));
		CHECK_ERR( count > 0 );
		
		_images.resize( count );
		VK_CHECK( vkGetSwapchainImagesKHR( _device.GetVkDevice(), _vkSwapchain, OUT &count, OUT _images.data() ));

		if ( resMngr )
		{
			_imageIDs.resize( count );

			VulkanImageDesc	desc;
			desc.dimension		= uint3(_surfaceSize, 1);
			desc.arrayLayers	= 1;
			desc.currentLayout	= VK_IMAGE_LAYOUT_UNDEFINED;
			desc.defaultLayout	= VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			desc.flags			= VkImageCreateFlagBits(0);
			desc.format			= _colorFormat;
			desc.imageType		= VK_IMAGE_TYPE_2D;
			desc.maxLevels		= 1;
			desc.samples		= VK_SAMPLE_COUNT_1_BIT;
			desc.usage			= _colorImageUsage;
			desc.canBeDestroyed	= false;	// images created by swapchain, so don't destroy they

			for (usize i = 0; i < count; ++i)
			{
				desc.image = _images[i];
				_imageIDs[i] = resMngr->CreateImage( desc, "SwapchainImage_"s + ToString(i) );
			}
		}
		return true;
	}

/*
=================================================
	_GetCompositeAlpha
=================================================
*/
	bool  VSwapchainInitializer::_GetCompositeAlpha (INOUT VkCompositeAlphaFlagBitsKHR &compositeAlpha,
													 const VkSurfaceCapabilitiesKHR &surfaceCaps) const
	{
		const VkCompositeAlphaFlagBitsKHR		composite_alpha_flags[] = {
			VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
			VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
			VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
			VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
		};
		
		if ( AllBits( surfaceCaps.supportedCompositeAlpha, compositeAlpha ))
			return true;	// keep current
		
		compositeAlpha = VK_COMPOSITE_ALPHA_FLAG_BITS_MAX_ENUM_KHR;

		for (auto& flag : composite_alpha_flags)
		{
			if ( AllBits( surfaceCaps.supportedCompositeAlpha, flag ))
			{
				compositeAlpha = flag;
				return true;
			}
		}

		RETURN_ERR( "no suitable composite alpha flags found!" );
	}
	
/*
=================================================
	_GetSwapChainExtent
=================================================
*/
	void  VSwapchainInitializer::_GetSwapChainExtent (INOUT VkExtent2D &extent,
													  const VkSurfaceCapabilitiesKHR &surfaceCaps) const
	{
		if ( surfaceCaps.currentExtent.width  == UMax and
			 surfaceCaps.currentExtent.height == UMax )
		{
			// keep window size
		}
		else
		{
			extent.width  = surfaceCaps.currentExtent.width;
			extent.height = surfaceCaps.currentExtent.height;
		}
	}
	
/*
=================================================
	_GetPresentMode
=================================================
*/
	void  VSwapchainInitializer::_GetPresentMode (INOUT VkPresentModeKHR &presentMode) const
	{
		uint						count		= 0;
		Array< VkPresentModeKHR >	present_modes;

		VK_CALL( vkGetPhysicalDeviceSurfacePresentModesKHR( _device.GetVkPhysicalDevice(), _vkSurface, OUT &count, null ));
		CHECK_ERRV( count > 0 );

		present_modes.resize( count );
		VK_CALL( vkGetPhysicalDeviceSurfacePresentModesKHR( _device.GetVkPhysicalDevice(), _vkSurface, OUT &count, OUT present_modes.data() ));

		bool	required_mode_supported		= false;
		bool	fifo_mode_supported			= false;
		bool	mailbox_mode_supported		= false;
		bool	immediate_mode_supported	= false;

		for (usize i = 0; i < present_modes.size(); ++i)
		{
			required_mode_supported		|= (present_modes[i] == presentMode);
			fifo_mode_supported			|= (present_modes[i] == VK_PRESENT_MODE_FIFO_KHR);
			mailbox_mode_supported		|= (present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR);
			immediate_mode_supported	|= (present_modes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR);
		}

		if ( required_mode_supported )
			return;	// keep current

		if ( mailbox_mode_supported )
			presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
		else
		if ( fifo_mode_supported )
			presentMode = VK_PRESENT_MODE_FIFO_KHR;
		else
		if ( immediate_mode_supported )
			presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
	}

/*
=================================================
	_GetSurfaceImageCount
=================================================
*/
	void  VSwapchainInitializer::_GetSurfaceImageCount (INOUT uint &minImageCount, const VkSurfaceCapabilitiesKHR &surfaceCaps) const
	{
		if ( minImageCount < surfaceCaps.minImageCount )
		{
			minImageCount = surfaceCaps.minImageCount;
		}

		if ( surfaceCaps.maxImageCount > 0 and minImageCount > surfaceCaps.maxImageCount )
		{
			minImageCount = surfaceCaps.maxImageCount;
		}
	}
	
/*
=================================================
	_GetSurfaceTransform
=================================================
*/
	void  VSwapchainInitializer::_GetSurfaceTransform (INOUT VkSurfaceTransformFlagBitsKHR &transform,
													   const VkSurfaceCapabilitiesKHR &surfaceCaps) const
	{
		if ( AllBits( surfaceCaps.supportedTransforms, transform ))
			return;	// keep current
		
		if ( AllBits( surfaceCaps.supportedTransforms, VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR ))
		{
			transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
		}
		else
		{
			transform = surfaceCaps.currentTransform;
		}
	}
	
/*
=================================================
	_GetImageUsage
=================================================
*/
	bool  VSwapchainInitializer::_GetImageUsage (OUT VkImageUsageFlags &imageUsage, const VkPresentModeKHR presentMode,
												 const VkFormat colorFormat, const VkSurfaceCapabilitiesKHR &surfaceCaps) const
	{
		if ( presentMode == VK_PRESENT_MODE_IMMEDIATE_KHR	or
			 presentMode == VK_PRESENT_MODE_MAILBOX_KHR		or
			 presentMode == VK_PRESENT_MODE_FIFO_KHR		or
			 presentMode == VK_PRESENT_MODE_FIFO_RELAXED_KHR )
		{
			imageUsage = surfaceCaps.supportedUsageFlags;
		}
		else
		if ( presentMode == VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR	or
			 presentMode == VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR )
		{
			if ( not _device.GetFeatures().surfaceCaps2 )
				return false;
			
			VkPhysicalDeviceSurfaceInfo2KHR	surf_info = {};
			surf_info.sType		= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR;
			surf_info.surface	= _vkSurface;

			VkSurfaceCapabilities2KHR	surf_caps2;
			VK_CALL( vkGetPhysicalDeviceSurfaceCapabilities2KHR( _device.GetVkPhysicalDevice(), &surf_info, OUT &surf_caps2 ));

			for (VkBaseInStructure const *iter = reinterpret_cast<VkBaseInStructure const *>(&surf_caps2);
					iter != null;
					iter = iter->pNext)
			{
				if ( iter->sType == VK_STRUCTURE_TYPE_SHARED_PRESENT_SURFACE_CAPABILITIES_KHR )
				{
					imageUsage = reinterpret_cast<VkSharedPresentSurfaceCapabilitiesKHR const*>(iter)->sharedPresentSupportedUsageFlags;
					break;
				}
			}
		}
		else
		{
			return false;
		}

		ASSERT( AllBits( imageUsage, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT ));
		imageUsage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		

		// validation:
		VkFormatProperties	format_props;
		vkGetPhysicalDeviceFormatProperties( _device.GetVkPhysicalDevice(), colorFormat, OUT &format_props );

		CHECK_ERR( AllBits( format_props.optimalTilingFeatures, VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT ));
		ASSERT( AllBits( format_props.optimalTilingFeatures, VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT ));
		
		if ( AllBits( imageUsage, VK_IMAGE_USAGE_TRANSFER_SRC_BIT ) and
			 (not AllBits( format_props.optimalTilingFeatures, VK_FORMAT_FEATURE_TRANSFER_SRC_BIT ) or
			  not AllBits( format_props.optimalTilingFeatures, VK_FORMAT_FEATURE_BLIT_DST_BIT )) )
		{
			imageUsage &= ~VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		}
		
		if ( AllBits( imageUsage, VK_IMAGE_USAGE_TRANSFER_DST_BIT ) and
			 not AllBits( format_props.optimalTilingFeatures, VK_FORMAT_FEATURE_TRANSFER_DST_BIT ))
		{
			imageUsage &= ~VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		}
		
		if ( AllBits( imageUsage, VK_IMAGE_USAGE_STORAGE_BIT ) and
			 not AllBits( format_props.optimalTilingFeatures, VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT ))
		{
			imageUsage &= ~VK_IMAGE_USAGE_STORAGE_BIT;
		}

		if ( AllBits( imageUsage, VK_IMAGE_USAGE_SAMPLED_BIT ) and
			 not AllBits( format_props.optimalTilingFeatures, VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT ))
		{
			imageUsage &= ~VK_IMAGE_USAGE_SAMPLED_BIT;
		}

		if ( AllBits( imageUsage, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT ) and
			 not AllBits( format_props.optimalTilingFeatures, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT ))
		{
			imageUsage &= ~VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		}

		return true;
	}
	
/*
=================================================
	_PrintInfo
=================================================
*/
	void  VSwapchainInitializer::_PrintInfo () const
	{
		AE_LOGI( "Created Vulkan Swapchain:"s
			<< "\n  format:          " << VkFormat_ToString( _colorFormat )
			<< "\n  color space:     " << VkColorSpaceKHR_ToString( _colorSpace )
			<< "\n  present mode:    " << VkPresentModeKHR_ToString( _presentMode )
			<< "\n  transform:       " << VkSurfaceTransformFlagBitsKHR_ToString( _preTransform )
			<< "\n  composite alpha: " << VkCompositeAlphaFlagBitsKHR_ToString( _compositeAlpha )
			<< "\n  image usage:     " << VkImageUsageFlags_ToString( _colorImageUsage )
			<< "\n  min image count: " << ToString( _minImageCount )
		);
	}
	
/*
=================================================
	_CreateSemaphores
=================================================
*/
	bool  VSwapchainInitializer::_CreateSemaphores ()
	{
		VkSemaphoreCreateInfo	info = {};
		info.sType	= VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		for (auto& id : _semaphores)
		{
			if ( id != VK_NULL_HANDLE )
				continue;

			VK_CHECK( vkCreateSemaphore( _device.GetVkDevice(), &info, null, &id ));
		}
		return true;
	}
	
/*
=================================================
	_DestroySemaphores
=================================================
*/
	void  VSwapchainInitializer::_DestroySemaphores ()
	{
		for (auto& id : _semaphores)
		{
			if ( id )
			{
				vkDestroySemaphore( _device.GetVkDevice(), id, null );
				id = VK_NULL_HANDLE;
			}
		}
	}


}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
