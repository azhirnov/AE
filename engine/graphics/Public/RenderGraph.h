// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "graphics/Public/Queue.h"
#include "graphics/Public/ImageDesc.h"
#include "graphics/Public/BufferDesc.h"
#include "graphics/Public/IDs.h"
#include "graphics/Public/ResourceEnums.h"
#include "graphics/Public/ShaderEnums.h"

namespace AE::Graphics
{

	//
	// Render Context interface
	//

	class IRenderContext
	{
	// types
	public:
		struct VulkanContext
		{
			uint64_t	cmdBuffer		= 0;
			uint64_t	renderPass		= 0;
			uint64_t	framebuffer		= 0;
			uint32_t	subpassIndex	= 0;
		};

		using NativeContext_t = Union< VulkanContext >;


	// interface
	public:
		ND_ virtual NativeContext_t  GetNativeContext () = 0;

		// reset graphics pipeline, descriptor sets, push constants and all dynamic render states.
		virtual void ResetStates () = 0;

		// pipeline and shader resources
		virtual void BindPipeline (GraphicsPipelineID ppln) = 0;
		virtual void BindPipeline (MeshPipelineID ppln) = 0;
		virtual void BindDescriptorSet (uint index, DescriptorSetID ds, ArrayView<uint> dynamicOffsets) = 0;
		virtual void PushConstant (BytesU offset, BytesU size, const void *values, EShaderStages stages = EShaderStages::AllGraphics) = 0;
		/*
		// dynamic states
		virtual void SetScissor (uint first, ArrayView<RectI> scissors) = 0;
		virtual void SetViewport () = 0;
		virtual void SetDepthBias () = 0;
		virtual void SetLineWidth () = 0;
		virtual void SetDepthBounds () = 0;
		virtual void SetStencilCompareMask () = 0;
		virtual void SetStencilWriteMask () = 0;
		virtual void SetStencilReference () = 0;
		virtual void SetBlendConstants () = 0;

		// dynamic state extensions
		virtual void SetSampleLocationsEXT () = 0;
		virtual void SetDiscardRectangleEXT () = 0;
		virtual void SetViewportWScalingNV () = 0;
		virtual void SetExclusiveScissorNV () = 0;

		// shading rate image extension
		virtual void SetViewportShadingRatePaletteNV () = 0;
		virtual void BindShadingRateImageNV () = 0;
		virtual void SetCoarseSampleOrderNV () = 0;
		*/
		// draw commands
		virtual void BindIndexBuffer () = 0;
		virtual void BindVertexBuffer () = 0;

		virtual void Draw (uint vertexCount,
						   uint instanceCount = 1,
						   uint firstVertex   = 0,
						   uint firstInstance = 0) = 0;

		virtual void DrawIndexed (uint indexCount,
								  uint instanceCount = 1,
								  uint firstIndex    = 0,
								  int  vertexOffset  = 0,
								  uint firstInstance = 0) = 0;

		virtual void DrawIndirect (GfxResourceID buffer,
								   BytesU		 offset,
								   uint			 drawCount,
								   BytesU		 stride) = 0;

		virtual void DrawIndexedIndirect (GfxResourceID	buffer,
										  BytesU		offset,
										  uint			drawCount,
										  BytesU		stride) = 0;

		// extension
		virtual void DrawIndirectCount (GfxResourceID	buffer,
										BytesU			offset,
										GfxResourceID	countBuffer,
										BytesU			countBufferOffset,
										uint			maxDrawCount,
										BytesU			stride) = 0;

		virtual void DrawIndexedIndirectCount (GfxResourceID	buffer,
											   BytesU			offset,
											   GfxResourceID	countBuffer,
											   BytesU			countBufferOffset,
											   uint				maxDrawCount,
											   BytesU			stride) = 0;

		// mesh draw commands (extension)
		virtual void DrawMeshTasksNV (uint taskCount, uint firstTask = 0) = 0;

		virtual void DrawMeshTasksIndirectNV (GfxResourceID	buffer,
											  BytesU		offset,
											  uint			drawCount,
											  BytesU		stride) = 0;

		virtual void DrawMeshTasksIndirectCountNV (GfxResourceID	buffer,
												   BytesU			offset,
												   GfxResourceID	countBuffer,
												   BytesU			countBufferOffset,
												   uint				maxDrawCount,
												   BytesU			stride) = 0;
		
		// TODO: debug draw command
	};


	//
	// Transfer Context
	//

	class ITransferContext
	{
	// types
	public:
		struct VulkanContext
		{
			uint64_t	cmdBuffer	= 0;
		};

		using NativeContext_t = Union< VulkanContext >;


	// interface
	public:
		/*
		ND_ virtual NativeContext_t  GetNativeContext () = 0;

		// if disabled context will commit all pending barriers and stop tracking for new barriers.
		// if enabled context starts tracking for all resources
		virtual void EnableAutoBarriers (bool enable) = 0;

		virtual void ImageBarrier () = 0;
		virtual void BufferBarrier () = 0;
		
		ND_ virtual GfxResourceID   GetOutput (GfxResourceID id) = 0;
		virtual void SetOutput (GfxResourceID id, GfxResourceID res) = 0;
		
		virtual void ClearColorImage () = 0;
		virtual void ClearDepthStencilImage () = 0;
		virtual void FillBuffer () = 0;

		virtual void UpdateBuffer () = 0;

		virtual void CopyBuffer () = 0;
		virtual void CopyImage () = 0;
		virtual void CopyBufferToImage () = 0;
		virtual void CopyImageToBuffer () = 0;
		
		virtual void Present () = 0;*/
	};


	//
	// Compute Context
	//

	class IComputeContext : public ITransferContext
	{
	// interface
	public:
		/*virtual void BindPipeline () = 0;
		virtual void BindDescriptorSet () = 0;
		virtual void PushConstant () = 0;

		virtual void Dispatch () = 0;
		virtual void DispatchIndirect () = 0;
		// TODO: debug dispatch command
		
		// ray tracing
		virtual void BuildRayTracingGeometry () = 0;
		virtual void BuildRayTracingScene () = 0;
		virtual void UpdateShaderBindingTable () = 0;
		virtual void TraceRays () = 0;
		// TODO: debug ray tracing
		*/
	};


	//
	// Graphics Context
	//

	class IGraphicsContext : public IComputeContext
	{
	// interface
	public:
		/*virtual void BlitImage () = 0;
		virtual void ResolveColorImage () = 0;
		virtual void ResolveDepthImage () = 0;*/
	};
//-----------------------------------------------------------------------------



	//
	// Render Graph
	//

	class IRenderGraph
	{
	// types
	public:
		// on input - resolved resource, on output - virtual and should be resolved
		using RenderPassSetupFn_t	= Function<void (const IGraphicsContext &ctx, ArrayView<GfxResourceID> input, ArrayView<GfxResourceID> output/*, OUT RenderPassDesc &*/)>;
		using RenderPassDrawFn_t	= Function<void (const IRenderContext   &ctx, ArrayView<GfxResourceID> input, ArrayView<GfxResourceID> output)>;
		using GraphicsCommandFn_t	= Function<void (const IGraphicsContext &ctx, ArrayView<GfxResourceID> input, ArrayView<GfxResourceID> output)>;
		using ComputeCommandFn_t	= Function<void (const IComputeContext  &ctx, ArrayView<GfxResourceID> input, ArrayView<GfxResourceID> output)>;
		using TransferCommandFn_t	= Function<void (const ITransferContext &ctx, ArrayView<GfxResourceID> input, ArrayView<GfxResourceID> output)>;

		using VirtualResources_t	= ArrayView<Pair< GfxResourceID, EVirtualResourceUsage >>;


	// interface
	public:
		ND_ virtual EQueueMask  GetPresentQueues () = 0;

		virtual bool Add (EQueueType				queue,
						  VirtualResources_t		input,
						  VirtualResources_t		output,
						  RenderPassSetupFn_t&&		setup,
						  RenderPassDrawFn_t&&		draw,
						  StringView				dbgName = Default) = 0;

		virtual bool Add (EQueueType				queue,
						  VirtualResources_t		input,
						  VirtualResources_t		output,
						  GraphicsCommandFn_t&&		pass,
						  StringView				dbgName = Default) = 0;

		virtual bool Add (EQueueType				queue,
						  VirtualResources_t		input,
						  VirtualResources_t		output,
						  ComputeCommandFn_t&&		pass,
						  StringView				dbgName = Default) = 0;

		virtual bool Add (EQueueType				queue,
						  VirtualResources_t		input,
						  VirtualResources_t		output,
						  TransferCommandFn_t&&		pass,
						  StringView				dbgName = Default) = 0;

		virtual bool Flush () = 0;
	};


}	// AE::Graphics
