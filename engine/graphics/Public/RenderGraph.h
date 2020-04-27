// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'
/*
	IRenderContext, ITransferContext, IComputeContext, IGraphicsContext - is wrappers for command buffer (or emulation).
	Use they in single thread or in multiple threads with sync primitives.

	IRenderGraph - is wrapper for command queues.
	All methods are thread safe.
*/

#pragma once

#include "graphics/Public/Queue.h"
#include "graphics/Public/ImageDesc.h"
#include "graphics/Public/BufferDesc.h"
#include "graphics/Public/PipelineDesc.h"
#include "graphics/Public/RenderPassDesc.h"
#include "graphics/Public/IDs.h"
#include "graphics/Public/EResourceState.h"
#include "graphics/Public/ResourceEnums.h"
#include "graphics/Public/RenderStateEnums.h"
#include "graphics/Public/ShaderEnums.h"
#include "graphics/Public/VertexEnums.h"
#include "graphics/Public/BufferView.h"
#include "graphics/Public/ImageView.h"
#include "graphics/Public/NativeTypes.OpenGL.h"
#include "graphics/Public/NativeTypes.Vulkan.h"
#include "stl/CompileTime/FunctionInfo.h"

namespace AE::Graphics
{
	class ISwapchain
	{
	public:
		virtual ~ISwapchain () {}
	};


	//
	// Render Context interface
	//

	class IRenderContext
	{
	// types
	public:
		using NativeContext_t = Union< VulkanRenderContext >;

		struct RenderContextInfo
		{
			RenderPassID	renderPass;
			uint			subpassIndex	= 0;
			uint			layerCount		= 0;	// same as viewportCount for pipeline description
			CmdBatchID		batchId;
		};


	// interface
	public:
		ND_ virtual NativeContext_t   GetNativeContext () = 0;
		ND_ virtual RenderContextInfo GetContextInfo () = 0;

		// reset graphics pipeline, descriptor sets, push constants and all dynamic render states.
		virtual void  ResetStates () = 0;

		// pipeline and shader resources
		virtual void  BindPipeline (GraphicsPipelineID ppln) = 0;
		virtual void  BindPipeline (MeshPipelineID ppln) = 0;
		virtual void  BindDescriptorSet (uint index, DescriptorSetID ds, ArrayView<uint> dynamicOffsets = Default) = 0;
		virtual void  PushConstant (BytesU offset, BytesU size, const void *values, EShaderStages stages = EShaderStages::AllGraphics) = 0;
		
		// dynamic states
		virtual void  SetScissor (uint first, ArrayView<RectI> scissors) = 0;
		//virtual void  SetViewport () = 0;
		virtual void  SetDepthBias (float depthBiasConstantFactor, float depthBiasClamp, float depthBiasSlopeFactor) = 0;
		virtual void  SetLineWidth (float lineWidth) = 0;
		virtual void  SetDepthBounds (float minDepthBounds, float maxDepthBounds) = 0;
		virtual void  SetStencilCompareMask (EStencilFace faceMask, uint compareMask) = 0;
		virtual void  SetStencilWriteMask (EStencilFace faceMask, uint writeMask) = 0;
		virtual void  SetStencilReference (EStencilFace faceMask, uint reference) = 0;
		virtual void  SetBlendConstants (const RGBA32f &color) = 0;
		/*
		// dynamic state extensions
		virtual void  SetSampleLocationsEXT () = 0;
		virtual void  SetDiscardRectangleEXT () = 0;
		virtual void  SetViewportWScalingNV () = 0;
		virtual void  SetExclusiveScissorNV () = 0;

		// shading rate image extension
		virtual void  SetViewportShadingRatePaletteNV () = 0;
		virtual void  BindShadingRateImageNV () = 0;
		virtual void  SetCoarseSampleOrderNV () = 0;
		*/
		// draw commands
		virtual void  BindIndexBuffer (GfxResourceID buffer, BytesU offset, EIndex indexType) = 0;
		virtual void  BindVertexBuffer (uint index, GfxResourceID buffer, BytesU offset) = 0;
		virtual void  BindVertexBuffers (uint firstBinding, ArrayView<Pair<GfxResourceID, BytesU>> bufferAndOffset) = 0;

		virtual void  Draw (uint vertexCount,
							uint instanceCount = 1,
							uint firstVertex   = 0,
							uint firstInstance = 0) = 0;

		virtual void  DrawIndexed (uint indexCount,
								   uint instanceCount = 1,
								   uint firstIndex    = 0,
								   int  vertexOffset  = 0,
								   uint firstInstance = 0) = 0;

		virtual void  DrawIndirect (GfxResourceID buffer,
									BytesU		  offset,
									uint		  drawCount,
									BytesU		  stride) = 0;

		virtual void  DrawIndexedIndirect (GfxResourceID	buffer,
										   BytesU			offset,
										   uint				drawCount,
										   BytesU			stride) = 0;

		// extension
		virtual void  DrawIndirectCount (GfxResourceID	buffer,
										 BytesU			offset,
										 GfxResourceID	countBuffer,
										 BytesU			countBufferOffset,
										 uint			maxDrawCount,
										 BytesU			stride) = 0;

		virtual void  DrawIndexedIndirectCount (GfxResourceID	buffer,
												BytesU			offset,
												GfxResourceID	countBuffer,
												BytesU			countBufferOffset,
												uint			maxDrawCount,
												BytesU			stride) = 0;

		// mesh draw commands (extension)
		virtual void  DrawMeshTasksNV (uint taskCount, uint firstTask = 0) = 0;

		virtual void  DrawMeshTasksIndirectNV (GfxResourceID	buffer,
											   BytesU			offset,
											   uint				drawCount,
											   BytesU			stride) = 0;

		virtual void  DrawMeshTasksIndirectCountNV (GfxResourceID	buffer,
													BytesU			offset,
													GfxResourceID	countBuffer,
													BytesU			countBufferOffset,
													uint			maxDrawCount,
													BytesU			stride) = 0;
		
		// TODO: debug draw command
	};
	


	//
	// Async Render Context interface
	//

	class IAsyncRenderContext : public IRenderContext
	{
	// interface
	public:
		// you can save command buffer and reuse it at next invocation
		ND_ virtual UniqueID<BakedCommandBufferID>  CacheCommandBuffer () = 0;

		// try to reuse command buffer, returns 'false' on error
		ND_ virtual bool  ReuseCommandBuffer (const BakedCommandBufferID &) = 0;
		
		// start recording new command buffer.
		// (instead of using 'ReuseCommandBuffer' or if 'ReuseCommandBuffer' was failed)
		virtual bool  BeginCommandBuffer () = 0;

		// render commands recorded to secondary command buffer and will be executed later,
		// so you can add pending barriers that will be executed before render pass
		virtual void  Barrier (ArrayView< Pair<GfxResourceID, EResourceState>>) = 0;
	};
	


	//
	// Direct Render Context interface
	//

	class IDirectRenderContext
	{
	// interface
	public:
		// pipeline and shader resources
		virtual void  BindPipeline (const PipelineName &name, const GraphicsPipelineDesc &desc) = 0;
		virtual void  BindPipeline (const PipelineName &name, const MeshPipelineDesc &desc) = 0;
		virtual void  BindDescriptorSet (uint index, DescriptorSetID ds, ArrayView<uint> dynamicOffsets = Default) = 0;
		virtual void  PushConstant (BytesU offset, BytesU size, const void *values, EShaderStages stages = EShaderStages::AllGraphics) = 0;

		// dynamic states
		virtual void  SetScissor (uint first, ArrayView<RectI> scissors) = 0;
		//virtual void  SetViewport () = 0;
		virtual void  SetDepthBias (float depthBiasConstantFactor, float depthBiasClamp, float depthBiasSlopeFactor) = 0;
		virtual void  SetLineWidth (float lineWidth) = 0;
		virtual void  SetDepthBounds (float minDepthBounds, float maxDepthBounds) = 0;
		virtual void  SetStencilCompareMask (EStencilFace faceMask, uint compareMask) = 0;
		virtual void  SetStencilWriteMask (EStencilFace faceMask, uint writeMask) = 0;
		virtual void  SetStencilReference (EStencilFace faceMask, uint reference) = 0;
		virtual void  SetBlendConstants (const RGBA32f &color) = 0;
		/*
		// dynamic state extensions
		virtual void  SetSampleLocationsEXT () = 0;
		virtual void  SetDiscardRectangleEXT () = 0;
		virtual void  SetViewportWScalingNV () = 0;
		virtual void  SetExclusiveScissorNV () = 0;

		// shading rate image extension
		virtual void  SetViewportShadingRatePaletteNV () = 0;
		virtual void  BindShadingRateImageNV () = 0;
		virtual void  SetCoarseSampleOrderNV () = 0;
		*/
		// draw commands
		virtual void  BindIndexBuffer (GfxResourceID buffer, BytesU offset, EIndex indexType) = 0;
		virtual void  BindVertexBuffer (uint index, GfxResourceID buffer, BytesU offset) = 0;
		virtual void  BindVertexBuffers (uint firstBinding, ArrayView<Pair<GfxResourceID, BytesU>> bufferAndOffset) = 0;

		virtual void  Draw (uint vertexCount,
							uint instanceCount = 1,
							uint firstVertex   = 0,
							uint firstInstance = 0) = 0;

		virtual void  DrawIndexed (uint indexCount,
								   uint instanceCount = 1,
								   uint firstIndex    = 0,
								   int  vertexOffset  = 0,
								   uint firstInstance = 0) = 0;

		virtual void  DrawIndirect (GfxResourceID buffer,
									BytesU		  offset,
									uint		  drawCount,
									BytesU		  stride) = 0;

		virtual void  DrawIndexedIndirect (GfxResourceID	buffer,
										   BytesU			offset,
										   uint				drawCount,
										   BytesU			stride) = 0;

		// extension
		virtual void  DrawIndirectCount (GfxResourceID	buffer,
										 BytesU			offset,
										 GfxResourceID	countBuffer,
										 BytesU			countBufferOffset,
										 uint			maxDrawCount,
										 BytesU			stride) = 0;

		virtual void  DrawIndexedIndirectCount (GfxResourceID	buffer,
												BytesU			offset,
												GfxResourceID	countBuffer,
												BytesU			countBufferOffset,
												uint			maxDrawCount,
												BytesU			stride) = 0;

		// mesh draw commands (extension)
		virtual void  DrawMeshTasksNV (uint taskCount, uint firstTask = 0) = 0;

		virtual void  DrawMeshTasksIndirectNV (GfxResourceID	buffer,
											   BytesU			offset,
											   uint				drawCount,
											   BytesU			stride) = 0;

		virtual void  DrawMeshTasksIndirectCountNV (GfxResourceID	buffer,
													BytesU			offset,
													GfxResourceID	countBuffer,
													BytesU			countBufferOffset,
													uint			maxDrawCount,
													BytesU			stride) = 0;
	};


	//
	// Transfer Context
	//

	class ITransferContext
	{
	// types
	public:
		using NativeContext_t = Union< VulkanContext >;
		
		struct ImageSubresourceRange
		{
			EImageAspect	aspectMask		= Default;
			MipmapLevel		baseMipLevel;
			uint			levelCount		= 1;
			ImageLayer		baseArrayLayer;
			uint			layerCount		= 1;
		};

		struct ImageSubresourceLayers
		{
			EImageAspect	aspectMask		= Default;
			MipmapLevel		mipLevel;
			ImageLayer		baseArrayLayer;
			uint			layerCount		= 1;
		};

		using ClearColor_t	= Union< RGBA32f, RGBA32i, RGBA32u >;

		struct BufferCopy
		{
			BytesU			srcOffset;
			BytesU			dstOffset;
			BytesU			size;
		};
		
		struct ImageCopy
		{
			ImageSubresourceLayers	srcSubresource;
			uint3					srcOffset;
			ImageSubresourceLayers	dstSubresource;
			uint3					dstOffset;
			uint3					extent;
		};

		struct BufferImageCopy
		{
			BytesU					bufferOffset;
			uint					bufferRowLength;
			uint					bufferImageHeight;
			ImageSubresourceLayers	imageSubresource;
			uint3					imageOffset;
			uint3					imageExtent;
		};
		
		struct ContextInfo
		{
			EQueueType		queue;
			CmdBatchID		batchId;
		};


	// interface
	public:
		ND_ virtual NativeContext_t  GetNativeContext () = 0;
		ND_ virtual ContextInfo		 GetContextInfo () = 0;
		
		// if disabled context will commit all pending barriers and stop tracking for new barriers.
		// if enabled context starts tracking for all resources
		//virtual void  EnableAutoBarriers (bool enable) = 0;

		virtual void  ImageBarrier (GfxResourceID image, EResourceState srcState, EResourceState dstState) = 0;
		virtual void  ImageBarrier (GfxResourceID image, EResourceState srcState, EResourceState dstState, const ImageSubresourceRange &subRes) = 0;

		virtual void  BufferBarrier (GfxResourceID buffer, EResourceState srcState, EResourceState dstState) = 0;
		virtual void  BufferBarrier (GfxResourceID buffer, EResourceState srcState, EResourceState dstState, BytesU offset, BytesU size) = 0;

		virtual void  DescriptorsBarrier (DescriptorSetID ds) = 0;

		// by default initial state is top_of_pipe stage and default image layout,
		// you can override this states using this function.
		virtual void  SetInitialState (GfxResourceID id, EResourceState state) = 0;
		virtual void  SetInitialState (GfxResourceID id, EResourceState state, EImageLayout layout) = 0;

		// for debugging only
		virtual void  GlobalBarrier () = 0;
		
		ND_ virtual GfxResourceID   GetOutput (GfxResourceID id) = 0;
		virtual bool  SetOutput (GfxResourceID id, GfxResourceID res) = 0;
		
		virtual void  ClearColorImage (GfxResourceID image, const ClearColor_t &color, ArrayView<ImageSubresourceRange> ranges) = 0;
		virtual void  ClearDepthStencilImage (GfxResourceID image, const DepthStencil &depthStencil, ArrayView<ImageSubresourceRange> ranges) = 0;
		virtual void  FillBuffer (GfxResourceID buffer, BytesU offset, BytesU size, uint data) = 0;

		virtual void  UpdateBuffer (GfxResourceID buffer, BytesU offset, BytesU size, const void* data) = 0;
		
		// update mapped memory
		virtual bool  UpdateHostBuffer (GfxResourceID buffer, BytesU offset, BytesU size, const void* data) = 0;
		virtual bool  MapHostBuffer (GfxResourceID buffer, BytesU offset, INOUT BytesU &size, OUT void* &mapped) = 0;

		// read from GPU memory using staging buffer
		virtual bool  ReadBuffer (GfxResourceID buffer, BytesU offset, BytesU size, Function<void (const BufferView &)>&& fn) = 0;
		virtual bool  ReadImage (GfxResourceID image, const uint3 &offset, const uint3 &size, MipmapLevel mipLevel,
								 ImageLayer arrayLayer, EImageAspect aspectMask, Function<void (const ImageView &)>&& fn) = 0;

		// upload data to GPU using staging buffer
		virtual bool  UploadBuffer (GfxResourceID buffer, BytesU offset, BytesU size, const void* data) = 0;
		virtual bool  UploadBuffer (GfxResourceID buffer, BytesU offset, BytesU size, OUT BufferView &ranges) = 0;
		virtual bool  UploadImage (GfxResourceID image, const uint3 &offset, const uint3 &size, MipmapLevel mipLevel,
								   ImageLayer arrayLayer, EImageAspect aspectMask, OUT ImageView &ranges) = 0; 
		
		// allocate space in the staging buffer
		virtual bool  AllocBuffer (BytesU size, BytesU alignment, OUT GfxResourceID &buffer, OUT BytesU &offset, OUT void* &mapped) = 0;

		virtual void  CopyBuffer (GfxResourceID srcBuffer, GfxResourceID dstBuffer, ArrayView<BufferCopy> ranges) = 0;
		virtual void  CopyImage (GfxResourceID srcImage, GfxResourceID dstImage, ArrayView<ImageCopy> ranges) = 0;
		virtual void  CopyBufferToImage (GfxResourceID srcBuffer, GfxResourceID dstImage, ArrayView<BufferImageCopy> ranges) = 0;
		virtual void  CopyImageToBuffer (GfxResourceID srcImage, GfxResourceID dstBuffer, ArrayView<BufferImageCopy> ranges) = 0;

		// acquire next swapchain image
		ND_ virtual GfxResourceID  AcquireImage (ISwapchain *swapchain) = 0;

		// present command may be unsupported on transfer queue, use 'GetPresentQueues()' to get supported queues.
		virtual bool  Present (ISwapchain *swapchain) = 0;


	public:
		template <typename T>
		bool  AllocBuffer (BytesU size, BytesU alignment, OUT GfxResourceID &buffer, OUT BytesU &offset, OUT T* &mapped) {
			void*	ptr = null;
			bool	res = AllocBuffer( size, alignment, OUT buffer, OUT offset, OUT ptr );
			mapped = Cast<T>( ptr );
			return res;
		}

		template <typename T>
		void  UpdateBuffer (GfxResourceID buffer, BytesU offset, ArrayView<T> data) {
			return UpdateBuffer( buffer, offset, ArraySizeOf(data), data.data() );
		}
		
		template <typename T>
		bool  UpdateHostBuffer (GfxResourceID buffer, BytesU offset, ArrayView<T> data) {
			return UpdateHostBuffer( buffer, offset, ArraySizeOf(data), data.data() );
		}
	};


	//
	// Compute Context
	//

	class IComputeContext : public ITransferContext
	{
	// interface
	public:
		virtual void  BindPipeline (ComputePipelineID ppln) = 0;
		virtual void  BindDescriptorSet (uint index, DescriptorSetID ds, ArrayView<uint> dynamicOffsets = Default) = 0;
		virtual void  PushConstant (BytesU offset, BytesU size, const void *values, EShaderStages stages) = 0;

		virtual void  Dispatch (const uint3 &groupCount) = 0;
		virtual void  DispatchIndirect (GfxResourceID buffer, BytesU offset) = 0;
		virtual void  DispatchBase (const uint3 &baseGroup, const uint3 &groupCount) = 0;
		// TODO: debug dispatch command
		/*
		// ray tracing
		virtual void  BuildRayTracingGeometry () = 0;
		virtual void  BuildRayTracingScene () = 0;
		virtual void  UpdateShaderBindingTable () = 0;
		virtual void  TraceRays () = 0;
		// TODO: debug ray tracing
		*/
	};


	//
	// Graphics Context
	//

	class IGraphicsContext : public IComputeContext
	{
	// types
	public:
		struct ImageBlit
		{
			ImageSubresourceLayers	srcSubresource;
			uint3					srcOffset0;
			uint3					srcOffset1;
			ImageSubresourceLayers	dstSubresource;
			uint3					dstOffset0;
			uint3					dstOffset1;
		};

		struct ImageResolve
		{
			ImageSubresourceLayers	srcSubresource;
			uint3					srcOffset;
			ImageSubresourceLayers	dstSubresource;
			uint3					dstOffset;
			uint3					extent;
		};


	// interface
	public:
		virtual void  BlitImage (GfxResourceID srcImage, GfxResourceID dstImage, EBlitFilter filter, ArrayView<ImageBlit> regions) = 0;
		virtual void  ResolveImage (GfxResourceID srcImage, GfxResourceID dstImage, ArrayView<ImageResolve> regions) = 0;
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
		using RenderPassDrawFn_t	= Function<void (IRenderContext       &ctx, ArrayView<GfxResourceID> input, ArrayView<GfxResourceID> output)>;
		using AsyncDrawFn_t			= Function<void (IAsyncRenderContext  &ctx, ArrayView<GfxResourceID> input, ArrayView<GfxResourceID> output)>;	// TODO: add AsyncTask
		using DirectDrawFn_t		= Function<void (IDirectRenderContext &ctx, ArrayView<GfxResourceID> input, ArrayView<GfxResourceID> output)>;
		using GraphicsCommandFn_t	= Function<void (IGraphicsContext     &ctx, ArrayView<GfxResourceID> input, ArrayView<GfxResourceID> output)>;
		using ComputeCommandFn_t	= Function<void (IComputeContext      &ctx, ArrayView<GfxResourceID> input, ArrayView<GfxResourceID> output)>;
		using TransferCommandFn_t	= Function<void (ITransferContext     &ctx, ArrayView<GfxResourceID> input, ArrayView<GfxResourceID> output)>;

		

		using VirtualResources_t	= ArrayView<Pair< GfxResourceID, EVirtualResourceUsage >>;
		using GfxResources_t		= ArrayView< GfxResourceID >;


	// interface
	public:
		virtual ~IRenderGraph () {}

		ND_ virtual EQueueMask  GetPresentQueues () = 0;


	// direct commands //
		// All callbacks executed immediatlly, commands recorded to software command buffer.
		// Virtual resources are not sopported.
		/*
		virtual bool  Add (EQueueType				queue,
						   GfxResources_t			input,
						   GfxResources_t			output,
						   RenderPassDesc const&	rpDesc,
						   DirectDrawFn_t &&		draw,
						   StringView				dbgName) = 0;

		virtual bool  Add (EQueueType				queue,
						   GfxResources_t			input,
						   GfxResources_t			output,
						   GraphicsCommandFn_t &&	pass,
						   StringView				dbgName = Default) = 0;

		virtual bool  Add (EQueueType				queue,
						   GfxResources_t			input,
						   GfxResources_t			output,
						   ComputeCommandFn_t &&	pass,
						   StringView				dbgName = Default) = 0;

		virtual bool  Add (EQueueType				queue,
						   GfxResources_t			input,
						   GfxResources_t			output,
						   TransferCommandFn_t &&	pass,
						   StringView				dbgName = Default) = 0;
		*/


	// indirect commands //
		// All callbacks executed on 'Submit()' call after render graph building.
		template <typename FN>
		bool  Add (EQueueType				queue,
				   VirtualResources_t		input,
				   VirtualResources_t		output,
				   RenderPassDesc const&	rpDesc,
				   FN &&					draw,
				   StringView				dbgName = Default);

		virtual bool  Add (EQueueType				queue,
						   VirtualResources_t		input,
						   VirtualResources_t		output,
						   GraphicsCommandFn_t &&	pass,
						   StringView				dbgName = Default) = 0;

		virtual bool  Add (EQueueType				queue,
						   VirtualResources_t		input,
						   VirtualResources_t		output,
						   ComputeCommandFn_t &&	pass,
						   StringView				dbgName = Default) = 0;

		virtual bool  Add (EQueueType				queue,
						   VirtualResources_t		input,
						   VirtualResources_t		output,
						   TransferCommandFn_t &&	pass,
						   StringView				dbgName = Default) = 0;


		// wait on CPU side
		virtual bool  Wait (ArrayView<CmdBatchID> batches) = 0;
		virtual bool  WaitIdle () = 0;

		ND_ virtual bool  IsComplete (ArrayView<CmdBatchID> batches) = 0;

		virtual CmdBatchID  Submit () = 0;


	private:
		virtual bool  _AddSync (EQueueType				queue,
								VirtualResources_t		input,
								VirtualResources_t		output,
								RenderPassDesc const&	rpDesc,
								RenderPassDrawFn_t &&	draw,
								StringView				dbgName) = 0;
		
		virtual bool  _AddAsync (EQueueType				queue,
								 VirtualResources_t		input,
								 VirtualResources_t		output,
								 RenderPassDesc const&	rpDesc,
								 AsyncDrawFn_t &&		asyncDraw,
								 StringView				dbgName) = 0;
	};



	//
	// Cmd Batch Task Dependency
	//

	struct CmdBatchDep
	{
		CmdBatchID		id;
	};

	
/*
=================================================
	Add
=================================================
*/
	template <typename FN>
	inline bool  IRenderGraph::Add (EQueueType				queue,
									VirtualResources_t		input,
									VirtualResources_t		output,
									RenderPassDesc const&	rpDesc,
									FN &&					draw,
									StringView				dbgName)
	{
		using FI = FunctionInfo<FN>;

		STATIC_ASSERT( FI::args::Count == 3 );
		STATIC_ASSERT( IsSameTypes< typename FI::args::template Get<1>, ArrayView<GfxResourceID> >);
		STATIC_ASSERT( IsSameTypes< typename FI::args::template Get<2>, ArrayView<GfxResourceID> >);

		if constexpr( IsSameTypes< typename FI::args::template Get<0>, IRenderContext& >)
		{
			return _AddSync( queue, input, output, rpDesc, draw, dbgName );
		}
		else
		if constexpr( IsSameTypes< typename FI::args::template Get<0>, IAsyncRenderContext& >)
		{
			return _AddAsync( queue, input, output, rpDesc, draw, dbgName );
		}
	}


}	// AE::Graphics
