// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#ifdef AE_ENABLE_VULKAN

# include "platform/GAPI/Vulkan/VulkanLoader.h"
# include "platform/GAPI/Vulkan/VulkanCheckError.h"
# include "stl/Algorithms/Cast.h"
# include "stl/Algorithms/StringUtils.h"


# if defined(PLATFORM_WINDOWS) or defined(VK_USE_PLATFORM_WIN32_KHR)

#	include "stl/Platforms/WindowsHeader.h"

	using SharedLib_t	= HMODULE;

# else
//#elif defined(PLATFORM_LINUX) or defined(PLATFORM_ANDROID)
#	include <dlfcn.h>
#   include <linux/limits.h>

	using SharedLib_t	= void*;
# endif


namespace AE::Vulkan
{

#	define VKLOADER_STAGE_FNPOINTER
#	 include "vulkan_loader/fn_vulkan_lib.h"
#	 include "vulkan_loader/fn_vulkan_inst.h"
#	undef  VKLOADER_STAGE_FNPOINTER

#	define VKLOADER_STAGE_DUMMYFN
#	 include "vulkan_loader/fn_vulkan_lib.h"
#	 include "vulkan_loader/fn_vulkan_inst.h"
#	 include "vulkan_loader/fn_vulkan_dev.h"
#	undef  VKLOADER_STAGE_DUMMYFN

/*
=================================================
	VulkanLib
=================================================
*/
namespace {
	struct VulkanLib
	{
		SharedLib_t		module;
		VkInstance		instance	= VK_NULL_HANDLE;
		int				refCounter	= 0;


		ND_ static VulkanLib&  Instance ()
		{
			static VulkanLib	lib;
			return lib;
		}
	};
}
/*
=================================================
	Initialize
----
	must be externally synchronized!
=================================================
*/
	bool VulkanLoader::Initialize (NtStringView libName)
	{
		VulkanLib&	lib = VulkanLib::Instance();
		
		if ( lib.module and lib.refCounter > 0 )
		{
			++lib.refCounter;
			return true;
		}

#	ifdef PLATFORM_WINDOWS
		if ( not libName.empty() )
			lib.module = ::LoadLibraryA( libName.c_str() );

		if ( lib.module == null )
			lib.module = ::LoadLibraryA( "vulkan-1.dll" );
		
		if ( lib.module == null )
			return false;
		
		// write library path to log
		{
			char	buf[MAX_PATH] = "";
			CHECK( ::GetModuleFileNameA( lib.module, buf, DWORD(CountOf(buf)) ) != FALSE );

			AE_LOGI( "Vulkan library path: \""s << buf << '"' );
		}

		const auto	Load =	[module = lib.module] (OUT auto& outResult, const char *procName, auto dummy)
							{
								using FN = decltype(dummy);
								FN	result = BitCast<FN>( ::GetProcAddress( module, procName ));
								outResult = result ? result : dummy;
							};

#	else
		if ( not libName.empty() )
			lib.module = ::dlopen( libName.c_str(), RTLD_NOW | RTLD_LOCAL );

		if ( lib.module == null )
			lib.module = ::dlopen( "libvulkan.so", RTLD_NOW | RTLD_LOCAL );
		
		if ( lib.module == null )
			lib.module = ::dlopen( "libvulkan.so.1", RTLD_NOW | RTLD_LOCAL );
		
		if ( lib.module == null )
			return false;
		
		// write library path to log
#		ifndef PLATFORM_ANDROID
		{
			char	buf[PATH_MAX] = "";
			CHECK( dlinfo( lib.module, RTLD_DI_ORIGIN, buf ) == 0 );
			
			AE_LOGI( "Vulkan library path: \""s << buf << '"' );
		}
#		endif

		const auto	Load =	[module = lib.module] (OUT auto& outResult, const char *procName, auto dummy)
							{
								using FN = decltype(dummy);
								FN	result = BitCast<FN>( ::dlsym( module, procName ));
								outResult = result ? result : dummy;
							};
#	endif

		++lib.refCounter;
		

#		define VKLOADER_STAGE_GETADDRESS
#		 include "vulkan_loader/fn_vulkan_lib.h"
#		undef  VKLOADER_STAGE_GETADDRESS

		ASSERT( _var_vkCreateInstance != &Dummy_vkCreateInstance );
		ASSERT( _var_vkGetInstanceProcAddr != &Dummy_vkGetInstanceProcAddr );

		return true;
	}
	
/*
=================================================
	LoadInstance
----
	must be externally synchronized!
=================================================
*/
	void VulkanLoader::LoadInstance (VkInstance instance)
	{
		VulkanLib&	lib = VulkanLib::Instance();

		ASSERT( instance );
		ASSERT( lib.instance == null or lib.instance == instance );
		ASSERT( _var_vkGetInstanceProcAddr != &Dummy_vkGetInstanceProcAddr );

		if ( lib.instance == instance )
			return;

		lib.instance = instance;

		const auto	Load =	[instance] (OUT auto& outResult, const char *procName, auto dummy)
							{
								using FN = decltype(dummy);
								FN	result = BitCast<FN>( vkGetInstanceProcAddr( instance, procName ));
								outResult = result ? result : dummy;
							};
		
#		define VKLOADER_STAGE_GETADDRESS
#		 include "vulkan_loader/fn_vulkan_inst.h"
#		undef  VKLOADER_STAGE_GETADDRESS
	}
	
/*
=================================================
	LoadDevice
----
	access to the 'vkGetDeviceProcAddr' must be externally synchronized!
=================================================
*/
	void VulkanLoader::LoadDevice (VkDevice device, OUT VulkanDeviceFnTable &table)
	{
		ASSERT( _var_vkGetDeviceProcAddr != &Dummy_vkGetDeviceProcAddr );

		const auto	Load =	[device] (OUT auto& outResult, const char *procName, auto dummy)
							{
								using FN = decltype(dummy);
								FN	result = BitCast<FN>( vkGetDeviceProcAddr( device, procName ));
								outResult = result ? result : dummy;
							};

#		define VKLOADER_STAGE_GETADDRESS
#		 include "vulkan_loader/fn_vulkan_dev.h"
#		undef  VKLOADER_STAGE_GETADDRESS
	}
	
/*
=================================================
	ResetDevice
=================================================
*/
	void VulkanLoader::ResetDevice (OUT VulkanDeviceFnTable &table)
	{
		const auto	Load =	[] (OUT auto& outResult, const char *, auto dummy) {
								outResult = dummy;
							};

#		define VKLOADER_STAGE_GETADDRESS
#		 include "vulkan_loader/fn_vulkan_dev.h"
#		undef  VKLOADER_STAGE_GETADDRESS
	}

/*
=================================================
	Unload
----
	must be externally synchronized!
=================================================
*/
	void VulkanLoader::Unload ()
	{
		VulkanLib&	lib = VulkanLib::Instance();
		
		ASSERT( lib.refCounter > 0 );

		if ( (--lib.refCounter) != 0 )
			return;
		
#	ifdef PLATFORM_WINDOWS
		if ( lib.module != null )
		{
			::FreeLibrary( lib.module );
		}
#	else
		if ( lib.module != null )
		{
			::dlclose( lib.module );
		}
#	endif

		lib.instance	= null;
		lib.module		= null;
		
		const auto	Load =	[] (OUT auto& outResult, const char *, auto dummy) {
								outResult = dummy;
							};

#		define VKLOADER_STAGE_GETADDRESS
#		 include "vulkan_loader/fn_vulkan_lib.h"
#		 include "vulkan_loader/fn_vulkan_inst.h"
#		undef  VKLOADER_STAGE_GETADDRESS
	}
	
/*
=================================================
	VulkanDeviceFn_Init
=================================================
*/
	void VulkanDeviceFn::VulkanDeviceFn_Init (const VulkanDeviceFn &other)
	{
		_table = other._table;
	}
	
/*
=================================================
	VulkanDeviceFn_Init
=================================================
*/
	void VulkanDeviceFn::VulkanDeviceFn_Init (VulkanDeviceFnTable *table)
	{
		_table = table;
	}

}	// AE::Vulkan

#endif	// AE_ENABLE_VULKAN
