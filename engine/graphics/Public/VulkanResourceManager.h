// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef AE_ENABLE_VULKAN

# include "graphics/Public/ImageDesc.h"
# include "graphics/Public/BufferDesc.h"
# include "graphics/Public/EResourceState.h"
# include "graphics/Public/PipelineDesc.h"
# include "graphics/Public/DescriptorSet.h"
# include "graphics/Public/VulkanTypes.h"
# include "stl/Containers/UntypedStorage.h"

namespace AE::Graphics
{

	//
	// Vulkan Memory Allocator interface
	//

	class VMemAllocator : public EnableRC<VMemAllocator>
	{
	// types
	public:
		using Storage_t	= UntypedStorage< sizeof(ulong) * 4, alignof(ulong) >;

	// interface
	public:
		virtual ~VMemAllocator () {}

		virtual bool AllocForImage (VkImage image, const ImageDesc &desc, OUT Storage_t &data) = 0;
		virtual bool AllocForBuffer (VkBuffer buffer, const BufferDesc &desc, OUT Storage_t &data) = 0;
		//virtual bool AllocateForAccelStruct (const NativeAccelStructHandle_t &as, EMemoryType memType, OUT Storage_t &data) = 0;

		virtual bool Dealloc (INOUT Storage_t &data) = 0;
			
		virtual bool GetInfo (const Storage_t &data, OUT VulkanMemoryObjInfo &info) const = 0;
	};
	using VMemAllocatorPtr = RC< VMemAllocator >;



	//
	// Vulkan Resource Manager interface
	//

	class VResourceManager
	{
	// interface
	public:
		virtual ~VResourceManager () {}

		ND_ virtual bool						IsSupported (const BufferDesc &desc) const = 0;
		ND_ virtual bool						IsSupported (const ImageDesc &desc) const = 0;
		ND_ virtual bool						IsSupported (BufferID buffer, const BufferViewDesc &desc) const = 0;
		ND_ virtual bool						IsSupported (ImageID image, const ImageViewDesc &desc) const = 0;

		ND_ virtual UniqueID<ImageID>			CreateImage (const ImageDesc &desc, EResourceState defaultState = Default, StringView dbgName = Default, const VMemAllocatorPtr &allocator = null) = 0;
		ND_ virtual UniqueID<BufferID>			CreateBuffer (const BufferDesc &desc, StringView dbgName = Default, const VMemAllocatorPtr &allocator = null) = 0;
		
		ND_ virtual UniqueID<ImageID>			CreateImage (const VulkanImageDesc &desc, StringView dbgName = Default) = 0;
		ND_ virtual UniqueID<BufferID>			CreateBuffer (const VulkanBufferDesc &desc, StringView dbgName = Default) = 0;

			virtual bool						InitializeDescriptorSet (GraphicsPipelineID ppln, const DescriptorSetName &dsName, OUT DescriptorSet &ds) const = 0;
			virtual bool						InitializeDescriptorSet (MeshPipelineID ppln, const DescriptorSetName &dsName, OUT DescriptorSet &ds) const = 0;
			virtual bool						InitializeDescriptorSet (ComputePipelineID ppln, const DescriptorSetName &dsName, OUT DescriptorSet &ds) const = 0;
			virtual bool						InitializeDescriptorSet (RayTracingPipelineID ppln, const DescriptorSetName &dsName, OUT DescriptorSet &ds) const = 0;
			virtual bool						InitializeDescriptorSet (const PipelineName &pplnName, const DescriptorSetName &dsName, OUT DescriptorSet &ds) const = 0;
		ND_ virtual DescriptorSetID				CreateDescriptorSet (const DescriptorSet &ds) = 0;

			virtual UniqueID<PipelinePackID>	LoadPipelinePack (const RC<RStream> &stream) = 0;
			virtual bool						ReloadPipelinePack (const RC<RStream> &stream, PipelinePackID id) = 0;

		ND_ virtual GraphicsPipelineID			GetGraphicsPipeline (const PipelineName &name, const GraphicsPipelineDesc &desc) = 0;
		ND_ virtual MeshPipelineID				GetMeshPipeline (const PipelineName &name, const MeshPipelineDesc &desc) = 0;
		ND_ virtual ComputePipelineID			GetComputePipeline (const PipelineName &name, const ComputePipelineDesc &desc) = 0;
		ND_ virtual RayTracingPipelineID		GetRayTracingPipeline (const PipelineName &name, const RayTracingPipelineDesc &desc) = 0;
		
			virtual bool						LoadSamplerPack (const RC<RStream> &stream) = 0;
		//ND_ virtual SamplerID					GetSampler (const SamplerName &name) = 0;
			
		ND_ virtual bool						IsResourceAlive (ImageID id) const = 0;
		ND_ virtual bool						IsResourceAlive (BufferID id) const = 0;
		ND_ virtual bool						IsResourceAlive (DescriptorSetID id) const = 0;

		// returns 'true' if resource has been destroyed (when ref counter is zero).
			virtual bool						ReleaseResource (UniqueID<ImageID> &id) = 0;
			virtual bool						ReleaseResource (UniqueID<BufferID> &id) = 0;
			virtual bool						ReleaseResource (UniqueID<PipelinePackID> &id) = 0;

		// returned reference is valid until resource is alive
		ND_ virtual BufferDesc const&			GetBufferDescription (BufferID id) const = 0;
		ND_ virtual ImageDesc const&			GetImageDescription (ImageID id) const = 0;

		ND_ virtual VulkanPipelineInfo			GetPipelineNativeDesc (GraphicsPipelineID id) const = 0;
		ND_ virtual VulkanPipelineInfo			GetPipelineNativeDesc (MeshPipelineID id) const = 0;
		ND_ virtual VulkanPipelineInfo			GetPipelineNativeDesc (ComputePipelineID id) const = 0;
		ND_ virtual VulkanPipelineInfo			GetPipelineNativeDesc (RayTracingPipelineID id) const = 0;

		ND_ virtual VkBuffer					GetBufferHandle (BufferID id) const = 0;
		ND_ virtual VkImage						GetImageHandle (ImageID id) const = 0;

			virtual bool						GetMemoryInfo (ImageID id, OUT VulkanMemoryObjInfo &info) const = 0;
			virtual bool						GetMemoryInfo (BufferID id, OUT VulkanMemoryObjInfo &info) const = 0;

		// remove unused render passes, framebuffers and other.
			virtual void						RunResourceValidation (uint maxIterations = 200) = 0;
	};


}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
