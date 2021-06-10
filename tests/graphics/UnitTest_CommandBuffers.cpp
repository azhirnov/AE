// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#include "graphics/Vulkan/VCommandBufferTypes.h"
#include "UnitTest_Common.h"

namespace
{
	template <typename CmdBufType, typename T>
	struct ToMethod {};

	template <typename CmdBufType, typename Result, typename ...Args>
	struct ToMethod< CmdBufType, Result (Args...) >
	{
		using type = Result (CmdBufType::*) (Args...);
	};

	#define CHECK_METHOD( _fn_, ... )	Unused( static_cast< typename ToMethod<CmdBufType, __VA_ARGS__>::type >( &CmdBufType::_fn_ ))


	template <typename CmdBufType>
	static constexpr void  CheckTransferContext ()
	{
		STATIC_ASSERT( CmdBufType::IsTransferContext );
		CHECK_METHOD( ImageBarrier,				void (ImageID image, EResourceState srcState, EResourceState dstState));
		CHECK_METHOD( ImageBarrier,				void (ImageID image, EResourceState srcState, EResourceState dstState, const ImageSubresourceRange &subRes));
		CHECK_METHOD( BufferBarrier,			void (BufferID buffer, EResourceState srcState, EResourceState dstState));
		CHECK_METHOD( BufferBarrier,			void (BufferID buffer, EResourceState srcState, EResourceState dstState, Bytes offset, Bytes size));
		CHECK_METHOD( SetInitialState,			void (BufferID id, EResourceState state));
		CHECK_METHOD( SetInitialState,			void (ImageID id, EResourceState state));
		CHECK_METHOD( SetInitialState,			void (ImageID id, EResourceState state, EImageLayout layout));
		CHECK_METHOD( ClearColorImage,			void (ImageID image, const ClearColor_t &color, ArrayView<ImageSubresourceRange> ranges));
		CHECK_METHOD( ClearDepthStencilImage,	void (ImageID image, const DepthStencil &depthStencil, ArrayView<ImageSubresourceRange> ranges));
		CHECK_METHOD( FillBuffer,				void (BufferID buffer, Bytes offset, Bytes size, uint data));
		CHECK_METHOD( UpdateBuffer,				void (BufferID buffer, Bytes offset, Bytes size, const void* data));
		CHECK_METHOD( CopyBuffer,				void (BufferID srcBuffer, BufferID dstBuffer, ArrayView<BufferCopy> ranges));
		CHECK_METHOD( CopyImage,				void (ImageID srcImage, ImageID dstImage, ArrayView<ImageCopy> ranges));
		CHECK_METHOD( CopyBufferToImage,		void (BufferID srcBuffer, ImageID dstImage, ArrayView<BufferImageCopy> ranges));
		CHECK_METHOD( CopyImageToBuffer,		void (ImageID srcImage, BufferID dstBuffer, ArrayView<BufferImageCopy> ranges));
		CHECK_METHOD( GlobalBarrier,			void ());
		CHECK_METHOD( DebugMarker,				void (StringView text, RGBA8u color));
		CHECK_METHOD( PushDebugGroup,			void (StringView text, RGBA8u color));
		CHECK_METHOD( PopDebugGroup,			void ());
		CHECK_METHOD( CommitBarriers,			void ());
	}
	

	template <typename CmdBufType>
	static constexpr void  CheckComputeContext ()
	{
		CheckTransferContext<CmdBufType>();
		STATIC_ASSERT( CmdBufType::IsComputeContext );
		CHECK_METHOD( BindPipeline,				void (ComputePipelineID ppln));
		CHECK_METHOD( DescriptorsBarrier,		void (DescriptorSetID ds, ArrayView<uint> dynamicOffsets));
		CHECK_METHOD( BindDescriptorSet,		void (uint index, DescriptorSetID ds, ArrayView<uint> dynamicOffsets));
		CHECK_METHOD( PushConstant,				void (Bytes offset, Bytes size, const void *values, EShaderStages stages));
		CHECK_METHOD( Dispatch,					void (const uint3 &groupCount));
		CHECK_METHOD( DispatchIndirect,			void (BufferID buffer, Bytes offset));
		CHECK_METHOD( DispatchBase,				void (const uint3 &baseGroup, const uint3 &groupCount));
	}
	

	template <typename CmdBufType>
	static constexpr void  CheckGraphicsContext ()
	{
		CheckComputeContext<CmdBufType>();
		STATIC_ASSERT( CmdBufType::IsGraphicsContext );
		CHECK_METHOD( BlitImage,				void (ImageID srcImage, ImageID dstImage, EBlitFilter filter, ArrayView<ImageBlit> regions));
		CHECK_METHOD( ResolveImage,				void (ImageID srcImage, ImageID dstImage, ArrayView<ImageResolve> regions));
	}


	template <typename CmdBufType>
	static constexpr void  CheckRenderContext ()
	{
		STATIC_ASSERT( CmdBufType::IsRenderContext );
		CHECK_METHOD( BindPipeline,				void (GraphicsPipelineID ppln));
		CHECK_METHOD( BindPipeline,				void (MeshPipelineID ppln));
		CHECK_METHOD( BindDescriptorSet,		void (uint index, DescriptorSetID ds, ArrayView<uint> dynamicOffsets));
		CHECK_METHOD( PushConstant,				void (Bytes offset, Bytes size, const void *values, EShaderStages stages));
		CHECK_METHOD( SetScissor,				void (uint first, ArrayView<RectI> scissors));
		CHECK_METHOD( SetDepthBias,				void (float depthBiasConstantFactor, float depthBiasClamp, float depthBiasSlopeFactor));
		CHECK_METHOD( SetDepthBounds,			void (float minDepthBounds, float maxDepthBounds));
		CHECK_METHOD( SetStencilCompareMask,	void (EStencilFace faceMask, uint compareMask));
		CHECK_METHOD( SetStencilWriteMask,		void (EStencilFace faceMask, uint writeMask));
		CHECK_METHOD( SetStencilReference,		void (EStencilFace faceMask, uint reference));
		CHECK_METHOD( SetBlendConstants,		void (const RGBA32f &color));
		CHECK_METHOD( BindIndexBuffer,			void (BufferID buffer, Bytes offset, EIndex indexType));
		CHECK_METHOD( BindVertexBuffer,			void (uint index, BufferID buffer, Bytes offset));
		CHECK_METHOD( BindVertexBuffers,		void (uint firstBinding, ArrayView<Pair<BufferID, Bytes>> bufferAndOffset));
		CHECK_METHOD( Draw,						void (uint vertexCount, uint instanceCount, uint firstVertex, uint firstInstance));
		CHECK_METHOD( DrawIndexed,				void (uint indexCount, uint instanceCount, uint firstIndex, int vertexOffset, uint firstInstance));
		CHECK_METHOD( DrawIndirect,				void (BufferID indirectBuffer, Bytes indirectBufferOffset, uint drawCount, Bytes stride));
		CHECK_METHOD( DrawIndexedIndirect,		void (BufferID indirectBuffer, Bytes indirectBufferOffset, uint drawCount, Bytes stride));
		CHECK_METHOD( DrawIndirectCount,		void (BufferID indirectBuffer, Bytes indirectBufferOffset, BufferID countBuffer, Bytes countBufferOffset, uint maxDrawCount, Bytes stride));
		CHECK_METHOD( DrawIndexedIndirectCount,	void (BufferID indirectBuffer, Bytes indirectBufferOffset, BufferID countBuffer, Bytes countBufferOffset, uint maxDrawCount, Bytes stride));
		CHECK_METHOD( DrawMeshTasksNV,			void (uint taskCount, uint firstTask));
		CHECK_METHOD( DrawMeshTasksIndirectNV,	void (BufferID indirectBuffer, Bytes indirectBufferOffset, uint drawCount, Bytes stride));
		CHECK_METHOD( DrawMeshTasksIndirectCountNV, void (BufferID indirectBuffer, Bytes indirectBufferOffset, BufferID countBuffer, Bytes countBufferOffset, uint maxDrawCount, Bytes stride));
	}
}


#ifdef AE_ENABLE_VULKAN
namespace
{
	template <typename CmdBufType>
	static constexpr void  CheckVulkanTransferContext ()
	{
		CheckTransferContext<CmdBufType>();
		STATIC_ASSERT( CmdBufType::IsVulkanTransferContext );
		CHECK_METHOD( ImageBarrier,				void (VkPipelineStageFlagBits srcStage, VkPipelineStageFlagBits dstStage, const VkImageMemoryBarrier &barrier));
		CHECK_METHOD( BufferBarrier,			void (VkPipelineStageFlagBits srcStage, VkPipelineStageFlagBits dstStage, const VkBufferMemoryBarrier &barrier));
		CHECK_METHOD( ClearColorImage,			void (VkImage image, const VkClearColorValue &color, ArrayView<VkImageSubresourceRange> ranges));
		CHECK_METHOD( ClearDepthStencilImage,	void (VkImage image, const VkClearDepthStencilValue &depthStencil, ArrayView<VkImageSubresourceRange> ranges));
		CHECK_METHOD( FillBuffer,				void (VkBuffer buffer, Bytes offset, Bytes size, uint data));
		CHECK_METHOD( UpdateBuffer,				void (VkBuffer buffer, Bytes offset, Bytes size, const void* data));
		CHECK_METHOD( CopyBuffer,				void (VkBuffer srcBuffer, VkBuffer dstBuffer, ArrayView<VkBufferCopy> ranges));
		CHECK_METHOD( CopyImage,				void (VkImage srcImage, VkImage dstImage, ArrayView<VkImageCopy> ranges));
		CHECK_METHOD( CopyBufferToImage,		void (VkBuffer srcBuffer, VkImage dstImage, ArrayView<VkBufferImageCopy> ranges));
		CHECK_METHOD( CopyImageToBuffer,		void (VkImage srcImage, VkBuffer dstBuffer, ArrayView<VkBufferImageCopy> ranges));
		//CHECK_METHOD( IsValid,				bool ());

		if constexpr( CmdBufType::IsIndirectContext )
		{
			CHECK_METHOD( Prepare,				VBakedCommands ());
		}
		else
		{
			CHECK_METHOD( ReleaseCommandBuffer,	VkCommandBuffer ());
		}
	}


	template <typename CmdBufType>
	static constexpr void  CheckVulkanComputeContext ()
	{
		CheckComputeContext<CmdBufType>();
		CheckVulkanTransferContext<CmdBufType>();
		STATIC_ASSERT( CmdBufType::IsVulkanComputeContext );
		CHECK_METHOD( BindDescriptorSet,		void (uint index, VkDescriptorSet ds, ArrayView<uint> dynamicOffsets));
		CHECK_METHOD( DispatchIndirect,			void (VkBuffer buffer, Bytes offset));
	}


	template <typename CmdBufType>
	static constexpr void  CheckVulkanGraphicsContext ()
	{
		CheckGraphicsContext<CmdBufType>();
		CheckVulkanComputeContext<CmdBufType>();
		STATIC_ASSERT( CmdBufType::IsVulkanGraphicsContext );
		CHECK_METHOD( BlitImage,				void (VkImage srcImage, VkImage dstImage, VkFilter filter, ArrayView<VkImageBlit> regions));
		CHECK_METHOD( ResolveImage,				void (VkImage srcImage, VkImage dstImage, ArrayView<VkImageResolve> regions));
	}


	template <typename CmdBufType>
	static constexpr void  CheckVulkanRenderContext ()
	{
		CheckRenderContext<CmdBufType>();
		STATIC_ASSERT( CmdBufType::IsVulkanRenderContext );
		CHECK_METHOD( BindDescriptorSet,		void (uint index, VkDescriptorSet ds, ArrayView<uint> dynamicOffsets));
		CHECK_METHOD( BindIndexBuffer,			void (VkBuffer buffer, Bytes offset, VkIndexType indexType));
		CHECK_METHOD( BindVertexBuffer,			void (uint index, VkBuffer buffer, Bytes offset));
		CHECK_METHOD( BindVertexBuffers,		void (uint firstBinding, ArrayView<Pair<VkBuffer, Bytes>> bufferAndOffset));
		CHECK_METHOD( Draw,						void (uint vertexCount, uint instanceCount, uint firstVertex, uint firstInstance));
		CHECK_METHOD( DrawIndexed,				void (uint indexCount, uint instanceCount, uint firstIndex, int vertexOffset, uint firstInstance));
		CHECK_METHOD( DrawIndirect,				void (VkBuffer indirectBuffer, Bytes indirectBufferOffset, uint drawCount, Bytes stride));
		CHECK_METHOD( DrawIndexedIndirect,		void (VkBuffer indirectBuffer, Bytes indirectBufferOffset, uint drawCount, Bytes stride));
		CHECK_METHOD( DrawIndirectCount,		void (VkBuffer indirectBuffer, Bytes indirectBufferOffset, VkBuffer countBuffer, Bytes countBufferOffset, uint maxDrawCount, Bytes stride));
		CHECK_METHOD( DrawIndexedIndirectCount,	void (VkBuffer indirectBuffer, Bytes indirectBufferOffset, VkBuffer countBuffer, Bytes countBufferOffset, uint maxDrawCount, Bytes stride));
		CHECK_METHOD( DrawMeshTasksNV,			void (uint taskCount, uint firstTask));
		CHECK_METHOD( DrawMeshTasksIndirectNV,	void (VkBuffer indirectBuffer, Bytes indirectBufferOffset, uint drawCount, Bytes stride));
		CHECK_METHOD( DrawMeshTasksIndirectCountNV, void (VkBuffer indirectBuffer, Bytes indirectBufferOffset, VkBuffer countBuffer, Bytes countBufferOffset, uint maxDrawCount, Bytes stride));
	}
}
#endif


extern void UnitTest_CommandBuffers ()
{
	#ifdef AE_ENABLE_VULKAN
	CheckVulkanTransferContext< VDirectTransferContextNoBarriers >();
	CheckVulkanTransferContext< VDirectTransferContextWithBarriers >();
	CheckVulkanTransferContext< VDirectTransferContextWithRangeBarriers >();
	
	CheckVulkanComputeContext< VDirectComputeContextNoBarriers >();
	CheckVulkanComputeContext< VDirectComputeContextWithBarriers >();
	CheckVulkanComputeContext< VDirectComputeContextWithRangeBarriers >();
	
	CheckVulkanGraphicsContext< VDirectGraphicsContextNoBarriers >();
	CheckVulkanGraphicsContext< VDirectGraphicsContextWithBarriers >();
	CheckVulkanGraphicsContext< VDirectGraphicsContextWithRangeBarriers >();
	
	CheckVulkanRenderContext< VDirectRenderContextNoBarriers >();
	//CheckVulkanRenderContext< VDirectRenderContextWithBarriers >();
	//CheckVulkanRenderContext< VDirectRenderContextWithRangeBarriers >();
	
	CheckVulkanRenderContext< VIndirectRenderContextNoBarriers >();
	#endif

	AE_LOGI( "UnitTest_CommandBuffers - passed" );
}
