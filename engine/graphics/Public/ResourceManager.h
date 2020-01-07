// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "graphics/Public/ImageDesc.h"
#include "graphics/Public/BufferDesc.h"
#include "graphics/Public/EResourceState.h"
#include "graphics/Public/PipelineDesc.h"

namespace AE::Graphics
{

	//
	// Resource Manager interface
	//

	class IResourceManager
	{
	// interface
	public:
		ND_ virtual GfxResourceID		CreateDependency (StringView dbgName = Default) = 0;
		ND_ virtual GfxResourceID		CreateImage (const VirtualImageDesc &desc, StringView dbgName = Default) = 0;
		ND_ virtual GfxResourceID		CreateBuffer (const VirtualBufferDesc &desc, StringView dbgName = Default) = 0;

		ND_ virtual GfxResourceID		CreateImage (const ImageDesc &desc, EResourceState defaultState = Default, StringView dbgName = Default) = 0;
		ND_ virtual GfxResourceID		CreateBuffer (const BufferDesc &desc, StringView dbgName = Default) = 0;

			virtual PipelinePackID		LoadPipelinePack (const SharedPtr<RStream> &stream) = 0;
			virtual bool				ReloadPipelinePack (const SharedPtr<RStream> &stream, PipelinePackID id) = 0;

		ND_ virtual GraphicsPipelineID	GetGraphicsPipeline (const PipelineName &name, const GraphicsPipelineDesc &desc) = 0;
		ND_ virtual MeshPipelineID		GetMeshPipeline (const PipelineName &name, const MeshPipelineDesc &desc) = 0;
		ND_ virtual ComputePipelineID	GetComputePipeline (const PipelineName &name, const ComputePipelineDesc &desc) = 0;
		ND_ virtual RayTracingPipelineID GetRayTracingPipeline (const PipelineName &name, const RayTracingPipelineDesc &desc) = 0;
		
			virtual bool				LoadSamplerPack (const SharedPtr<RStream> &stream) = 0;
		//ND_ virtual SamplerID			GetSampler (const SamplerName &name) = 0;

			virtual bool				ReleaseResource (GfxResourceID id) = 0;
			virtual bool				ReleaseResource (MemoryID id) = 0;
			virtual bool				ReleaseResource (PipelinePackID id) = 0;
	};


}	// AE::Graphics
