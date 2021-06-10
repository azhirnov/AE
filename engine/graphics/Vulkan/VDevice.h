// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef AE_ENABLE_VULKAN

# include "stl/Utils/Version.h"
# include "graphics/Vulkan/VQueue.h"

namespace AE::Graphics
{

	//
	// Vulkan Device
	//

	class VDevice : public VulkanDeviceFn, public Noncopyable
	{
	// types
	public:
		using InstanceVersion = Version2;
		
		struct EnabledFeatures
		{
			// vulkan 1.1 core
			bool	bindMemory2					: 1;
			bool	dedicatedAllocation			: 1;
			bool	descriptorUpdateTemplate	: 1;
			bool	imageViewUsage				: 1;
			bool	commandPoolTrim				: 1;
			bool	dispatchBase				: 1;
			bool	array2DCompatible			: 1;
			bool	blockTexelView				: 1;
			bool	maintenance3				: 1;
			bool	subgroup					: 1;	// GL_KHR_shader_subgroup and other
			// vulkan 1.2 core
			bool	samplerMirrorClamp			: 1;
			bool	shaderAtomicInt64			: 1;	// GL_EXT_shader_atomic_int64, for uniform/storage buffer, for shared variables check features
			bool	float16Arithmetic			: 1;	// GL_EXT_shader_16bit_storage
			bool	int8Arithmetic				: 1;	// GL_EXT_shader_8bit_storage
			bool	bufferAddress				: 1;	// GL_EXT_buffer_reference
			bool	descriptorIndexing			: 1;	// GL_EXT_nonuniform_qualifier 
			bool	renderPass2					: 1;
			bool	depthStencilResolve			: 1;
			bool	drawIndirectCount			: 1;
			bool	spirv14						: 1;
			bool	memoryModel					: 1;	// GL_KHR_memory_scope_semantics, SPV_KHR_vulkan_memory_model
			bool	samplerFilterMinmax			: 1;
			bool	hostQueryReset				: 1;
			bool	subgroupExtendedTypes		: 1;
			// window extensions
			bool	surface						: 1;
			bool	surfaceCaps2				: 1;
			bool	swapchain					: 1;
			// extensions
			bool	debugUtils					: 1;
			bool	shaderClock					: 1;
			bool	timelineSemaphore			: 1;
			bool	pushDescriptor				: 1;
			bool	robustness2					: 1;
			bool	shaderStencilExport			: 1;
			bool	extendedDynamicState		: 1;
			bool	accelerationStructure		: 1;
			bool	rayTracingPipeline			: 1;	// GLSL_EXT_ray_tracing
			bool	rayQuery					: 1;	// GLSL_EXT_ray_query
			bool	pipelineExecutableProps		: 1;
			// NV extensions
			bool	compShaderDerivativesNV		: 1;
			bool	shaderSMBuiltinsNV			: 1;	// GL_NV_shader_sm_builtins
			bool	meshShaderNV				: 1;	// GL_NV_mesh_shader
			bool	rayTracingNV				: 1;	// GL_NV_ray_tracing
			bool	shadingRateImageNV			: 1;	// GL_NV_shading_rate_image
			bool	imageFootprintNV			: 1;	// GL_NV_shader_texture_footprint
			bool	fragmentBarycentricNV		: 1;	// GL_NV_fragment_shader_barycentric
			// TODO: shader atomic float
			// TODO: shader image int64
		};

		struct DeviceProperties
		{
			VkPhysicalDeviceProperties								properties;
			VkPhysicalDeviceFeatures								features;
			VkPhysicalDeviceMemoryProperties						memoryProperties;
			// extensions
			#ifdef VK_VERSION_1_1
			VkPhysicalDeviceSubgroupProperties						subgroup;
			#endif
			#ifdef VK_KHR_vulkan_memory_model
			VkPhysicalDeviceVulkanMemoryModelFeaturesKHR			memoryModel;
			#endif
			#ifdef VK_KHR_shader_clock
			VkPhysicalDeviceShaderClockFeaturesKHR					shaderClockFeatures;
			#endif
			#ifdef VK_KHR_timeline_semaphore
			VkPhysicalDeviceTimelineSemaphorePropertiesKHR			timelineSemaphoreProps;
			#endif
			#ifdef VK_KHR_buffer_device_address
			VkPhysicalDeviceBufferDeviceAddressFeaturesKHR			bufferDeviceAddress;
			#endif
			#ifdef VK_KHR_depth_stencil_resolve
			VkPhysicalDeviceDepthStencilResolvePropertiesKHR		depthStencilResolve;
			#endif
			#ifdef VK_KHR_shader_atomic_int64
			VkPhysicalDeviceShaderAtomicInt64FeaturesKHR			shaderAtomicInt64;
			#endif
			#ifdef VK_EXT_descriptor_indexing
			VkPhysicalDeviceDescriptorIndexingFeaturesEXT			descriptorIndexingFeatures;
			VkPhysicalDeviceDescriptorIndexingPropertiesEXT			descriptorIndexingProperties;
			#endif
			#ifdef VK_VERSION_1_2
			//VkPhysicalDeviceVulkan11Properties					properties110;		// duplicates values from other properties
			//VkPhysicalDeviceVulkan12Properties					properties120;
			#endif
			#ifdef VK_EXT_robustness2
			VkPhysicalDeviceRobustness2FeaturesEXT					robustness2Features;
			VkPhysicalDeviceRobustness2PropertiesEXT				robustness2Properties;
			#endif
			#ifdef VK_KHR_maintenance3
			VkPhysicalDeviceMaintenance3Properties					maintenance3Properties;
			#endif
			#ifdef VK_EXT_sampler_filter_minmax
			VkPhysicalDeviceSamplerFilterMinmaxPropertiesEXT		samplerFilerMinmaxProperties;
			#endif
			#ifdef VK_EXT_extended_dynamic_state
			//VkPhysicalDeviceExtendedDynamicStateFeaturesEXT		extendedDynamicStateFeatures;
			#endif
			#ifdef VK_KHR_acceleration_structure
			VkPhysicalDeviceAccelerationStructureFeaturesKHR		accelerationStructureFeatures;
			VkPhysicalDeviceAccelerationStructurePropertiesKHR		accelerationStructureProperties;
			#endif
			#ifdef VK_KHR_ray_tracing_pipeline
			VkPhysicalDeviceRayTracingPipelineFeaturesKHR			rayTracingPipelineFeatures;
			VkPhysicalDeviceRayTracingPipelinePropertiesKHR			rayTracingPipelineProperties;
			#endif
			#ifdef VK_KHR_ray_query
			//VkPhysicalDeviceRayQueryFeaturesKHR					rayQueryFeatures;
			#endif
			#ifdef VK_KHR_pipeline_executable_properties
			//VkPhysicalDevicePipelineExecutablePropertiesFeaturesKHR	pipelineExecutablePropsFeatures;
			#endif
			#ifdef VK_EXT_host_query_reset
			//VkPhysicalDeviceHostQueryResetFeaturesEXT				hostQueryResetFeatures;
			#endif
			// NV extensions
			#ifdef VK_NV_compute_shader_derivatives
			VkPhysicalDeviceComputeShaderDerivativesFeaturesNV		computeShaderDerivativesNVFeatures;
			#endif
			#ifdef VK_NV_shader_sm_builtins
			//VkPhysicalDeviceShaderSMBuiltinsFeaturesNV			shaderSMBuiltinsNVFeatures;
			VkPhysicalDeviceShaderSMBuiltinsPropertiesNV			shaderSMBuiltinsNVProperties;
			#endif
			#ifdef VK_NV_mesh_shader
			//VkPhysicalDeviceMeshShaderFeaturesNV					meshShaderNVFeatures;
			VkPhysicalDeviceMeshShaderPropertiesNV					meshShaderNVProperties;
			#endif
			#ifdef VK_NV_shading_rate_image
			VkPhysicalDeviceShadingRateImageFeaturesNV				shadingRateImageNVFeatures;
			VkPhysicalDeviceShadingRateImagePropertiesNV			shadingRateImageNVProperties;
			#endif
			#ifdef VK_NV_ray_tracing
			VkPhysicalDeviceRayTracingPropertiesNV					rayTracingNVProperties;
			#endif
			#ifdef VK_NV_shader_image_footprint
			//VkPhysicalDeviceShaderImageFootprintFeaturesNV		shaderImageFootprintNVFeatures;
			#endif
			#ifdef VK_NV_fragment_shader_barycentric
			//VkPhysicalDeviceFragmentShaderBarycentricFeaturesNV	fragmentBarycentricNVFeatures;
			#endif
		};
		
		struct DeviceFlags
		{
			//EResourceState			graphicsShaderStages	= Zero;	// TODO
			VkPipelineStageFlagBits		allWritableStages		= Zero;
			VkPipelineStageFlagBits		allReadableStages		= Zero;
			VkImageCreateFlagBits		imageCreateFlags		= Zero;
		};


	private:
		using Queues_t			= FixedArray< VQueue, VConfig::MaxQueues >;
		using QueueTypes_t		= StaticArray< VQueuePtr, uint(EQueueType::_Count) >;
		
		using ExtensionName_t	= FixedString<VK_MAX_EXTENSION_NAME_SIZE>;
		using ExtensionSet_t	= HashSet< ExtensionName_t >;


	// variables
	protected:
		VkDevice				_vkLogicalDevice;

		Queues_t				_vkQueues;
		QueueTypes_t			_queueTypes;

		VkPhysicalDevice		_vkPhysicalDevice;
		VkInstance				_vkInstance;
		InstanceVersion			_vkVersion;
		
		EnabledFeatures			_features;
		DeviceFlags				_flags;

		VulkanDeviceFnTable		_deviceFnTable;
		
		DeviceProperties		_properties;
		
		ExtensionSet_t			_instanceExtensions;
		ExtensionSet_t			_deviceExtensions;


	// methods
	public:
		VDevice ();
		~VDevice ();
		
		ND_ EnabledFeatures const&	GetFeatures ()					const	{ return _features; }
		ND_ DeviceProperties const&	GetProperties ()				const	{ return _properties; }
		ND_ DeviceFlags const&		GetFlags ()						const	{ return _flags; }

		ND_ VkDevice				GetVkDevice ()					const	{ return _vkLogicalDevice; }
		ND_ VkPhysicalDevice		GetVkPhysicalDevice ()			const	{ return _vkPhysicalDevice; }
		ND_ VkInstance				GetVkInstance ()				const	{ return _vkInstance; }
		ND_ InstanceVersion			GetVkVersion ()					const	{ return _vkVersion; }
		ND_ ArrayView< VQueue >		GetVkQueues ()					const	{ return _vkQueues; }
		ND_ VQueuePtr				GetQueue (EQueueType type)		const	{ return uint(type) < _queueTypes.size() ? _queueTypes[uint(type)] : null; }

		// check extensions
		ND_ bool  HasInstanceExtension (StringView name) const;
		ND_ bool  HasDeviceExtension (StringView name) const;
		
		bool  SetObjectName (uint64_t id, NtStringView name, VkObjectType type) const;

		void  GetQueueFamilies (EQueueMask mask, OUT VQueueFamilyIndices_t &) const;
		
		bool  GetMemoryTypeIndex (uint memoryTypeBits, VkMemoryPropertyFlagBits includeFlags, VkMemoryPropertyFlagBits optFlags,
								  VkMemoryPropertyFlagBits excludeFlags, OUT uint &memoryTypeIndex) const;
	};



	//
	// Vulkan Device Initializer
	//

	class VDeviceInitializer final : public VDevice
	{
	// types
	public:
		struct QueueCreateInfo
		{
			VkQueueFlagBits		flags		= Zero;
			float				priority	= 0.0f;
			DebugName_t			debugName;

			QueueCreateInfo () {}
			QueueCreateInfo (VkQueueFlagBits flags, float priority = 0.0f, StringView name = {}) : flags{flags}, priority{priority}, debugName{name} {}
		};
		
		struct ObjectDbgInfo
		{
			StringView				type;
			StringView				name;
			uint64_t				handle;
		};

		struct DebugReport
		{
			ArrayView<ObjectDbgInfo>	objects;
			StringView					message;
			bool						isError		= false;
		};
		using DebugReport_t = Function< void (const DebugReport &) >;


	// variable
	private:
		VkDebugUtilsMessengerEXT	_debugUtilsMessenger	= VK_NULL_HANDLE;
		DebugReport_t				_callback;
		
		Mutex						_guard;
		bool						_breakOnValidationError	= true;
		const bool					_enableInfoLog			= false;
		Array<ObjectDbgInfo>		_tempObjectDbgInfos;
		String						_tempString;


	// methods
	public:
		explicit VDeviceInitializer (bool enableInfoLog = false);
		~VDeviceInitializer ();

		ND_ InstanceVersion  GetInstanceVersion () const;

		bool  CreateInstance (NtStringView appName, NtStringView engineName, ArrayView<const char*> instanceLayers,
							  ArrayView<const char*> instanceExtensions = {}, InstanceVersion version = {1,2}, uint appVer = 0, uint engineVer = 0);
		bool  SetInstance (VkInstance value, ArrayView<const char*> instanceExtensions = {});

		bool  DestroyInstance ();

		bool  ChooseDevice (StringView deviceName);
		bool  ChooseHighPerformanceDevice ();
		bool  SetPhysicalDevice (VkPhysicalDevice value);
		
		bool  CreateLogicalDevice (ArrayView<QueueCreateInfo> queues, ArrayView<const char*> extensions = {});
		//bool  SetLogicalDevice (VkDevice value, ArrayView<const char*> extensions = {});
		bool  DestroyLogicalDevice ();
		
		bool  CreateDebugCallback (VkDebugUtilsMessageSeverityFlagsEXT severity, DebugReport_t &&callback = Default);
		void  DestroyDebugCallback ();

		bool  CheckConstantLimits () const;

		ND_ static ArrayView<const char*>	GetRecomendedInstanceLayers ();

		ND_ VulkanDeviceFnTable &			EditDeviceFnTable ()			{ return _deviceFnTable; }


	private:
		// TODO: make public?
		ND_ static ArrayView<const char*>	GetInstanceExtensions_v100 ();
		ND_ static ArrayView<const char*>	GetInstanceExtensions_v110 ();
		ND_ static ArrayView<const char*>	GetInstanceExtensions_v120 ();
		ND_ static ArrayView<const char*>	GetDeviceExtensions_v100 ();
		ND_ static ArrayView<const char*>	GetDeviceExtensions_v110 ();
		ND_ static ArrayView<const char*>	GetDeviceExtensions_v120 ();

		ND_ static ArrayView<const char*>	_GetInstanceExtensions (InstanceVersion ver);
		ND_ static ArrayView<const char*>	_GetDeviceExtensions (InstanceVersion ver);

		void  _ValidateInstanceVersion (INOUT uint &version) const;
		void  _ValidateInstanceLayers (INOUT Array<const char*> &layers) const;
		void  _ValidateInstanceExtensions (INOUT Array<const char*> &ext) const;
		void  _ValidateDeviceExtensions (INOUT Array<const char*> &ext) const;

		void  _LogPhysicalDevices () const;

		bool  _SetupQueues (ArrayView<QueueCreateInfo> queue);
		bool  _ChooseQueueIndex (ArrayView<VkQueueFamilyProperties> props, INOUT VkQueueFlagBits &flags, OUT uint &index) const;
		bool  _InitDeviceFeatures ();
		void  _InitDeviceFlags ();
		void  _UpdateInstanceVersion (uint ver);

		bool  _SetupQueueTypes ();
		bool  _AddGraphicsQueue ();
		bool  _AddAsyncComputeQueue ();
		bool  _AddAsyncTransferQueue ();
		
		VKAPI_ATTR static VkBool32 VKAPI_CALL
			_DebugUtilsCallback (VkDebugUtilsMessageSeverityFlagBitsEXT			messageSeverity,
								 VkDebugUtilsMessageTypeFlagsEXT				messageTypes,
								 const VkDebugUtilsMessengerCallbackDataEXT*	pCallbackData,
								 void*											pUserData);

		ND_ static StringView  _ObjectTypeToString (VkObjectType objType);

		void  _DebugReport (const DebugReport &);
	};
	
	static constexpr VkDebugUtilsMessageSeverityFlagsEXT	DefaultDebugMessageSeverity = //VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
																							//VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
																							VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
																							VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;


}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
