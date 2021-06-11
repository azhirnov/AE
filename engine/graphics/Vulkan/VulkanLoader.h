// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef AE_ENABLE_VULKAN

# include "stl/Common.h"
# include "stl/Containers/NtStringView.h"
# include "stl/Math/BitMath.h"

# if not defined(VK_NO_PROTOTYPES) and defined(VULKAN_CORE_H_)
#	error invalid configuration, include vulkan.h after this file.
# endif

# ifndef VK_NO_PROTOTYPES
#	define VK_NO_PROTOTYPES
# endif

# ifdef COMPILER_MSVC
#	pragma warning (push, 0)
#	include <vulkan/vulkan.h>
#	pragma warning (pop)
# else
#	include <vulkan/vulkan.h>
# endif

namespace AE::Graphics
{
	using namespace AE::STL;

#	define VKLOADER_STAGE_DECLFNPOINTER
#	 include "vulkan_loader/fn_vulkan_lib.h"
#	 include "vulkan_loader/fn_vulkan_inst.h"
#	undef  VKLOADER_STAGE_DECLFNPOINTER

#	define VKLOADER_STAGE_INLINEFN
#	 include "vulkan_loader/fn_vulkan_lib.h"
#	 include "vulkan_loader/fn_vulkan_inst.h"
#	undef  VKLOADER_STAGE_INLINEFN



	//
	// Vulkan Device Functions Table
	//
	struct VulkanDeviceFnTable final
	{
		friend struct VulkanLoader;
		friend class VulkanDeviceFn;

	// variables
	private:
#		define VKLOADER_STAGE_FNPOINTER
#		 include "vulkan_loader/fn_vulkan_dev.h"
#		undef  VKLOADER_STAGE_FNPOINTER


	// methods
	public:
		VulkanDeviceFnTable () {}

		VulkanDeviceFnTable (const VulkanDeviceFnTable &) = delete;
		VulkanDeviceFnTable (VulkanDeviceFnTable &&) = delete;

		VulkanDeviceFnTable&  operator = (const VulkanDeviceFnTable &) = delete;
		VulkanDeviceFnTable&  operator = (VulkanDeviceFnTable &&) = delete;
	};



	//
	// Vulkan Device Functions
	//
	class VulkanDeviceFn
	{
	// variables
	private:
		VulkanDeviceFnTable *		_table;

	// methods
	protected:
		void  VulkanDeviceFn_Init (const VulkanDeviceFn &other);
		void  VulkanDeviceFn_Init (VulkanDeviceFnTable *table);

	public:
		VulkanDeviceFn () : _table{null} {}
		VulkanDeviceFn (const VulkanDeviceFn &) = default;
		explicit VulkanDeviceFn (VulkanDeviceFnTable *table) : _table{table} {}

#		define VKLOADER_STAGE_INLINEFN
#		 include "vulkan_loader/fn_vulkan_dev.h"
#		undef  VKLOADER_STAGE_INLINEFN
	};



	//
	// Vulkan Loader
	//
	struct VulkanLoader final
	{
		VulkanLoader () = delete;

		ND_ static bool  Initialize (NtStringView libName = {});
		ND_ static bool  LoadInstance (VkInstance instance);
			static void  Unload ();
		
		ND_ static bool  LoadDevice (VkDevice device, OUT VulkanDeviceFnTable &table);
			static void  ResetDevice (OUT VulkanDeviceFnTable &table);
		
			static void  SetupInstanceBackwardCompatibility (uint version);
			static void  SetupDeviceBackwardCompatibility (uint version, INOUT VulkanDeviceFnTable &table);
	};

}	// AE::Graphics

	
	// debugger can't show enum names for VkFlags, so use enum instead
#define VULKAN_ENUM_BIT_OPERATORS( _type_ ) \
			inline constexpr _type_&  operator |= (_type_ &lhs, _type_ rhs) { return lhs = _type_( AE::Math::ToNearUInt( lhs ) | AE::Math::ToNearUInt( rhs )); } \
		ND_ inline constexpr _type_   operator |  (_type_ lhs, _type_ rhs)	{ return _type_( AE::Math::ToNearUInt( lhs ) | AE::Math::ToNearUInt( rhs )); } \
			inline constexpr _type_&  operator &= (_type_ &lhs, _type_ rhs) { return lhs = _type_( AE::Math::ToNearUInt( lhs ) & AE::Math::ToNearUInt( rhs )); } \
		ND_ inline constexpr _type_   operator &  (_type_ lhs, _type_ rhs)	{ return _type_( AE::Math::ToNearUInt( lhs ) & AE::Math::ToNearUInt( rhs )); } \
		ND_ inline constexpr _type_   operator ~  (_type_ value)			{ return _type_( ~AE::Math::ToNearUInt( value )); } \

	VULKAN_ENUM_BIT_OPERATORS( VkPipelineStageFlagBits );
	VULKAN_ENUM_BIT_OPERATORS( VkAccessFlagBits );
	VULKAN_ENUM_BIT_OPERATORS( VkDependencyFlagBits );
	VULKAN_ENUM_BIT_OPERATORS( VkImageAspectFlagBits );
	VULKAN_ENUM_BIT_OPERATORS( VkStencilFaceFlagBits );
	VULKAN_ENUM_BIT_OPERATORS( VkShaderStageFlagBits );
	VULKAN_ENUM_BIT_OPERATORS( VkImageCreateFlagBits );
	VULKAN_ENUM_BIT_OPERATORS( VkQueueFlagBits );
	VULKAN_ENUM_BIT_OPERATORS( VkImageUsageFlagBits );
	VULKAN_ENUM_BIT_OPERATORS( VkBufferUsageFlagBits );
	VULKAN_ENUM_BIT_OPERATORS( VkSampleCountFlagBits );
	
#	ifdef VK_NV_ray_tracing
	VULKAN_ENUM_BIT_OPERATORS( VkGeometryFlagBitsNV );
	VULKAN_ENUM_BIT_OPERATORS( VkBuildAccelerationStructureFlagBitsNV );
#	endif
	
#undef VULKAN_ENUM_BIT_OPERATORS


// check for 'VulkanDeviceFnTable' structure size mismatch
# ifdef AE_CPP_DETECT_MISMATCH

#  if defined(VK_USE_PLATFORM_ANDROID_KHR) and VK_USE_PLATFORM_ANDROID_KHR
#	pragma detect_mismatch( "VK_USE_PLATFORM_ANDROID_KHR", "1" )
#  else
#	pragma detect_mismatch( "VK_USE_PLATFORM_ANDROID_KHR", "0" )
#  endif

#  if defined(VK_USE_PLATFORM_IOS_MVK) and VK_USE_PLATFORM_IOS_MVK
#	pragma detect_mismatch( "VK_USE_PLATFORM_IOS_MVK", "1" )
#  else
#	pragma detect_mismatch( "VK_USE_PLATFORM_IOS_MVK", "0" )
#  endif

#  if defined(VK_USE_PLATFORM_MACOS_MVK) and VK_USE_PLATFORM_MACOS_MVK
#	pragma detect_mismatch( "VK_USE_PLATFORM_MACOS_MVK", "1" )
#  else
#	pragma detect_mismatch( "VK_USE_PLATFORM_MACOS_MVK", "0" )
#  endif

#  if defined(VK_USE_PLATFORM_MIR_KHR) and VK_USE_PLATFORM_MIR_KHR
#	pragma detect_mismatch( "VK_USE_PLATFORM_MIR_KHR", "1" )
#  else
#	pragma detect_mismatch( "VK_USE_PLATFORM_MIR_KHR", "0" )
#  endif

#  if defined(VK_USE_PLATFORM_VI_NN) and VK_USE_PLATFORM_VI_NN
#	pragma detect_mismatch( "VK_USE_PLATFORM_VI_NN", "1" )
#  else
#	pragma detect_mismatch( "VK_USE_PLATFORM_VI_NN", "0" )
#  endif

#  if defined(VK_USE_PLATFORM_WAYLAND_KHR) and VK_USE_PLATFORM_WAYLAND_KHR
#	pragma detect_mismatch( "VK_USE_PLATFORM_WAYLAND_KHR", "1" )
#  else
#	pragma detect_mismatch( "VK_USE_PLATFORM_WAYLAND_KHR", "0" )
#  endif

#  if defined(VK_USE_PLATFORM_WIN32_KHR) and VK_USE_PLATFORM_WIN32_KHR
#	pragma detect_mismatch( "VK_USE_PLATFORM_WIN32_KHR", "1" )
#  else
#	pragma detect_mismatch( "VK_USE_PLATFORM_WIN32_KHR", "0" )
#  endif

#  if defined(VK_USE_PLATFORM_XCB_KHR) and VK_USE_PLATFORM_XCB_KHR
#	pragma detect_mismatch( "VK_USE_PLATFORM_XCB_KHR", "1" )
#  else
#	pragma detect_mismatch( "VK_USE_PLATFORM_XCB_KHR", "0" )
#  endif

#  if defined(VK_USE_PLATFORM_XLIB_KHR) and VK_USE_PLATFORM_XLIB_KHR
#	pragma detect_mismatch( "VK_USE_PLATFORM_XLIB_KHR", "1" )
#  else
#	pragma detect_mismatch( "VK_USE_PLATFORM_XLIB_KHR", "0" )
#  endif

#  if defined(VK_USE_PLATFORM_XLIB_XRANDR_EXT) and VK_USE_PLATFORM_XLIB_XRANDR_EXT
#	pragma detect_mismatch( "VK_USE_PLATFORM_XLIB_XRANDR_EXT", "1" )
#  else
#	pragma detect_mismatch( "VK_USE_PLATFORM_XLIB_XRANDR_EXT", "0" )
#  endif

#  ifdef VK_VERSION_1_1
#	pragma detect_mismatch( "VK_VERSION_1_1", "1" )
#  else
#	pragma detect_mismatch( "VK_VERSION_1_1", "0" )
#  endif
	
#  ifdef VK_VERSION_1_2
#	pragma detect_mismatch( "VK_VERSION_1_2", "1" )
#  else
#	pragma detect_mismatch( "VK_VERSION_1_2", "0" )
#  endif

#  ifdef AE_ENABLE_VULKAN
#	pragma detect_mismatch( "AE_ENABLE_VULKAN", "1" )
#  else
#	pragma detect_mismatch( "AE_ENABLE_VULKAN", "0" )
#  endif
#endif	// AE_CPP_DETECT_MISSMATCH
#endif	// AE_ENABLE_VULKAN
