// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

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
	warning: multiple instances are not supported!
=================================================
*/
	bool VulkanLoader::LoadInstance (VkInstance instance)
	{
		VulkanLib&	lib = VulkanLib::Instance();

		ASSERT( instance );
		ASSERT( lib.instance == null or lib.instance == instance );
		ASSERT( _var_vkGetInstanceProcAddr != &Dummy_vkGetInstanceProcAddr );

		if ( lib.instance == instance )
			return true;	// functions already loaded for this instance

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

		return true;
	}
	
/*
=================================================
	LoadDevice
----
	access to the 'vkGetDeviceProcAddr' must be externally synchronized!
=================================================
*/
	bool VulkanLoader::LoadDevice (VkDevice device, OUT VulkanDeviceFnTable &table)
	{
		CHECK_ERR( _var_vkGetDeviceProcAddr != &Dummy_vkGetDeviceProcAddr );

		const auto	Load =	[device] (OUT auto& outResult, const char *procName, auto dummy)
							{
								using FN = decltype(dummy);
								FN	result = BitCast<FN>( vkGetDeviceProcAddr( device, procName ));
								outResult = result ? result : dummy;
							};

#		define VKLOADER_STAGE_GETADDRESS
#		 include "vulkan_loader/fn_vulkan_dev.h"
#		undef  VKLOADER_STAGE_GETADDRESS

		return true;
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
	SetupInstanceBackwardCompatibility
=================================================
*/
	void VulkanLoader::SetupInstanceBackwardCompatibility (uint version)
	{
		Unused( version );
		
	#define VK_COMPAT( _dst_, _src_ ) \
		ASSERT( _var_##_src_ != null ); \
		/*ASSERT( _var_##_dst_ == null );*/ \
		_var_##_dst_ = _var_##_src_

	#ifdef VK_VERSION_1_1
		if ( VK_VERSION_MAJOR(version) > 1 or (VK_VERSION_MAJOR(version) == 1 and VK_VERSION_MINOR(version) >= 1) )
		{
		// VK_KHR_get_physical_device_properties2
			VK_COMPAT( vkGetPhysicalDeviceFeatures2KHR,						vkGetPhysicalDeviceFeatures2 );
			VK_COMPAT( vkGetPhysicalDeviceProperties2KHR,					vkGetPhysicalDeviceProperties2 );
			VK_COMPAT( vkGetPhysicalDeviceFormatProperties2KHR,				vkGetPhysicalDeviceFormatProperties2 );
			VK_COMPAT( vkGetPhysicalDeviceImageFormatProperties2KHR,		vkGetPhysicalDeviceImageFormatProperties2 );
			VK_COMPAT( vkGetPhysicalDeviceQueueFamilyProperties2KHR,		vkGetPhysicalDeviceQueueFamilyProperties2 );
			VK_COMPAT( vkGetPhysicalDeviceMemoryProperties2KHR,				vkGetPhysicalDeviceMemoryProperties2 );
			VK_COMPAT( vkGetPhysicalDeviceSparseImageFormatProperties2KHR,	vkGetPhysicalDeviceSparseImageFormatProperties2 );
		}
	#endif
	#ifdef VK_VERSION_1_2
		if ( VK_VERSION_MAJOR(version) > 1 or (VK_VERSION_MAJOR(version) == 1 and VK_VERSION_MINOR(version) >= 2) )
		{
		}
	#endif

	#undef VK_COMPAT
	}
	
/*
=================================================
	SetupDeviceBackwardCompatibility
=================================================
*/
	void VulkanLoader::SetupDeviceBackwardCompatibility (uint version, INOUT VulkanDeviceFnTable &table)
	{
		Unused( version, table );
		
	#define VK_COMPAT( _dst_, _src_ ) \
		ASSERT( table._var_##_src_ != null ); \
		/*ASSERT( table._var_##_dst_ == null );*/ \
		table._var_##_dst_ = table._var_##_src_

	#ifdef VK_VERSION_1_1
		if ( VK_VERSION_MAJOR(version) > 1 or (VK_VERSION_MAJOR(version) == 1 and VK_VERSION_MINOR(version) >= 1) )
		{
		// VK_KHR_maintenance1
			VK_COMPAT( vkTrimCommandPoolKHR, vkTrimCommandPool );

		// VK_KHR_bind_memory2
			VK_COMPAT( vkBindBufferMemory2KHR,	vkBindBufferMemory2 );
			VK_COMPAT( vkBindImageMemory2KHR,	vkBindImageMemory2 );

		// VK_KHR_get_memory_requirements2
			VK_COMPAT( vkGetImageMemoryRequirements2KHR,		vkGetImageMemoryRequirements2 );
			VK_COMPAT( vkGetBufferMemoryRequirements2KHR,		vkGetBufferMemoryRequirements2 );
			VK_COMPAT( vkGetImageSparseMemoryRequirements2KHR,	vkGetImageSparseMemoryRequirements2 );

		// VK_KHR_sampler_ycbcr_conversion
			VK_COMPAT( vkCreateSamplerYcbcrConversionKHR,	vkCreateSamplerYcbcrConversion );
			VK_COMPAT( vkDestroySamplerYcbcrConversionKHR,	vkDestroySamplerYcbcrConversion );

		// VK_KHR_descriptor_update_template
			VK_COMPAT( vkCreateDescriptorUpdateTemplateKHR,		vkCreateDescriptorUpdateTemplate );
			VK_COMPAT( vkDestroyDescriptorUpdateTemplateKHR,	vkDestroyDescriptorUpdateTemplate );
			VK_COMPAT( vkUpdateDescriptorSetWithTemplateKHR,	vkUpdateDescriptorSetWithTemplate );
			
		// VK_KHR_device_group
			VK_COMPAT( vkCmdDispatchBaseKHR, vkCmdDispatchBase );
		}
	#endif
	#ifdef VK_VERSION_1_2
		if ( VK_VERSION_MAJOR(version) > 1 or (VK_VERSION_MAJOR(version) == 1 and VK_VERSION_MINOR(version) >= 2) )
		{
		// VK_KHR_draw_indirect_count
			VK_COMPAT( vkCmdDrawIndirectCountKHR,		 vkCmdDrawIndirectCount );
			VK_COMPAT( vkCmdDrawIndexedIndirectCountKHR, vkCmdDrawIndexedIndirectCountKHR );

		// VK_KHR_create_renderpass2
			VK_COMPAT( vkCreateRenderPass2KHR,		vkCreateRenderPass2 );
			VK_COMPAT( vkCmdBeginRenderPass2KHR,	vkCmdBeginRenderPass2 );
			VK_COMPAT( vkCmdNextSubpass2KHR,		vkCmdNextSubpass2 );
			VK_COMPAT( vkCmdEndRenderPass2KHR,		vkCmdEndRenderPass2 );

		// VK_KHR_timeline_semaphore
			VK_COMPAT( vkGetSemaphoreCounterValueKHR,	vkGetSemaphoreCounterValue );
			VK_COMPAT( vkWaitSemaphoresKHR,				vkWaitSemaphores );
			VK_COMPAT( vkSignalSemaphoreKHR,			vkSignalSemaphore );

		// VK_KHR_buffer_device_address
			VK_COMPAT( vkGetBufferDeviceAddressKHR,					vkGetBufferDeviceAddress );
			VK_COMPAT( vkGetBufferOpaqueCaptureAddressKHR,			vkGetBufferOpaqueCaptureAddress );
			VK_COMPAT( vkGetDeviceMemoryOpaqueCaptureAddressKHR,	vkGetDeviceMemoryOpaqueCaptureAddress );
		}
	#endif
		
	#undef VK_COMPAT
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
	
	void VulkanDeviceFn::VulkanDeviceFn_Init (VulkanDeviceFnTable *table)
	{
		_table = table;
	}

}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
