// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#ifdef AE_ENABLE_VULKAN

# include "graphics/Vulkan/VulkanLoader.h"
# include "graphics/Vulkan/VulkanCheckError.h"
# include "stl/Algorithms/Cast.h"
# include "stl/Algorithms/StringUtils.h"
# include "stl/Platforms/PlatformUtils.h"


namespace AE::Graphics
{

#  if 1
#	define VK_LOG( _msg_ )	static struct VkLogCallOnce { VkLogCallOnce() { AE_LOGD( _msg_ ); } } log
#  else
#	define VK_LOG( _msg_ )	AE_LOGD( _msg_ )
#  endif

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
		Library			module;
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
		
		if ( not libName.empty() )
			lib.module.Load( libName );

#	ifdef PLATFORM_WINDOWS
		if ( not lib.module )
			lib.module.Load( NtStringView{ "vulkan-1.dll" });
#	else
		if ( not lib.module )
			lib.module.Load( NtStringView{ "libvulkan.so" });
		
		if ( not lib.module )
			lib.module.Load( NtStringView{ "libvulkan.so.1" });
#	endif
		
		if ( not lib.module  )
			return false;
		
		// write library path to log
		AE_LOGI( "Vulkan library path: \""s << lib.module.GetPath().string() << '"' );

		const auto	Load =	[module = &lib.module] (OUT auto& outResult, const char *procName, auto dummy)
							{
								module->GetProcAddr( procName, OUT outResult );
								outResult = outResult ? outResult : dummy;
							};

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
		
		lib.module.Unload();
		lib.instance = null;
		
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

}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
