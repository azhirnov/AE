// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef AE_ENABLE_VULKAN

# include "graphics/Vulkan/VQueue.h"
# include "graphics/Vulkan/VulkanCheckError.h"

namespace AE::Graphics
{

	//
	// Vulkan Device
	//

	class VDevice : public VulkanDeviceFn, public Noncopyable
	{
	// types
	public:
		struct InstanceVersion {
			uint	major	: 16;
			uint	minor	: 16;

			InstanceVersion () : major{0}, minor{0} {}
			InstanceVersion (uint maj, uint min) : major{maj}, minor{min} {}

			ND_ bool  operator == (const InstanceVersion &rhs) const;
			ND_ bool  operator >  (const InstanceVersion &rhs) const;
			ND_ bool  operator >= (const InstanceVersion &rhs) const;
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
		
		struct {
			bool					debugUtils			: 1;
			bool					meshShaderNV		: 1;
			bool					rayTracingNV		: 1;
			bool					samplerMirrorClamp	: 1;
			bool					shadingRateImageNV	: 1;
			bool					descriptorIndexing	: 1;
		//	bool					pushDescriptor		: 1;
		//	bool					inlineUniformBlock	: 1;
		//	bool					atomicInt64			: 1;
		}						_supported;

		VulkanDeviceFnTable		_deviceFnTable;
		
		struct {
			VkPhysicalDeviceProperties						properties;
			VkPhysicalDeviceFeatures						features;
			VkPhysicalDeviceMemoryProperties				memoryProperties;
			VkPhysicalDeviceMeshShaderFeaturesNV			meshShaderFeatures;
			VkPhysicalDeviceMeshShaderPropertiesNV			meshShaderProperties;
			VkPhysicalDeviceShadingRateImageFeaturesNV		shadingRateImageFeatures;
			VkPhysicalDeviceShadingRateImagePropertiesNV	shadingRateImageProperties;
			VkPhysicalDeviceRayTracingPropertiesNV			rayTracingProperties;
		}						_deviceInfo;
		
		ExtensionSet_t			_instanceExtensions;
		ExtensionSet_t			_deviceExtensions;


	// methods
	public:
		VDevice ();
		~VDevice ();
		
		ND_ bool					IsDebugUtilsEnabled ()			const	{ return _supported.debugUtils; }
		ND_ bool					IsMeshShaderEnabled ()			const	{ return _supported.meshShaderNV; }
		ND_ bool					IsRayTracingEnabled ()			const	{ return _supported.rayTracingNV; }
		ND_ bool					IsSamplerMirrorClampEnabled ()	const	{ return _supported.samplerMirrorClamp; }
		ND_ bool					IsShadingRateImageEnabled ()	const	{ return _supported.shadingRateImageNV; }
		ND_ bool					IsDescriptorIndexingEnabled ()	const	{ return _supported.descriptorIndexing; }

		ND_ VkDevice				GetVkDevice ()					const	{ return _vkLogicalDevice; }
		ND_ VkPhysicalDevice		GetVkPhysicalDevice ()			const	{ return _vkPhysicalDevice; }
		ND_ VkInstance				GetVkInstance ()				const	{ return _vkInstance; }
		ND_ InstanceVersion			GetVkVersion ()					const	{ return _vkVersion; }
		ND_ ArrayView< VQueue >		GetVkQueues ()					const	{ return _vkQueues; }
		ND_ VQueuePtr				GetQueue (EQueueType type)		const	{ return uint(type) < _queueTypes.size() ? _queueTypes[uint(type)] : null; }


		ND_ VkPhysicalDeviceProperties const&					GetDeviceProperties ()					const	{ return _deviceInfo.properties; }
		ND_ VkPhysicalDeviceFeatures const&						GetDeviceFeatures ()					const	{ return _deviceInfo.features; }
		//ND_ VkPhysicalDeviceMemoryProperties const&				GetDeviceMemoryProperties ()			const	{ return _deviceInfo.memoryProperties; }
		//ND_ VkPhysicalDeviceLimits const&						GetDeviceLimits ()						const	{ return _deviceInfo.properties.limits; }
		//ND_ VkPhysicalDeviceSparseProperties const&				GetDeviceSparseProperties ()			const	{ return _deviceInfo.properties.sparseProperties; }
		//ND_ VkPhysicalDeviceMeshShaderPropertiesNV const&		GetDeviceMeshShaderProperties ()		const	{ return _deviceInfo.meshShaderProperties; }
		//ND_ VkPhysicalDeviceRayTracingPropertiesNV const&		GetDeviceRayTracingProperties ()		const	{ return _deviceInfo.rayTracingProperties; }
		//ND_ VkPhysicalDeviceShadingRateImagePropertiesNV const&	GetDeviceShadingRateImageProperties ()	const	{ return _deviceInfo.shadingRateImageProperties; }
		
		// check extensions
		ND_ bool HasExtension (StringView name) const;
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
			VkQueueFlags		flags		= 0;
			float				priority	= 0.0f;
			DebugName_t			debugName;

			QueueCreateInfo () {}
			QueueCreateInfo (VkQueueFlags flags, float priority = 0.0f, StringView name = {}) : flags{flags}, priority{priority}, debugName{name} {}
		};


	// methods
	public:
		ND_ InstanceVersion  GetInstanceVersion () const;

		bool CreateInstance (NtStringView appName, NtStringView engineName, ArrayView<const char*> instanceLayers,
							 ArrayView<const char*> instanceExtensions, InstanceVersion version, uint appVer = 0, uint engineVer = 0);
		bool SetInstance (VkInstance value, ArrayView<const char*> instanceExtensions = {});
		bool DestroyInstance ();

		bool ChooseDevice (StringView deviceName);
		bool ChooseHighPerformanceDevice ();
		bool SetPhysicalDevice (VkPhysicalDevice value);
		
		bool CreateLogicalDevice (ArrayView<QueueCreateInfo> queues, ArrayView<const char*> extensions);
		bool SetLogicalDevice (VkDevice value, ArrayView<const char*> extensions = {});
		bool DestroyLogicalDevice ();
		
		ND_ static ArrayView<const char*>	GetRecomendedInstanceLayers ();
		ND_ static ArrayView<const char*>	GetInstanceExtensions_v100 ();
		ND_ static ArrayView<const char*>	GetInstanceExtensions_v110 ();
		ND_ static ArrayView<const char*>	GetDeviceExtensions_v100 ();
		ND_ static ArrayView<const char*>	GetDeviceExtensions_v110 ();

	private:
		void _ValidateInstanceVersion (INOUT uint &version) const;
		void _ValidateInstanceLayers (INOUT Array<const char*> &layers) const;
		void _ValidateInstanceExtensions (INOUT Array<const char*> &ext) const;
		void _ValidateDeviceExtensions (INOUT Array<const char*> &ext) const;

		bool _SetupQueues (ArrayView<QueueCreateInfo> queue);
		bool _ChooseQueueIndex (ArrayView<VkQueueFamilyProperties> props, INOUT VkQueueFlags &flags, OUT uint &index) const;
		bool _InitDeviceFeatures ();

		bool _SetupQueueTypes ();
		bool _AddGraphicsQueue ();
		bool _AddAsyncComputeQueue ();
		bool _AddAsyncTransferQueue ();
	};


}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
