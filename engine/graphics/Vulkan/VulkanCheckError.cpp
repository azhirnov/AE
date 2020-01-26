// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#ifdef AE_ENABLE_VULKAN

# include "graphics/Vulkan/VulkanLoader.h"
# include "stl/Algorithms/StringUtils.h"

# define VK_NO_PROTOTYPES
# include <vulkan/vulkan.h>

namespace AE::Graphics
{

/*
=================================================
	__vk_CheckErrors
=================================================
*/
	bool __vk_CheckErrors (VkResult errCode, const char *vkcall, const char *func, const char *file, int line)
	{
		if ( errCode == VK_SUCCESS )
			return true;

		#define VK1_CASE_ERR( _code_ ) \
			case _code_ :	msg += AE_PRIVATE_TOSTRING( _code_ ); break;
		
		String	msg( "Vulkan error: " );

		BEGIN_ENUM_CHECKS();
		switch ( errCode )
		{
			VK1_CASE_ERR( VK_NOT_READY )
			VK1_CASE_ERR( VK_TIMEOUT )
			VK1_CASE_ERR( VK_EVENT_SET )
			VK1_CASE_ERR( VK_EVENT_RESET )
			VK1_CASE_ERR( VK_INCOMPLETE )
			VK1_CASE_ERR( VK_ERROR_OUT_OF_HOST_MEMORY )
			VK1_CASE_ERR( VK_ERROR_OUT_OF_DEVICE_MEMORY )
			VK1_CASE_ERR( VK_ERROR_INITIALIZATION_FAILED )
			VK1_CASE_ERR( VK_ERROR_DEVICE_LOST )
			VK1_CASE_ERR( VK_ERROR_MEMORY_MAP_FAILED )
			VK1_CASE_ERR( VK_ERROR_LAYER_NOT_PRESENT )
			VK1_CASE_ERR( VK_ERROR_EXTENSION_NOT_PRESENT )
			VK1_CASE_ERR( VK_ERROR_FEATURE_NOT_PRESENT )
			VK1_CASE_ERR( VK_ERROR_INCOMPATIBLE_DRIVER )
			VK1_CASE_ERR( VK_ERROR_TOO_MANY_OBJECTS )
			VK1_CASE_ERR( VK_ERROR_FORMAT_NOT_SUPPORTED )
			VK1_CASE_ERR( VK_ERROR_FRAGMENTED_POOL )
			VK1_CASE_ERR( VK_ERROR_SURFACE_LOST_KHR )
			VK1_CASE_ERR( VK_ERROR_NATIVE_WINDOW_IN_USE_KHR )
			VK1_CASE_ERR( VK_SUBOPTIMAL_KHR )
			VK1_CASE_ERR( VK_ERROR_OUT_OF_DATE_KHR )
			VK1_CASE_ERR( VK_ERROR_INCOMPATIBLE_DISPLAY_KHR )
			VK1_CASE_ERR( VK_ERROR_VALIDATION_FAILED_EXT )
			VK1_CASE_ERR( VK_ERROR_INVALID_SHADER_NV )
			VK1_CASE_ERR( VK_ERROR_OUT_OF_POOL_MEMORY )
			VK1_CASE_ERR( VK_ERROR_INVALID_EXTERNAL_HANDLE )
			VK1_CASE_ERR( VK_ERROR_FRAGMENTATION_EXT )
			VK1_CASE_ERR( VK_ERROR_NOT_PERMITTED_EXT )

			#ifdef VK_EXT_image_drm_format_modifier
			VK1_CASE_ERR( VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT )
			#endif

			#ifdef VK_EXT_buffer_device_address
			VK1_CASE_ERR( VK_ERROR_INVALID_DEVICE_ADDRESS_KHR )
			#endif

			#ifdef VK_EXT_full_screen_exclusive
			VK1_CASE_ERR( VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT )
			#endif

			#ifdef VK_VERSION_1_2
			VK1_CASE_ERR( VK_ERROR_UNKNOWN )
			#endif

			case VK_SUCCESS :
			case VK_RESULT_RANGE_SIZE :
			case VK_RESULT_MAX_ENUM :
			default :	msg = msg + "unknown (" + ToString(int(errCode)) + ')';  break;
		}
		END_ENUM_CHECKS();
		#undef VK1_CASE_ERR
		
		msg = msg + ", in " + vkcall + ", function: " + func;

		AE_LOGE( msg, file, line );
		return false;
	}

}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
