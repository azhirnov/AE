// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef AE_ENABLE_VULKAN

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
		struct InstanceVersion
		{
			uint	major	: 16;
			uint	minor	: 16;

			InstanceVersion () : major{0}, minor{0} {}
			InstanceVersion (uint maj, uint min) : major{maj}, minor{min} {}

			ND_ bool  operator == (const InstanceVersion &rhs) const;
			ND_ bool  operator >  (const InstanceVersion &rhs) const;
			ND_ bool  operator >= (const InstanceVersion &rhs) const;
		};

		struct EnabledFeatures
		{
			// vulkan 1.1 core
			bool	bindMemory2				: 1;
			bool	dedicatedAllocation		: 1;
			bool	descriptorUpdateTemplate: 1;
			bool	imageViewUsage			: 1;
			bool	create2DArrayCompatible	: 1;
			bool	commandPoolTrim			: 1;
			bool	dispatchBase			: 1;
			// vulkan 1.2 core
			bool	samplerMirrorClamp		: 1;
			bool	shaderAtomicInt64		: 1;	// for uniform/storage buffer, for shared variables check features
			bool	float16Arithmetic		: 1;
			bool	bufferAddress			: 1;
			bool	descriptorIndexing		: 1;
			bool	renderPass2				: 1;
			bool	depthStencilResolve		: 1;
			bool	drawIndirectCount		: 1;
			bool	spirv14					: 1;
			// window extensions
			bool	surface					: 1;
			bool	surfaceCaps2			: 1;
			bool	swapchain				: 1;
			// extensions
			bool	debugUtils				: 1;
			bool	meshShaderNV			: 1;
			bool	rayTracingNV			: 1;
			bool	shadingRateImageNV		: 1;
		//	bool	pushDescriptor			: 1;
		//	bool	inlineUniformBlock		: 1;
			bool	shaderClock				: 1;
			bool	timelineSemaphore		: 1;
			bool	pushDescriptor			: 1;
		};

		struct DeviceProperties
		{
			VkPhysicalDeviceProperties							properties;
			VkPhysicalDeviceFeatures							features;
			VkPhysicalDeviceMemoryProperties					memoryProperties;
			#ifdef VK_NV_mesh_shader
			VkPhysicalDeviceMeshShaderFeaturesNV				meshShaderFeatures;
			VkPhysicalDeviceMeshShaderPropertiesNV				meshShaderProperties;
			#endif
			#ifdef VK_NV_shading_rate_image
			VkPhysicalDeviceShadingRateImageFeaturesNV			shadingRateImageFeatures;
			VkPhysicalDeviceShadingRateImagePropertiesNV		shadingRateImageProperties;
			#endif
			#ifdef VK_NV_ray_tracing
			VkPhysicalDeviceRayTracingPropertiesNV				rayTracingProperties;
			#endif
			#ifdef VK_KHR_shader_clock
			VkPhysicalDeviceShaderClockFeaturesKHR				shaderClock;
			#endif
			#ifdef VK_KHR_timeline_semaphore
			VkPhysicalDeviceTimelineSemaphorePropertiesKHR		timelineSemaphoreProps;
			#endif
			#ifdef VK_KHR_buffer_device_address
			VkPhysicalDeviceBufferDeviceAddressFeaturesKHR		bufferDeviceAddress;
			#endif
			#ifdef VK_KHR_depth_stencil_resolve
			VkPhysicalDeviceDepthStencilResolvePropertiesKHR	depthStencilResolve;
			#endif
			#ifdef VK_KHR_shader_atomic_int64
			VkPhysicalDeviceShaderAtomicInt64FeaturesKHR		shaderAtomicInt64;
			#endif
			#ifdef VK_VERSION_1_2
			VkPhysicalDeviceVulkan11Properties					properties110;
			VkPhysicalDeviceVulkan12Properties					properties120;
			#endif
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

		ND_ VkDevice				GetVkDevice ()					const	{ return _vkLogicalDevice; }
		ND_ VkPhysicalDevice		GetVkPhysicalDevice ()			const	{ return _vkPhysicalDevice; }
		ND_ VkInstance				GetVkInstance ()				const	{ return _vkInstance; }
		ND_ InstanceVersion			GetVkVersion ()					const	{ return _vkVersion; }
		ND_ ArrayView< VQueue >		GetVkQueues ()					const	{ return _vkQueues; }
		ND_ VQueuePtr				GetQueue (EQueueType type)		const	{ return uint(type) < _queueTypes.size() ? _queueTypes[uint(type)] : null; }

		// check extensions
		ND_ bool HasInstanceExtension (StringView name) const;
		ND_ bool HasDeviceExtension (StringView name) const;
		
		bool SetObjectName (uint64_t id, NtStringView name, VkObjectType type) const;

		void GetQueueFamilies (EQueueMask mask, OUT VQueueFamilyIndices_t &) const;
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
			VkQueueFlagBits		flags		= VkQueueFlagBits(0);
			float				priority	= 0.0f;
			DebugName_t			debugName;

			QueueCreateInfo () {}
			QueueCreateInfo (VkQueueFlagBits flags, float priority = 0.0f, StringView name = {}) : flags{flags}, priority{priority}, debugName{name} {}
		};


	// methods
	public:
		ND_ InstanceVersion  GetInstanceVersion () const;

		bool CreateInstance (NtStringView appName, NtStringView engineName, ArrayView<const char*> instanceLayers,
							 ArrayView<const char*> instanceExtensions = {}, InstanceVersion version = {1,2}, uint appVer = 0, uint engineVer = 0);
		bool SetInstance (VkInstance value, ArrayView<const char*> instanceExtensions = {});
		bool DestroyInstance ();

		bool ChooseDevice (StringView deviceName);
		bool ChooseHighPerformanceDevice ();
		bool SetPhysicalDevice (VkPhysicalDevice value);
		
		bool CreateLogicalDevice (ArrayView<QueueCreateInfo> queues, ArrayView<const char*> extensions = {});
		bool SetLogicalDevice (VkDevice value, ArrayView<const char*> extensions = {});
		bool DestroyLogicalDevice ();
		
		ND_ static ArrayView<const char*>	GetRecomendedInstanceLayers ();


	private:
		ND_ static ArrayView<const char*>	GetInstanceExtensions_v100 ();
		ND_ static ArrayView<const char*>	GetInstanceExtensions_v110 ();
		ND_ static ArrayView<const char*>	GetInstanceExtensions_v120 ();
		ND_ static ArrayView<const char*>	GetDeviceExtensions_v100 ();
		ND_ static ArrayView<const char*>	GetDeviceExtensions_v110 ();
		ND_ static ArrayView<const char*>	GetDeviceExtensions_v120 ();

		ND_ static ArrayView<const char*>	_GetInstanceExtensions (InstanceVersion ver);
		ND_ static ArrayView<const char*>	_GetDeviceExtensions (InstanceVersion ver);

		void _ValidateInstanceVersion (INOUT uint &version) const;
		void _ValidateInstanceLayers (INOUT Array<const char*> &layers) const;
		void _ValidateInstanceExtensions (INOUT Array<const char*> &ext) const;
		void _ValidateDeviceExtensions (INOUT Array<const char*> &ext) const;

		bool _SetupQueues (ArrayView<QueueCreateInfo> queue);
		bool _ChooseQueueIndex (ArrayView<VkQueueFamilyProperties> props, INOUT VkQueueFlagBits &flags, OUT uint &index) const;
		bool _InitDeviceFeatures ();
		void _UpdateInstanceVersion (uint ver);

		bool _SetupQueueTypes ();
		bool _AddGraphicsQueue ();
		bool _AddAsyncComputeQueue ();
		bool _AddAsyncTransferQueue ();

		void _SetupInstanceBackwardCompatibility ();
		void _SetupDeviceBackwardCompatibility ();
	};


}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
