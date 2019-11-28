// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#ifdef AE_ENABLE_VULKAN

# include "platform/GAPI/Vulkan/VDevice.h"
# include "stl/Algorithms/StringUtils.h"

namespace AE::Vulkan
{
	
	bool  VDevice::InstanceVersion::operator == (const InstanceVersion &rhs) const
	{
		return major == rhs.major and minor == rhs.minor;
	}

	bool  VDevice::InstanceVersion::operator >  (const InstanceVersion &rhs) const
	{
		return	major != rhs.major ? major > rhs.major :
									 minor > rhs.minor;
	}

	bool  VDevice::InstanceVersion::operator >= (const InstanceVersion &rhs) const
	{
		return not (rhs > *this);
	}
//-----------------------------------------------------------------------------

	
/*
=================================================
	constructor
=================================================
*/
	VDevice::VDevice () :
		_vkLogicalDevice{ VK_NULL_HANDLE },
		_vkPhysicalDevice{ VK_NULL_HANDLE },
		_vkInstance{ VK_NULL_HANDLE }
	{
		std::memset( &_supported, 0, sizeof(_supported) );

		VulkanDeviceFn_Init( &_deviceFnTable );
	}

/*
=================================================
	destructor
=================================================
*/
	VDevice::~VDevice ()
	{
		CHECK( not _vkInstance );
		CHECK( not _vkLogicalDevice );
	}

/*
=================================================
	HasExtension
=================================================
*/
	bool VDevice::HasExtension (StringView name) const
	{
		return !!_instanceExtensions.count( name );
	}

/*
=================================================
	HasDeviceExtension
=================================================
*/
	bool VDevice::HasDeviceExtension (StringView name) const
	{
		return !!_deviceExtensions.count( name );
	}

/*
=================================================
	SetObjectName
=================================================
*/
	bool VDevice::SetObjectName (uint64_t id, NtStringView name, VkObjectType type) const
	{
		if ( name.empty() or id == VK_NULL_HANDLE )
			return false;

		if ( _supported.debugUtils )
		{
			VkDebugUtilsObjectNameInfoEXT	info = {};
			info.sType			= VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
			info.objectType		= type;
			info.objectHandle	= id;
			info.pObjectName	= name.c_str();

			VK_CALL( vkSetDebugUtilsObjectNameEXT( _vkLogicalDevice, &info ));
			return true;
		}

		return false;
	}
//-----------------------------------------------------------------------------


/*
=================================================
	GetInstanceVersion
=================================================
*/
	VDeviceInitializer::InstanceVersion  VDeviceInitializer::GetInstanceVersion () const
	{
		uint	ver = VK_MAKE_VERSION( 1, 0, 0 );
		Unused( vkEnumerateInstanceVersion( OUT &ver ));

		return InstanceVersion{ VK_VERSION_MAJOR(ver), VK_VERSION_MINOR(ver) };
	}
	
/*
=================================================
	CreateInstance
=================================================
*/
	bool VDeviceInitializer::CreateInstance (NtStringView appName, NtStringView engineName, ArrayView<const char*> layers,
											 ArrayView<const char*> extensions, InstanceVersion version, uint appVer, uint engineVer)
	{
		CHECK_ERR( not _vkInstance );
		CHECK_ERR( VulkanLoader::Initialize() );
		
		Array< const char* >	instance_layers;
		instance_layers.assign( layers.begin(), layers.end() );

		Array< const char* >	instance_extensions;
		instance_extensions.assign( extensions.begin(), extensions.end() );

		uint	vk_ver = VK_MAKE_VERSION( version.major, version.minor, 0 );

		_ValidateInstanceVersion( INOUT vk_ver );
		_ValidateInstanceLayers( INOUT instance_layers );
		_ValidateInstanceExtensions( INOUT instance_extensions );
		
		VkApplicationInfo		app_info = {};
		app_info.sType				= VK_STRUCTURE_TYPE_APPLICATION_INFO;
		app_info.apiVersion			= vk_ver;
		app_info.pApplicationName	= appName.c_str();
		app_info.applicationVersion	= appVer;
		app_info.pEngineName		= engineName.c_str();
		app_info.engineVersion		= engineVer;

		VkInstanceCreateInfo			instance_create_info = {};
		instance_create_info.sType						= VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instance_create_info.pApplicationInfo			= &app_info;
		instance_create_info.enabledExtensionCount		= uint(instance_extensions.size());
		instance_create_info.ppEnabledExtensionNames	= instance_extensions.data();
		instance_create_info.enabledLayerCount			= uint(instance_layers.size());
		instance_create_info.ppEnabledLayerNames		= instance_layers.data();

		VK_CHECK( vkCreateInstance( &instance_create_info, null, OUT &_vkInstance ));

		VulkanLoader::LoadInstance( _vkInstance );

		for (auto inst : instance_extensions) {
			_instanceExtensions.insert( inst );
		}

		_vkVersion = { VK_VERSION_MAJOR(vk_ver), VK_VERSION_MINOR(vk_ver) };
		return true;
	}
	
/*
=================================================
	SetInstance
=================================================
*/
	bool VDeviceInitializer::SetInstance (VkInstance newInstance, ArrayView<const char*> instanceExtensions)
	{
		CHECK_ERR( not _vkInstance );
		CHECK_ERR( newInstance );
		CHECK_ERR( VulkanLoader::Initialize() );

		_vkInstance = newInstance;

		VulkanLoader::LoadInstance( _vkInstance );
		
		for (auto inst : instanceExtensions) {
			_instanceExtensions.insert( inst );
		}

		// TODO: set version
		return true;
	}
	
/*
=================================================
	DestroyInstance
=================================================
*/
	bool VDeviceInitializer::DestroyInstance ()
	{
		if ( not _vkInstance )
			return false;
		
		vkDestroyInstance( _vkInstance, null );
		VulkanLoader::Unload();
		
		_vkInstance			= VK_NULL_HANDLE;
		_vkPhysicalDevice	= VK_NULL_HANDLE;
		
		AE_LOGD( "Destroyed Vulkan instance" );
		return true;
	}

/*
=================================================
	WithPhysicalDevices
=================================================
*/
namespace {
	template <typename Fn>
	void WithPhysicalDevices (const VDevice &vulkan, Fn &&fn)
	{
		uint						count	= 0;
		Array< VkPhysicalDevice >	devices;
		
		VK_CALL( vkEnumeratePhysicalDevices( vulkan.GetVkInstance(), OUT &count, null ));
		CHECK_ERR( count > 0, void());

		devices.resize( count );
		VK_CALL( vkEnumeratePhysicalDevices( vulkan.GetVkInstance(), OUT &count, OUT devices.data() ));

		for (auto& dev : devices)
		{
			VkPhysicalDeviceProperties			prop	 = {};
			VkPhysicalDeviceFeatures			feat	 = {};
			VkPhysicalDeviceMemoryProperties	mem_prop = {};
			
			vkGetPhysicalDeviceProperties( dev, OUT &prop );
			vkGetPhysicalDeviceFeatures( dev, OUT &feat );
			vkGetPhysicalDeviceMemoryProperties( dev, OUT &mem_prop );

			AE_LOGI( "Found Vulkan device: "s << prop.deviceName );

			fn( dev, prop, feat, mem_prop );
		}
	}
}
/*
=================================================
	GetVendorNameByID
----
	from https://www.reddit.com/r/vulkan/comments/4ta9nj/is_there_a_comprehensive_list_of_the_names_and/
=================================================
*/
namespace {
	ND_ StringView  GetVendorNameByID (uint id)
	{
		switch ( id )
		{
			case 0x1002 :	return "AMD";
			case 0x1010 :	return "ImgTec";
			case 0x10DE :	return "NVIDIA";
			case 0x13B5 :	return "ARM";
			case 0x5143 :	return "Qualcomm";
			case 0x8086 :	return "INTEL";
		}
		return "unknown";
	}
}
/*
=================================================
	CalcTotalMemory
=================================================
*/
namespace {
	ND_ BytesU  CalcTotalMemory (VkPhysicalDeviceMemoryProperties memProps)
	{
		BytesU	total;

		for (uint32_t j = 0; j < memProps.memoryTypeCount; ++j)
		{
			if ( EnumEq( memProps.memoryTypes[j].propertyFlags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT ) )
			{
				const uint32_t idx = memProps.memoryTypes[j].heapIndex;

				if ( EnumEq( memProps.memoryHeaps[idx].flags, VK_MEMORY_HEAP_DEVICE_LOCAL_BIT ) )
				{
					total += BytesU( memProps.memoryHeaps[idx].size );
					memProps.memoryHeaps[idx].size = 0;
				}
			}
		}
		return total;
	}
}
/*
=================================================
	ChooseDevice
=================================================
*/
	bool VDeviceInitializer::ChooseDevice (StringView deviceName)
	{
		CHECK_ERR( _vkInstance );
		CHECK_ERR( not _vkLogicalDevice );

		VkPhysicalDevice	pdev = VK_NULL_HANDLE;

		WithPhysicalDevices( *this,
			[deviceName, &pdev] (VkPhysicalDevice dev, auto& prop, auto&...)
			{
				bool	math = not deviceName.empty()							and
							   (HasSubStringIC( prop.deviceName, deviceName )	or
							    HasSubStringIC( GetVendorNameByID( prop.vendorID ), deviceName ));

				if ( math and not pdev )
					pdev = dev;
			});

		_vkPhysicalDevice = pdev;
		CHECK_ERR( _vkPhysicalDevice );

		return true;
	}
	
/*
=================================================
	ChooseHighPerformanceDevice
=================================================
*/
	bool VDeviceInitializer::ChooseHighPerformanceDevice ()
	{
		CHECK_ERR( _vkInstance );
		CHECK_ERR( not _vkLogicalDevice );
		
		VkPhysicalDevice	any_device			= VK_NULL_HANDLE;
		VkPhysicalDevice	high_perf_device	= VK_NULL_HANDLE;
		float				max_performance		= 0.0f;

		WithPhysicalDevices( *this,
			[&any_device, &high_perf_device, &max_performance] (VkPhysicalDevice dev, auto& prop, auto& feat, auto&)
			{
				const bool	is_gpu		  = (prop.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU or
											 prop.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU);
				const bool	is_integrated = prop.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;

				//const BytesU global_mem   = CalcTotalMemory( mem );
																										// magic function:
				const float	perf	=	//float(global_mem.Mb()) / 1024.0f +									// commented because of bug on Intel (incorrect heap size)
										float(prop.limits.maxComputeSharedMemorySize >> 10) / 64.0f +		// big local cache is good
										float(is_gpu and not is_integrated ? 4 : 1) +						// 
										float(prop.limits.maxComputeWorkGroupInvocations) / 1024.0f +		// 
										float(feat.tessellationShader + feat.geometryShader);
				
				if ( perf > max_performance ) {
					max_performance		= perf;
					high_perf_device	= dev;
				}

				if ( not any_device )
					any_device = dev;
			});

		_vkPhysicalDevice = (high_perf_device ? high_perf_device : any_device);
		CHECK_ERR( _vkPhysicalDevice );

		return true;
	}
	
/*
=================================================
	SetPhysicalDevice
=================================================
*/
	bool VDeviceInitializer::SetPhysicalDevice (VkPhysicalDevice newPhysicalDevice)
	{
		CHECK_ERR( _vkInstance );
		CHECK_ERR( not _vkLogicalDevice );
		CHECK_ERR( newPhysicalDevice );

		_vkPhysicalDevice = newPhysicalDevice;
		return true;
	}
	
/*
=================================================
	DeviceFeatures
=================================================
*/
namespace {
	struct DeviceFeatures
	{
		VkPhysicalDeviceFeatures								main;
		//VkDeviceGeneratedCommandsFeaturesNVX					generatedCommands;
		VkPhysicalDeviceMultiviewFeatures						multiview;
		//VkPhysicalDeviceShaderAtomicInt64FeaturesKHR			shaderAtomicI64;
		VkPhysicalDevice8BitStorageFeaturesKHR					storage8Bit;
		VkPhysicalDevice16BitStorageFeatures					storage16Bit;
		VkPhysicalDeviceSamplerYcbcrConversionFeatures			samplerYcbcrConversion;
		VkPhysicalDeviceBlendOperationAdvancedFeaturesEXT		blendOpAdvanced;
		VkPhysicalDeviceConditionalRenderingFeaturesEXT			conditionalRendering;
		VkPhysicalDeviceShaderDrawParameterFeatures				shaderDrawParameters;
		VkPhysicalDeviceMeshShaderFeaturesNV					meshShader;
		VkPhysicalDeviceDescriptorIndexingFeaturesEXT			descriptorIndexing;
		//VkPhysicalDeviceVertexAttributeDivisorFeaturesEXT		vertexAttribDivisor;
		//VkPhysicalDeviceASTCDecodeFeaturesEXT					astcDecode;
		VkPhysicalDeviceVulkanMemoryModelFeaturesKHR			memoryModel;
		VkPhysicalDeviceInlineUniformBlockFeaturesEXT			inlineUniformBlock;
		VkPhysicalDeviceRepresentativeFragmentTestFeaturesNV	representativeFragmentTest;
		//VkPhysicalDeviceExclusiveScissorFeaturesNV			exclusiveScissorTest;
		//VkPhysicalDeviceCornerSampledImageFeaturesNV			cornerSampledImage;
		//VkPhysicalDeviceComputeShaderDerivativesFeaturesNV	computeShaderDerivatives;
		VkPhysicalDeviceFragmentShaderBarycentricFeaturesNV		fragmentShaderBarycentric;
		VkPhysicalDeviceShaderImageFootprintFeaturesNV			shaderImageFootprint;
		VkPhysicalDeviceShadingRateImageFeaturesNV				shadingRateImage;

		DeviceFeatures () {
			std::memset( this, 0, sizeof(*this) );
		}
	};
}
/*
=================================================
	SetupDeviceFeatures
=================================================
*/
namespace {
	bool SetupDeviceFeatures (VkPhysicalDevice pdev, DeviceFeatures &features, void** nextExt, ArrayView<const char*> extensions)
	{
		VkPhysicalDeviceFeatures2	feat2		= {};
		void **						next_feat	= &feat2.pNext;;
		feat2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		
		for (StringView ext : extensions)
		{
			if ( ext == VK_NV_MESH_SHADER_EXTENSION_NAME )
			{
				*next_feat	= *nextExt		= &features.meshShader;
				next_feat	= nextExt		= &features.meshShader.pNext;
				features.meshShader.sType	= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_NV;
			}
			else
			if ( ext == VK_KHR_MULTIVIEW_EXTENSION_NAME )
			{
				*next_feat	= *nextExt		= &features.multiview;
				next_feat	= nextExt		= &features.multiview.pNext;
				features.multiview.sType	= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES;
			}
			else
			if ( ext == VK_KHR_8BIT_STORAGE_EXTENSION_NAME )
			{
				*next_feat	= *nextExt		= &features.storage8Bit;
				next_feat	= nextExt		= &features.storage8Bit.pNext;
				features.storage8Bit.sType	= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_8BIT_STORAGE_FEATURES_KHR;
			}
			else
			if ( ext == VK_KHR_16BIT_STORAGE_EXTENSION_NAME )
			{
				*next_feat	= *nextExt		= &features.storage16Bit;
				next_feat	= nextExt		= &features.storage16Bit.pNext;
				features.storage16Bit.sType	= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES;
			}
			else
			if ( ext == VK_KHR_SAMPLER_YCBCR_CONVERSION_EXTENSION_NAME )
			{
				*next_feat	= *nextExt					= &features.samplerYcbcrConversion;
				next_feat	= nextExt					= &features.samplerYcbcrConversion.pNext;
				features.samplerYcbcrConversion.sType	= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SAMPLER_YCBCR_CONVERSION_FEATURES;
			}
			else
			if ( ext == VK_EXT_BLEND_OPERATION_ADVANCED_EXTENSION_NAME )
			{
				*next_feat	= *nextExt			= &features.blendOpAdvanced;
				next_feat	= nextExt			= &features.blendOpAdvanced.pNext;
				features.blendOpAdvanced.sType	= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BLEND_OPERATION_ADVANCED_FEATURES_EXT;
			}
			else
			if ( ext == VK_EXT_CONDITIONAL_RENDERING_EXTENSION_NAME )
			{
				*next_feat	= *nextExt				= &features.conditionalRendering;
				next_feat	= nextExt				= &features.conditionalRendering.pNext;
				features.conditionalRendering.sType	= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CONDITIONAL_RENDERING_FEATURES_EXT;
			}
			else
			if ( ext == VK_KHR_VULKAN_MEMORY_MODEL_EXTENSION_NAME )
			{
				*next_feat	= *nextExt		= &features.memoryModel;
				next_feat	= nextExt		= &features.memoryModel.pNext;
				features.memoryModel.sType	= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_MEMORY_MODEL_FEATURES_KHR;
			}
			else
			if ( ext == VK_EXT_INLINE_UNIFORM_BLOCK_EXTENSION_NAME )
			{
				*next_feat	= *nextExt				= &features.inlineUniformBlock;
				next_feat	= nextExt				= &features.inlineUniformBlock.pNext;
				features.inlineUniformBlock.sType	= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INLINE_UNIFORM_BLOCK_FEATURES_EXT;
			}
			else
			if ( ext == VK_NV_REPRESENTATIVE_FRAGMENT_TEST_EXTENSION_NAME )
			{
				*next_feat	= *nextExt						= &features.representativeFragmentTest;
				next_feat	= nextExt						= &features.representativeFragmentTest.pNext;
				features.representativeFragmentTest.sType	= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_REPRESENTATIVE_FRAGMENT_TEST_FEATURES_NV;
			}
			else
			if ( ext == VK_NV_FRAGMENT_SHADER_BARYCENTRIC_EXTENSION_NAME )
			{
				*next_feat	= *nextExt						= &features.fragmentShaderBarycentric;
				next_feat	= nextExt						= &features.fragmentShaderBarycentric.pNext;
				features.fragmentShaderBarycentric.sType	= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_BARYCENTRIC_FEATURES_NV;
			}
			else
			if ( ext == VK_NV_SHADER_IMAGE_FOOTPRINT_EXTENSION_NAME )
			{
				*next_feat	= *nextExt				= &features.shaderImageFootprint;
				next_feat	= nextExt				= &features.shaderImageFootprint.pNext;
				features.shaderImageFootprint.sType	= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_IMAGE_FOOTPRINT_FEATURES_NV;
			}
			else
			if ( ext == VK_NV_SHADING_RATE_IMAGE_EXTENSION_NAME )
			{
				*next_feat	= *nextExt			= &features.shadingRateImage;
				next_feat	= nextExt			= &features.shadingRateImage.pNext;
				features.shadingRateImage.sType	= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADING_RATE_IMAGE_FEATURES_NV;
			}
			else
			if ( ext == VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME )
			{
				*next_feat	= *nextExt				= &features.descriptorIndexing;
				next_feat	= nextExt				= &features.descriptorIndexing.pNext;
				features.descriptorIndexing.sType	= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;
			}
		}
		
		*next_feat	= *nextExt				= &features.shaderDrawParameters;
		next_feat	= nextExt				= &features.shaderDrawParameters.pNext;
		features.shaderDrawParameters.sType	= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETER_FEATURES;

		vkGetPhysicalDeviceFeatures2( pdev, &feat2 );
		return true;
	}
}
/*
=================================================
	CreateLogicalDevice
=================================================
*/
	bool VDeviceInitializer::CreateLogicalDevice (ArrayView<QueueCreateInfo> queues, ArrayView<const char*> extensions)
	{
		CHECK_ERR( _vkPhysicalDevice );
		CHECK_ERR( not _vkLogicalDevice );

		CHECK_ERR( _SetupQueues( queues ));
		
		// setup device create info
		VkDeviceCreateInfo		device_info	= {};
		void **					next_ext	= const_cast<void **>( &device_info.pNext );
		device_info.sType		= VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;


		// setup extensions
		Array<const char *>		device_extensions;
		device_extensions.assign( extensions.begin(), extensions.end() );

		_ValidateDeviceExtensions( INOUT device_extensions );

		if ( not device_extensions.empty() )
		{
			device_info.enabledExtensionCount	= uint(device_extensions.size());
			device_info.ppEnabledExtensionNames	= device_extensions.data();
		}


		// setup queues
		Array< VkDeviceQueueCreateInfo >	queue_infos;
		Array< FixedArray<float,16> >		priorities;
		{
			uint	max_queue_families = 0;
			vkGetPhysicalDeviceQueueFamilyProperties( _vkPhysicalDevice, OUT &max_queue_families, null );

			queue_infos.resize( max_queue_families );
			priorities.resize( max_queue_families );

			for (size_t i = 0; i < queue_infos.size(); ++i)
			{
				auto&	ci = queue_infos[i];

				ci.sType			= VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				ci.pNext			= null;
				ci.queueFamilyIndex	= uint(i);
				ci.queueCount		= 0;
				ci.pQueuePriorities	= priorities[i].data();
			}

			for (auto& q : _vkQueues)
			{
				q.queueIndex = (queue_infos[ uint(q.familyIndex) ].queueCount++);
				priorities[ uint(q.familyIndex) ].push_back( q.priority );
			}

			// remove unused queues
			for (auto iter = queue_infos.begin(); iter != queue_infos.end();)
			{
				if ( iter->queueCount == 0 )
					iter = queue_infos.erase( iter );
				else
					++iter;
			}
		}
		device_info.queueCreateInfoCount	= uint(queue_infos.size());
		device_info.pQueueCreateInfos		= queue_infos.data();


		// setup features
		DeviceFeatures	features;
		vkGetPhysicalDeviceFeatures( _vkPhysicalDevice, OUT &features.main );
		device_info.pEnabledFeatures = &features.main;

		if ( _vkVersion >= InstanceVersion{1,1} )
		{
			CHECK_ERR( SetupDeviceFeatures( _vkPhysicalDevice, features, INOUT next_ext, device_extensions ));
		}

		VK_CHECK( vkCreateDevice( _vkPhysicalDevice, &device_info, null, OUT &_vkLogicalDevice ));
		

		VulkanLoader::LoadDevice( _vkLogicalDevice, OUT _deviceFnTable );

		for (auto& q : _vkQueues) {
			vkGetDeviceQueue( _vkLogicalDevice, uint(q.familyIndex), q.queueIndex, OUT &q.handle );
		}
		
		for (auto ext : extensions) {
			_deviceExtensions.insert( ext );
		}
		
		CHECK_ERR( _InitDevice() );
		return true;
	}

/*
=================================================
	SetLogicalDevice
=================================================
*/
	bool VDeviceInitializer::SetLogicalDevice (VkDevice newDevice, ArrayView<const char*> extensions)
	{
		CHECK_ERR( _vkPhysicalDevice );
		CHECK_ERR( not _vkLogicalDevice );
		CHECK_ERR( not _vkQueues.empty() );
		CHECK_ERR( newDevice );

		_vkLogicalDevice = newDevice;

		VulkanLoader::LoadDevice( _vkLogicalDevice, OUT _deviceFnTable );

		for (auto ext : extensions) {
			_deviceExtensions.insert( ext );
		}

		// TODO: get queues

		CHECK_ERR( _InitDevice() );
		return true;
	}
	
/*
=================================================
	DestroyLogicalDevice
=================================================
*/
	bool VDeviceInitializer::DestroyLogicalDevice ()
	{
		if ( not _vkLogicalDevice )
			return false;
		
		vkDestroyDevice( _vkLogicalDevice, null );
		VulkanLoader::ResetDevice( OUT _deviceFnTable );

		_vkLogicalDevice = VK_NULL_HANDLE;
		_vkQueues.clear();
		
		AE_LOGD( "Destroyed Vulkan logical device" );
		return true;
	}

/*
=================================================
	_InitDevice
=================================================
*/
	bool VDeviceInitializer::_InitDevice ()
	{
		vkGetPhysicalDeviceFeatures( _vkPhysicalDevice, OUT &_deviceInfo.features );
		vkGetPhysicalDeviceProperties( _vkPhysicalDevice, OUT &_deviceInfo.properties );
		vkGetPhysicalDeviceMemoryProperties( _vkPhysicalDevice, OUT &_deviceInfo.memoryProperties );

		// get api version
		{
			_vkVersion.major = VK_VERSION_MAJOR( GetDeviceProperties().apiVersion );
			_vkVersion.minor = VK_VERSION_MINOR( GetDeviceProperties().apiVersion );
		}
		
		_supported.debugUtils			= HasExtension( VK_EXT_DEBUG_UTILS_EXTENSION_NAME );
		_supported.meshShaderNV			= HasDeviceExtension( VK_NV_MESH_SHADER_EXTENSION_NAME );
		_supported.rayTracingNV			= HasDeviceExtension( VK_NV_RAY_TRACING_EXTENSION_NAME );
		_supported.shadingRateImageNV	= HasDeviceExtension( VK_NV_SHADING_RATE_IMAGE_EXTENSION_NAME );
		_supported.samplerMirrorClamp	= HasDeviceExtension( VK_KHR_SAMPLER_MIRROR_CLAMP_TO_EDGE_EXTENSION_NAME );
		_supported.descriptorIndexing	= HasDeviceExtension( VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME );

		AE_LOGI( "Created vulkan device on GPU: "s << _deviceInfo.properties.deviceName
			<< "\n  version:             " << ToString(_vkVersion.major) << '.' << ToString(_vkVersion.minor)
			<< "\n  debug_utils:         " << ToString( _supported.debugUtils )
			<< "\n  mesh_shader:         " << ToString( _supported.meshShaderNV )
			<< "\n  ray_tracing:         " << ToString( _supported.rayTracingNV )
			<< "\n  shading_rate_image:  " << ToString( _supported.shadingRateImageNV )
			<< "\n  descriptor_indexing: " << ToString( _supported.descriptorIndexing )
		);

		// load extensions
		if ( _vkVersion >= InstanceVersion{1,1} )
		{
			VkPhysicalDeviceFeatures2	feat2		= {};
			void **						next_feat	= &feat2.pNext;
			feat2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
			
			if ( _supported.meshShaderNV )
			{
				*next_feat	= &_deviceInfo.meshShaderFeatures;
				next_feat	= &_deviceInfo.meshShaderFeatures.pNext;
				_deviceInfo.meshShaderFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_NV;
			}
			if ( _supported.shadingRateImageNV )
			{
				*next_feat	= &_deviceInfo.shadingRateImageFeatures;
				next_feat	= &_deviceInfo.shadingRateImageFeatures.pNext;
				_deviceInfo.shadingRateImageFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADING_RATE_IMAGE_FEATURES_NV;
			}
			vkGetPhysicalDeviceFeatures2( GetVkPhysicalDevice(), &feat2 );

			_supported.meshShaderNV			&= (_deviceInfo.meshShaderFeatures.meshShader or _deviceInfo.meshShaderFeatures.taskShader);
			_supported.shadingRateImageNV	&= (_deviceInfo.shadingRateImageFeatures.shadingRateImage == VK_TRUE);


			VkPhysicalDeviceProperties2	props2		= {};
			void **						next_props	= &props2.pNext;
			props2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;

			if ( _supported.meshShaderNV )
			{
				*next_props	= &_deviceInfo.meshShaderProperties;
				next_props	= &_deviceInfo.meshShaderProperties.pNext;
				_deviceInfo.meshShaderProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_PROPERTIES_NV;
			}
			if ( _supported.shadingRateImageNV )
			{
				*next_props	= &_deviceInfo.shadingRateImageProperties;
				next_props	= &_deviceInfo.shadingRateImageProperties.pNext;
				_deviceInfo.shadingRateImageProperties.sType	= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADING_RATE_IMAGE_PROPERTIES_NV;
			}
			if ( _supported.rayTracingNV )
			{
				*next_props	= &_deviceInfo.rayTracingProperties;
				next_props	= &_deviceInfo.rayTracingProperties.pNext;
				_deviceInfo.rayTracingProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PROPERTIES_NV;
			}
			vkGetPhysicalDeviceProperties2( GetVkPhysicalDevice(), &props2 );
		}

		return true;
	}

/*
=================================================
	_ChooseQueueIndex
=================================================
*/
	bool VDeviceInitializer::_ChooseQueueIndex (ArrayView<VkQueueFamilyProperties> queueFamilyProps, INOUT VkQueueFlags &requiredFlags, OUT uint &familyIndex) const
	{
		// validate required flags
		{
			// if the capabilities of a queue family include VK_QUEUE_GRAPHICS_BIT or VK_QUEUE_COMPUTE_BIT,
			// then reporting the VK_QUEUE_TRANSFER_BIT capability separately for that queue family is optional.
			if ( requiredFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT) )
				requiredFlags &= ~VK_QUEUE_TRANSFER_BIT;
		}

		Pair<VkQueueFlags, uint>	compatible {0, UMax};

		for (size_t i = 0; i < queueFamilyProps.size(); ++i)
		{
			const auto&		prop			 = queueFamilyProps[i];
			VkQueueFlags	curr_flags		 = prop.queueFlags;

			if ( curr_flags == requiredFlags )
			{
				requiredFlags	= curr_flags;
				familyIndex		= uint(i);
				return true;
			}

			if ( EnumEq( curr_flags, requiredFlags ) and 
				 (compatible.first == 0 or BitSet<32>{compatible.first}.count() > BitSet<32>{curr_flags}.count()) )
			{
				compatible = { curr_flags, uint(i) };
			}
		}

		if ( compatible.first ) {
			requiredFlags	= compatible.first;
			familyIndex		= compatible.second;
			return true;
		}

		RETURN_ERR( "no suitable queue family found!" );
	}

/*
=================================================
	_SetupQueues
=================================================
*/
	bool VDeviceInitializer::_SetupQueues (ArrayView<QueueCreateInfo> queues)
	{
		CHECK_ERR( _vkQueues.empty() );
		
		constexpr uint	MaxQueueFamilies = 32;
		using QueueFamilyProperties_t	= FixedArray< VkQueueFamilyProperties, MaxQueueFamilies >;
		using QueueCount_t				= FixedArray< uint, MaxQueueFamilies >;

		uint	count = 0;
		vkGetPhysicalDeviceQueueFamilyProperties( _vkPhysicalDevice, OUT &count, null );
		CHECK_ERR( count > 0 );
		
		QueueFamilyProperties_t  queue_family_props;
		queue_family_props.resize( Min( count, queue_family_props.capacity() ));
		vkGetPhysicalDeviceQueueFamilyProperties( _vkPhysicalDevice, OUT &count, OUT queue_family_props.data() );


		// setup default queue
		if ( queues.empty() )
		{
			uint			family_index	= 0;
			VkQueueFlags	flags			= VkQueueFlags(VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT);

			CHECK_ERR( _ChooseQueueIndex( queue_family_props, INOUT flags, OUT family_index ));

			VQueue	vq;
			vq.familyIndex	= EQueueFamily(family_index);
			vq.familyFlags	= flags;
			vq.minImageTransferGranularity = { queue_family_props[family_index].minImageTransferGranularity.width,
											   queue_family_props[family_index].minImageTransferGranularity.height,
											   queue_family_props[family_index].minImageTransferGranularity.depth };

			_vkQueues.push_back( std::move(vq) );
			return true;
		}

		QueueCount_t	qcount;	qcount.resize( queue_family_props.size() );

		for (auto& q : queues)
		{
			uint			family_index	= 0;
			VkQueueFlags	flags			= q.flags;
			CHECK_ERR( _ChooseQueueIndex( queue_family_props, INOUT flags, OUT family_index ));

			if ( qcount[family_index]++ < queue_family_props[family_index].queueCount )
			{
				VQueue	vq;
				vq.familyIndex	= EQueueFamily(family_index);
				vq.familyFlags	= flags;
				vq.priority		= q.priority;
				vq.debugName	= q.debugName;
				vq.minImageTransferGranularity = { queue_family_props[family_index].minImageTransferGranularity.width,
												   queue_family_props[family_index].minImageTransferGranularity.height,
												   queue_family_props[family_index].minImageTransferGranularity.depth };
				
				_vkQueues.push_back( std::move(vq) );
			}
		}

		return true;
	}

/*
=================================================
	_ValidateInstanceVersion
=================================================
*/
	void VDeviceInitializer::_ValidateInstanceVersion (INOUT uint &version) const
	{
		const uint	min_ver		= VK_API_VERSION_1_0;
		const uint	old_ver		= version;
		uint		current_ver	= 0;

		VK_CALL( vkEnumerateInstanceVersion( OUT &current_ver ));

		version = Min( Max( version, min_ver ), Max( current_ver, min_ver ));
		
		if ( old_ver != version )
			AE_LOGI( "Vulkan instance version changed to: "s << ToString(VK_VERSION_MAJOR(version)) << '.' << ToString(VK_VERSION_MINOR(version)) );
	}

/*
=================================================
	_ValidateInstanceLayers
=================================================
*/
	void VDeviceInitializer::_ValidateInstanceLayers (INOUT Array<const char*> &layers) const
	{
		Array<VkLayerProperties> inst_layers;

		// load supported layers
		uint	count = 0;
		VK_CALL( vkEnumerateInstanceLayerProperties( OUT &count, null ));

		if (count == 0)
		{
			layers.clear();
			return;
		}

		inst_layers.resize( count );
		VK_CALL( vkEnumerateInstanceLayerProperties( OUT &count, OUT inst_layers.data() ));


		// validate
		for (auto iter = layers.begin(); iter != layers.end();)
		{
			bool	found = false;

			for (auto& prop : inst_layers)
			{
				if ( StringView(*iter) == prop.layerName ) {
					found = true;
					break;
				}
			}

			if ( not found )
			{
				AE_LOGI( "Vulkan layer \""s << (*iter) << "\" not supported and will be removed" );

				iter = layers.erase( iter );
			}
			else
				++iter;
		}
	}

/*
=================================================
	_ValidateInstanceExtensions
=================================================
*/
	void VDeviceInitializer::_ValidateInstanceExtensions (INOUT Array<const char*> &extensions) const
	{
		using ExtName_t = StaticString<VK_MAX_EXTENSION_NAME_SIZE>;

		HashSet<ExtName_t>	instance_extensions;


		// load supported extensions
		uint	count = 0;
		VK_CALL( vkEnumerateInstanceExtensionProperties( null, OUT &count, null ));

		if ( count == 0 )
		{
			extensions.clear();
			return;
		}

		Array< VkExtensionProperties >		inst_ext;
		inst_ext.resize( count );

		VK_CALL( vkEnumerateInstanceExtensionProperties( null, OUT &count, OUT inst_ext.data() ));

		for (auto& ext : inst_ext) {
			instance_extensions.insert( StringView(ext.extensionName) );
		}


		// validate
		for (auto iter = extensions.begin(); iter != extensions.end();)
		{
			if ( instance_extensions.find( ExtName_t{*iter} ) == instance_extensions.end() )
			{
				AE_LOGI( "Vulkan instance extension \""s << (*iter) << "\" not supported and will be removed" );

				iter = extensions.erase( iter );
			}
			else
				 ++iter;
		}
	}
	
/*
=================================================
	_ValidateDeviceExtensions
=================================================
*/
	void VDeviceInitializer::_ValidateDeviceExtensions (INOUT Array<const char*> &extensions) const
	{
		// load supported device extensions
		uint	count = 0;
		VK_CALL( vkEnumerateDeviceExtensionProperties( _vkPhysicalDevice, null, OUT &count, null ));

		if ( count == 0 )
		{
			extensions.clear();
			return;
		}

		Array< VkExtensionProperties >	dev_ext;
		dev_ext.resize( count );

		VK_CALL( vkEnumerateDeviceExtensionProperties( _vkPhysicalDevice, null, OUT &count, OUT dev_ext.data() ));


		// validate
		for (auto iter = extensions.begin(); iter != extensions.end();)
		{
			bool	found = false;

			for (auto& ext : dev_ext)
			{
				if ( StringView(*iter) == ext.extensionName )
				{
					found = true;
					break;
				}
			}

			if ( not found )
			{
				AE_LOGI( "Vulkan device extension \""s << (*iter) << "\" not supported and will be removed" );

				iter = extensions.erase( iter );
			}
			else
				++iter;
		}
	}

/*
=================================================
	GetRecomendedInstanceLayers
=================================================
*/
	ArrayView<const char*>	VDeviceInitializer::GetRecomendedInstanceLayers ()
	{
		#ifdef PLATFORM_ANDROID
			static const char*	instance_layers[] = {
				"VK_LAYER_GOOGLE_threading",
				"VK_LAYER_LUNARG_parameter_validation",
				"VK_LAYER_LUNARG_object_tracker",
				"VK_LAYER_LUNARG_core_validation",
				"VK_LAYER_GOOGLE_unique_objects",

				// may be unsupported:
				"VK_LAYER_LUNARG_vktrace",
				"VK_LAYER_ARM_MGD",
				"VK_LAYER_ARM_mali_perf_doc"
			};
		#else
			static const char*	instance_layers[] = {
				"VK_LAYER_LUNARG_standard_validation",
				"VK_LAYER_LUNARG_assistant_layer"
			};
		#endif
		return instance_layers;
	}
	
/*
=================================================
	GetInstanceExtensions_v100
=================================================
*/
	ArrayView<const char*>	VDeviceInitializer::GetInstanceExtensions_v100 ()
	{
		static const char*	instance_extensions[] =
		{
			#ifdef VK_KHR_surface
				VK_KHR_SURFACE_EXTENSION_NAME,
			#endif
			#ifdef VK_KHR_get_physical_device_properties2
				VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
			#endif
			#ifdef VK_KHR_get_surface_capabilities2
				VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME,
			#endif
			#ifdef VK_EXT_debug_utils
				VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
			#elif defined VK_EXT_debug_report
				VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
			#endif
		};
		return instance_extensions;
	}
	
/*
=================================================
	GetInstanceExtensions_v110
=================================================
*/
	ArrayView<const char*>	VDeviceInitializer::GetInstanceExtensions_v110 ()
	{
		static const char*	instance_extensions[] =
		{
			#ifdef VK_KHR_surface
				VK_KHR_SURFACE_EXTENSION_NAME,
			#endif
			#ifdef VK_EXT_debug_utils
				VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
			#endif
		};
		return instance_extensions;
	}
	
/*
=================================================
	GetDeviceExtensions_v100
=================================================
*/
	ArrayView<const char*>	VDeviceInitializer::GetDeviceExtensions_v100 ()
	{
		static const char *	device_extensions[] =
		{
			VK_KHR_SWAPCHAIN_EXTENSION_NAME,

			#ifdef VK_KHR_get_memory_requirements2
				VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,
			#endif
			#ifdef VK_KHR_bind_memory2
				VK_KHR_BIND_MEMORY_2_EXTENSION_NAME,
			#endif
			#ifdef VK_KHR_dedicated_allocation
				VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME,
			#endif
			#ifdef VK_KHR_create_renderpass2
				VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME,
			#endif
			#ifdef VK_KHR_sampler_ycbcr_conversion
				VK_KHR_SAMPLER_YCBCR_CONVERSION_EXTENSION_NAME,
			#endif
			#ifdef VK_KHR_descriptor_update_template
				VK_KHR_DESCRIPTOR_UPDATE_TEMPLATE_EXTENSION_NAME,
			#endif
		};
		return device_extensions;
	}
	
/*
=================================================
	GetDeviceExtensions_v110
=================================================
*/
	ArrayView<const char*>	VDeviceInitializer::GetDeviceExtensions_v110 ()
	{
		static const char *	device_extensions[] =
		{
			VK_KHR_SWAPCHAIN_EXTENSION_NAME,

			#ifdef VK_KHR_create_renderpass2
				VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME,
			#endif
			#ifdef VK_KHR_draw_indirect_count
				VK_KHR_DRAW_INDIRECT_COUNT_EXTENSION_NAME,
			#endif
			#ifdef VK_KHR_8bit_storage
				VK_KHR_8BIT_STORAGE_EXTENSION_NAME,
			#endif
			#ifdef VK_EXT_conservative_rasterization
				VK_EXT_CONSERVATIVE_RASTERIZATION_EXTENSION_NAME,
			#endif
			#ifdef VK_EXT_sample_locations
				VK_EXT_SAMPLE_LOCATIONS_EXTENSION_NAME,
			#endif
			#ifdef VK_KHR_push_descriptor
				VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME,
			#endif
			#ifdef VK_KHR_shader_atomic_int64
				VK_KHR_SHADER_ATOMIC_INT64_EXTENSION_NAME,
			#endif
			#ifdef VK_KHR_shader_float16_int8
				VK_KHR_SHADER_FLOAT16_INT8_EXTENSION_NAME,
			#endif
			#ifdef VK_KHR_shader_float_controls
				VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME,
			#endif
			#ifdef VK_EXT_blend_operation_advanced
				VK_EXT_BLEND_OPERATION_ADVANCED_EXTENSION_NAME,
			#endif
			#ifdef VK_EXT_conditional_rendering
				VK_EXT_CONDITIONAL_RENDERING_EXTENSION_NAME,
			#endif
			#ifdef VK_EXT_inline_uniform_block
				VK_EXT_INLINE_UNIFORM_BLOCK_EXTENSION_NAME,
			#endif
			#ifdef VK_EXT_descriptor_indexing
				VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
			#endif
			#ifdef VK_EXT_memory_budget
				VK_EXT_MEMORY_BUDGET_EXTENSION_NAME,
			#endif

			// Vendor specific extensions
			#ifdef VK_NV_mesh_shader
				VK_NV_MESH_SHADER_EXTENSION_NAME,
			#endif
			#ifdef VK_NV_shader_image_footprint
				VK_NV_SHADER_IMAGE_FOOTPRINT_EXTENSION_NAME,
			#endif
			#ifdef VK_NV_shading_rate_image
				VK_NV_SHADING_RATE_IMAGE_EXTENSION_NAME,
			#endif
			#ifdef VK_NV_fragment_shader_barycentric
				VK_NV_FRAGMENT_SHADER_BARYCENTRIC_EXTENSION_NAME,
			#endif
			#ifdef VK_NV_ray_tracing
				VK_NV_RAY_TRACING_EXTENSION_NAME,
			#endif
			#ifdef VK_NVX_device_generated_commands
				VK_NVX_DEVICE_GENERATED_COMMANDS_EXTENSION_NAME,
			#endif
		};
		return device_extensions;
	}

}	// AE::Vulkan

#endif	// AE_ENABLE_VULKAN
