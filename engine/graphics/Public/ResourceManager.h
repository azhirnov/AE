// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "graphics/Public/ImageDesc.h"
#include "graphics/Public/BufferDesc.h"
#include "graphics/Public/EResourceState.h"
#include "graphics/Public/PipelineDesc.h"
#include "graphics/Public/GfxMemAllocator.h"

namespace AE::Graphics
{

	//
	// Resource Manager interface
	//

	class IResourceManager
	{
	// types
	public:
		using NativeBufferDesc_t	= Union< NullUnion, VulkanBufferDesc >;
		using NativeImageDesc_t		= Union< NullUnion, VulkanImageDesc >;
		using NativeBufferHandle_t	= Union< NullUnion, BufferVk_t >;
		using NativeImageHandle_t	= Union< NullUnion, ImageVk_t >;
		using NativeMemInfo_t		= IGfxMemAllocator::NativeMemInfo_t;


	// interface
	public:
		virtual ~IResourceManager () {}

		ND_ virtual bool						IsSupported (const BufferDesc &desc) const = 0;
		ND_ virtual bool						IsSupported (const ImageDesc &desc) const = 0;
		ND_ virtual bool						IsSupported (GfxResourceID buffer, const BufferViewDesc &desc) const = 0;
		ND_ virtual bool						IsSupported (GfxResourceID image, const ImageViewDesc &desc) const = 0;

		ND_ virtual UniqueID<GfxResourceID>		CreateDependency (StringView dbgName = Default) = 0;
		ND_ virtual UniqueID<GfxResourceID>		CreateImage (const VirtualImageDesc &desc, StringView dbgName = Default) = 0;
		ND_ virtual UniqueID<GfxResourceID>		CreateBuffer (const VirtualBufferDesc &desc, StringView dbgName = Default) = 0;

		ND_ virtual UniqueID<GfxResourceID>		CreateImage (const ImageDesc &desc, EResourceState defaultState = Default, StringView dbgName = Default, const GfxMemAllocatorPtr &allocator = null) = 0;
		ND_ virtual UniqueID<GfxResourceID>		CreateBuffer (const BufferDesc &desc, StringView dbgName = Default, const GfxMemAllocatorPtr &allocator = null) = 0;
		
		ND_ virtual UniqueID<GfxResourceID>		CreateImage (const NativeImageDesc_t &desc, StringView dbgName = Default) = 0;
		ND_ virtual UniqueID<GfxResourceID>		CreateBuffer (const NativeBufferDesc_t &desc, StringView dbgName = Default) = 0;

			virtual UniqueID<PipelinePackID>	LoadPipelinePack (const SharedPtr<RStream> &stream) = 0;
			virtual bool						ReloadPipelinePack (const SharedPtr<RStream> &stream, PipelinePackID id) = 0;

		ND_ virtual GraphicsPipelineID			GetGraphicsPipeline (const PipelineName &name, const GraphicsPipelineDesc &desc) = 0;
		ND_ virtual MeshPipelineID				GetMeshPipeline (const PipelineName &name, const MeshPipelineDesc &desc) = 0;
		ND_ virtual ComputePipelineID			GetComputePipeline (const PipelineName &name, const ComputePipelineDesc &desc) = 0;
		ND_ virtual RayTracingPipelineID		GetRayTracingPipeline (const PipelineName &name, const RayTracingPipelineDesc &desc) = 0;
		
			virtual bool						LoadSamplerPack (const SharedPtr<RStream> &stream) = 0;
		//ND_ virtual SamplerID					GetSampler (const SamplerName &name) = 0;

		ND_ virtual bool						IsResourceAlive (GfxResourceID id) const = 0;

			virtual bool						ReleaseResource (UniqueID<GfxResourceID> &id) = 0;
			virtual bool						ReleaseResource (UniqueID<PipelinePackID> &id) = 0;
			virtual bool						ReleaseResource (UniqueID<BakedCommandBufferID> &id) = 0;

		// returned reference is valid until resource is alive
		ND_ virtual BufferDesc const&			GetBufferDescription (GfxResourceID id) const = 0;
		ND_ virtual ImageDesc const&			GetImageDescription (GfxResourceID id) const = 0;
		ND_ virtual VirtualBufferDesc const&	GetVirtualBufferDescription (GfxResourceID id) const = 0;
		ND_ virtual VirtualImageDesc const&		GetVirtualImageDescription (GfxResourceID id) const = 0;

		//ND_ virtual NativeBufferDesc_t		GetBufferNativeDesc (GfxResourceID id) const = 0;
		//ND_ virtual NativeImageDesc_t			GetImageNativeDesc (GfxResourceID id) const = 0;

		ND_ virtual NativeBufferHandle_t		GetBufferHandle (GfxResourceID id) const = 0;
		ND_ virtual NativeImageHandle_t			GetImageHandle (GfxResourceID id) const = 0;

			virtual bool						GetMemoryInfo (GfxResourceID id, OUT NativeMemInfo_t &info) const = 0;
	};


}	// AE::Graphics
