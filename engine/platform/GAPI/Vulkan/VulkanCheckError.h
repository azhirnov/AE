// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef AE_ENABLE_VULKAN

#if 0 //ndef AE_DEBUG
#	define VK_CALL( ... )		{ (void)(__VA_ARGS__); }
#	define VK_CHECK( ... )		{ if ((__VA_ARGS__) != VK_SUCCESS) return false; }

#else
#	define VK_CALL( ... ) \
	{ \
		const ::VkResult __vk_err__ = (__VA_ARGS__); \
		::AE::Vulkan::__vk_CheckErrors( __vk_err__, AE_PRIVATE_TOSTRING( __VA_ARGS__ ), AE_FUNCTION_NAME, __FILE__, __LINE__ ); \
	}

#	define AE_PRIVATE_VK_CALL_R( _func_, _ret_, ... ) \
	{ \
		const ::VkResult __vk_err__ = (_func_); \
		if ( not ::AE::Vulkan::__vk_CheckErrors( __vk_err__, AE_PRIVATE_TOSTRING( _func_ ), AE_FUNCTION_NAME, __FILE__, __LINE__ ) ) \
			return _ret_; \
	}

#	define VK_CHECK( ... ) \
		AE_PRIVATE_VK_CALL_R( AE_PRIVATE_GETARG_0( __VA_ARGS__ ), AE_PRIVATE_GETARG_1( __VA_ARGS__, ::AE::STL::Default ) )
#endif


namespace AE::Vulkan
{
	bool __vk_CheckErrors (VkResult errCode, const char *vkcall, const char *func, const char *file, int line);
	
}	// AE::Vulkan

#endif	// AE_ENABLE_VULKAN
