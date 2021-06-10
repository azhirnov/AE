// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

# include "graphics/Public/CommandBufferTypes.h"
# include "graphics/Public/VulkanTypes.h"


namespace AE::Graphics
{

	//
	// Draw Context interface
	//

	class IDrawContext
	{
	// interface
	public:
		// reset graphics pipeline, descriptor sets, push constants and all dynamic render states.
		virtual void  ResetStates () = 0;

		// pipeline and shader resources
		virtual void  BindPipeline (GraphicsPipelineID ppln) = 0;
		virtual void  BindPipeline (MeshPipelineID ppln) = 0;
		virtual void  BindDescriptorSet (uint index, DescriptorSetID ds, ArrayView<uint> dynamicOffsets = Default) = 0;
		virtual void  PushConstant (Bytes offset, Bytes size, const void *values, EShaderStages stages = EShaderStages::AllGraphics) = 0;
		
		// dynamic states
		virtual void  SetScissor (uint first, ArrayView<RectI> scissors) = 0;
		virtual void  SetDepthBias (float depthBiasConstantFactor, float depthBiasClamp, float depthBiasSlopeFactor) = 0;
		virtual void  SetDepthBounds (float minDepthBounds, float maxDepthBounds) = 0;
		virtual void  SetStencilCompareMask (EStencilFace faceMask, uint compareMask) = 0;
		virtual void  SetStencilWriteMask (EStencilFace faceMask, uint writeMask) = 0;
		virtual void  SetStencilReference (EStencilFace faceMask, uint reference) = 0;
		virtual void  SetBlendConstants (const RGBA32f &color) = 0;

		// draw commands
		virtual void  BindIndexBuffer (BufferID buffer, Bytes offset, EIndex indexType) = 0;
		virtual void  BindVertexBuffer (uint index, BufferID buffer, Bytes offset) = 0;
		virtual void  BindVertexBuffers (uint firstBinding, ArrayView<Pair<BufferID, Bytes>> bufferAndOffset) = 0;

		virtual void  Draw (uint vertexCount,
							uint instanceCount = 1,
							uint firstVertex   = 0,
							uint firstInstance = 0) = 0;

		virtual void  DrawIndexed (uint indexCount,
								   uint instanceCount = 1,
								   uint firstIndex    = 0,
								   int  vertexOffset  = 0,
								   uint firstInstance = 0) = 0;

		virtual void  DrawIndirect (BufferID	indirectBuffer,
									Bytes		indirectBufferOffset,
									uint		drawCount,
									Bytes		stride) = 0;

		virtual void  DrawIndexedIndirect (BufferID		indirectBuffer,
										   Bytes		indirectBufferOffset,
										   uint			drawCount,
										   Bytes		stride) = 0;
	};

	

	//
	// Transfer Context interface
	//

	class ITransferContext
	{
	// interface
	public:
		// by default initial state is top_of_pipe stage and default image layout,
		// you can override this states using this function.
		virtual void  SetInitialState (BufferID id, EResourceState state) = 0;
		virtual void  SetInitialState (ImageID id, EResourceState state) = 0;
		virtual void  SetInitialState (ImageID id, EResourceState state, EImageLayout layout) = 0;
		
		virtual void  ClearColorImage (ImageID image, const ClearColor_t &color, ArrayView<ImageSubresourceRange> ranges) = 0;
		virtual void  ClearDepthStencilImage (ImageID image, const DepthStencil &depthStencil, ArrayView<ImageSubresourceRange> ranges) = 0;

		virtual void  FillBuffer (BufferID buffer, Bytes offset, Bytes size, uint data) = 0;
		virtual void  UpdateBuffer (BufferID buffer, Bytes offset, Bytes size, const void* data) = 0;
		
		virtual void  CopyBuffer (BufferID srcBuffer, BufferID dstBuffer, ArrayView<BufferCopy> ranges) = 0;
		virtual void  CopyImage (ImageID srcImage, ImageID dstImage, ArrayView<ImageCopy> ranges) = 0;
		virtual void  CopyBufferToImage (BufferID srcBuffer, ImageID dstImage, ArrayView<BufferImageCopy> ranges) = 0;
		virtual void  CopyImageToBuffer (ImageID srcImage, BufferID dstBuffer, ArrayView<BufferImageCopy> ranges) = 0;
		
		// write to device local memory from host
		virtual bool  UploadBuffer (BufferID buffer, Bytes offset, Bytes size, const void* data, EStagingHeapType heapType = EStagingHeapType::Static) = 0;
		virtual bool  UploadBuffer (BufferID buffer, Bytes offset, Bytes size, OUT BufferView &ranges, EStagingHeapType heapType = EStagingHeapType::Static) = 0;
		//virtual bool  UploadImage (ImageID image, const uint3 &offset, const uint3 &size, MipmapLevel mipLevel, ImageLayer arrayLayer, EImageAspect aspectMask,
		//						   OUT ImageView &ranges, EStagingHeapType heapType = EStagingHeapType::Static) = 0;

		// read from device local memory to host memory
		ND_ virtual Promise<BufferView>  ReadBuffer (BufferID buffer, Bytes offset, Bytes size, EStagingHeapType heapType = EStagingHeapType::Static) = 0;
		//ND_ virtual Promise<ImageView>   ReadImage (ImageID image, const uint3 &offset, const uint3 &size, MipmapLevel mipLevel,
		//											ImageLayer arrayLayer, EImageAspect aspectMask, EStagingHeapType heapType = EStagingHeapType::Static) = 0;

		// partially upload
		//virtual bool  UploadBuffer (BufferStream &stream, OUT BufferView &ranges, EStagingHeapType heapType = EStagingHeapType::Static) = 0;
		//virtual bool  UploadImage (ImageStream &stream, OUT ImageView &ranges, EStagingHeapType heapType = EStagingHeapType::Static) = 0;

		// partially read
		//ND_ virtual Promise<BufferView>  ReadBuffer (BufferStream &stream, EStagingHeapType heapType = EStagingHeapType::Static) = 0;
		//ND_ virtual Promise<ImageView>   ReadImage (ImageStream &stream, EStagingHeapType heapType = EStagingHeapType::Static) = 0;

		// only for host-visible memory
		virtual bool  UpdateHostBuffer (BufferID buffer, Bytes offset, Bytes size, const void* data) = 0;
		virtual bool  MapHostBuffer (BufferID buffer, Bytes offset, INOUT Bytes &size, OUT void* &mapped) = 0;
		
	public:
		template <typename T>
		void  UpdateBuffer (BufferID buffer, Bytes offset, ArrayView<T> data) {
			return UpdateBuffer( buffer, offset, ArraySizeOf(data), data.data() );
		}
		
		template <typename T>
		bool  UpdateHostBuffer (BufferID buffer, Bytes offset, ArrayView<T> data) {
			return UpdateHostBuffer( buffer, offset, ArraySizeOf(data), data.data() );
		}
	};



	//
	// Compute Context interface
	//

	class IComputeContext : public ITransferContext
	{
	// interface
	public:
		virtual void  BindPipeline (ComputePipelineID ppln) = 0;
		virtual void  DescriptorsBarrier (DescriptorSetID ds, ArrayView<uint> dynamicOffsets = Default) = 0;
		virtual void  BindDescriptorSet (uint index, DescriptorSetID ds, ArrayView<uint> dynamicOffsets = Default) = 0;
		virtual void  PushConstant (Bytes offset, Bytes size, const void *values, EShaderStages stages) = 0;

		virtual void  Dispatch (const uint3 &groupCount) = 0;
		virtual void  DispatchIndirect (BufferID buffer, Bytes offset) = 0;
		virtual void  DispatchBase (const uint3 &baseGroup, const uint3 &groupCount) = 0;

		// ray tracing
		//virtual void  RT_BindPipeline (RayTracingPipelineID ppln) = 0;
		//virtual void  RT_BindDescriptorSet (DescriptorSetID ds, ArrayView<uint> dynamicOffsets = Default) = 0;
		//virtual void  RT_PushConstant (Bytes offset, Bytes size, const void *values, EShaderStages stages) = 0;
		//virtual void  TraceRays (const uint3 dim) = 0;
		//virtual void  TraceRaysIndirect () = 0;
	};



	//
	// Graphics Context interface
	//

	class IGraphicsContext : public IComputeContext
	{
	// interface
	public:
		virtual void  BlitImage (ImageID srcImage, ImageID dstImage, EBlitFilter filter, ArrayView<ImageBlit> regions) = 0;
		virtual void  ResolveImage (ImageID srcImage, ImageID dstImage, ArrayView<ImageResolve> regions) = 0;

		//virtual void  BeginRenderPass (ArrayView<RenderPassDesc> passes) = 0;
		//virtual void  NextSubpass () = 0;
		//virtual void  EndRenderPass () = 0;
	};



}	// AE::Graphics
