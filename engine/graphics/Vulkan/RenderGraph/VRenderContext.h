// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

namespace AE::Graphics
{
	
	//
	// Render Context
	//

	class VRenderGraph::RenderContext final : public VulkanDeviceFn, public IRenderContext
	{
	// variables
	private:
		VkCommandBuffer				_cmdbuf			= VK_NULL_HANDLE;

		GraphicsContext &			_graphicsCtx;
		VLogicalRenderPass const&	_logicalRP;

		// cached states
		VkPipelineLayout			_curLayout		= VK_NULL_HANDLE;
		VkPipeline					_curPipeline	= VK_NULL_HANDLE;

		VkBuffer					_indexBuffer	= VK_NULL_HANDLE;
		BytesU						_indexOffset;


	// methods
	public:
		RenderContext (GraphicsContext &gctx, const VLogicalRenderPass &logicalRP);

	// IRenderContext
		NativeContext_t    GetNativeContext () override;
		RenderContextInfo  GetContextInfo () override;

		void ResetStates () override;

		void BindPipeline (GraphicsPipelineID ppln) override;
		void BindPipeline (MeshPipelineID ppln) override;
		void BindDescriptorSet (uint index, DescriptorSetID ds, ArrayView<uint> dynamicOffsets) override;
		void PushConstant (BytesU offset, BytesU size, const void *values, EShaderStages stages) override;
		
		void SetScissor (uint first, ArrayView<RectI> scissors) override;
		void SetDepthBias (float depthBiasConstantFactor, float depthBiasClamp, float depthBiasSlopeFactor) override;
		void SetLineWidth (float lineWidth) override;
		void SetDepthBounds (float minDepthBounds, float maxDepthBounds) override;
		void SetStencilCompareMask (EStencilFace faceMask, uint compareMask) override;
		void SetStencilWriteMask (EStencilFace faceMask, uint writeMask) override;
		void SetStencilReference (EStencilFace faceMask, uint reference) override;
		void SetBlendConstants (const RGBA32f &color) override;

		void BindIndexBuffer (GfxResourceID buffer, BytesU offset, EIndex indexType) override;
		void BindVertexBuffer (uint index, GfxResourceID buffer, BytesU offset) override;
		void BindVertexBuffers (uint firstBinding, ArrayView<Pair<GfxResourceID, BytesU>> bufferAndOffset) override;

		void Draw (uint vertexCount, uint instanceCount, uint firstVertex, uint firstInstance) override;
		void DrawIndexed (uint indexCount, uint instanceCount, uint firstIndex, int vertexOffset, uint firstInstance) override;
		void DrawIndirect (GfxResourceID buffer, BytesU offset, uint drawCount, BytesU stride) override;
		void DrawIndexedIndirect (GfxResourceID buffer, BytesU offset, uint drawCount, BytesU stride) override;
		void DrawIndirectCount (GfxResourceID buffer, BytesU offset, GfxResourceID countBuffer, BytesU countBufferOffset, uint maxDrawCount, BytesU stride) override;
		void DrawIndexedIndirectCount (GfxResourceID buffer, BytesU offset, GfxResourceID countBuffer, BytesU countBufferOffset, uint maxDrawCount, BytesU stride) override;

		void DrawMeshTasksNV (uint taskCount, uint firstTask) override;
		void DrawMeshTasksIndirectNV (GfxResourceID buffer, BytesU offset, uint drawCount, BytesU stride) override;
		void DrawMeshTasksIndirectCountNV (GfxResourceID buffer, BytesU offset, GfxResourceID countBuffer, BytesU countBufferOffset, uint maxDrawCount, BytesU stride) override;
	};
//-----------------------------------------------------------------------------
	
/*
=================================================
	constructor
=================================================
*/
	VRenderGraph::RenderContext::RenderContext (GraphicsContext &gctx, const VLogicalRenderPass &logicalRP) :
		_cmdbuf{ gctx.GetCommandBuffer() },
		_graphicsCtx{ gctx }, _logicalRP{ logicalRP }
	{
		VulkanDeviceFn_Init( _graphicsCtx.GetDevice() );
	}
	
/*
=================================================
	GetNativeContext
=================================================
*/
	IRenderContext::NativeContext_t  VRenderGraph::RenderContext::GetNativeContext ()
	{
		auto*	render_pass	= _graphicsCtx.AcquireResource( _logicalRP.GetRenderPass() );
		auto*	framebuffer	= _graphicsCtx.AcquireResource( _logicalRP.GetFramebuffer() );
		CHECK_ERR( render_pass and framebuffer );

		VulkanRenderContext	vctx;
		vctx.cmdBuffer		= BitCast<CommandBufferVk_t>(_cmdbuf);
		vctx.renderPass		= BitCast<RenderPassVk_t>(render_pass->Handle());
		vctx.framebuffer	= BitCast<FramebufferVk_t>(framebuffer->Handle());
		vctx.subpassIndex	= _logicalRP.GetSubpassIndex();
		return vctx;
	}
		
/*
=================================================
	GetContextInfo
=================================================
*/
	IRenderContext::RenderContextInfo  VRenderGraph::RenderContext::GetContextInfo ()
	{
		RenderContextInfo	result;
		result.renderPass	= _logicalRP.GetRenderPass();
		result.subpassIndex	= _logicalRP.GetSubpassIndex();
		return result;
	}
	
/*
=================================================
	ResetStates
=================================================
*/
	void VRenderGraph::RenderContext::ResetStates ()
	{
		_curLayout		= VK_NULL_HANDLE;
		_curPipeline	= VK_NULL_HANDLE;

		_indexBuffer	= VK_NULL_HANDLE;
		_indexOffset	= 0_b;
	}
	
/*
=================================================
	BindPipeline
=================================================
*/
	void VRenderGraph::RenderContext::BindPipeline (GraphicsPipelineID ppln)
	{
		auto*	gppln = _graphicsCtx.AcquireResource( ppln );
		CHECK_ERR( gppln, void());

		if ( _curPipeline == gppln->Handle() )
			return;

		_curPipeline	= gppln->Handle();
		_curLayout		= gppln->Layout();
		vkCmdBindPipeline( _cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, _curPipeline );
	}
	
/*
=================================================
	BindPipeline
=================================================
*/
	void VRenderGraph::RenderContext::BindPipeline (MeshPipelineID ppln)
	{
		auto*	mppln = _graphicsCtx.AcquireResource( ppln );
		CHECK_ERR( mppln, void());

		if ( _curPipeline == mppln->Handle() )
			return;

		_curPipeline	= mppln->Handle();
		_curLayout		= mppln->Layout();
		vkCmdBindPipeline( _cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, _curPipeline );
	}
		
/*
=================================================
	BindDescriptorSet
=================================================
*/
	void VRenderGraph::RenderContext::BindDescriptorSet (uint index, DescriptorSetID ds, ArrayView<uint> dynamicOffsets)
	{
		// TODO
		//vkCmdBindDescriptorSets( _cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, _curLayout, index, 1, ds, uint(dynamicOffsets.size()), dynamicOffsets.data() );
	}
	
/*
=================================================
	PushConstant
=================================================
*/
	void VRenderGraph::RenderContext::PushConstant (BytesU offset, BytesU size, const void *values, EShaderStages stages)
	{
		vkCmdPushConstants( _cmdbuf, _curLayout, VEnumCast(stages), uint(offset), uint(size), values );
	}
		
/*
=================================================
	SetScissor
=================================================
*/
	void VRenderGraph::RenderContext::SetScissor (uint first, ArrayView<RectI> scissors)
	{
		StaticArray< VkRect2D, GraphicsConfig::MaxViewports >	vk_scissors;

		for (size_t i = 0; i < scissors.size(); ++i)
		{
			auto&	src = scissors[i];
			auto&	dst = vk_scissors[i];

			dst.offset	= { src.left,          src.top };
			dst.extent	= { uint(src.Width()), uint(src.Height()) };
		}
		vkCmdSetScissor( _cmdbuf, first, uint(scissors.size()), vk_scissors.data() );
	}
	
/*
=================================================
	SetDepthBias
=================================================
*/
	void VRenderGraph::RenderContext::SetDepthBias (float depthBiasConstantFactor, float depthBiasClamp, float depthBiasSlopeFactor)
	{
		vkCmdSetDepthBias( _cmdbuf, depthBiasConstantFactor, depthBiasClamp, depthBiasSlopeFactor );
	}
	
/*
=================================================
	SetLineWidth
=================================================
*/
	void VRenderGraph::RenderContext::SetLineWidth (float lineWidth)
	{
		vkCmdSetLineWidth( _cmdbuf, lineWidth );
	}
	
/*
=================================================
	SetDepthBounds
=================================================
*/
	void VRenderGraph::RenderContext::SetDepthBounds (float minDepthBounds, float maxDepthBounds)
	{
		vkCmdSetDepthBounds( _cmdbuf, minDepthBounds, maxDepthBounds );
	}
	
/*
=================================================
	SetStencilCompareMask
=================================================
*/
	void VRenderGraph::RenderContext::SetStencilCompareMask (EStencilFace faceMask, uint compareMask)
	{
		vkCmdSetStencilCompareMask( _cmdbuf, VEnumCast(faceMask), compareMask );
	}
	
/*
=================================================
	SetStencilWriteMask
=================================================
*/
	void VRenderGraph::RenderContext::SetStencilWriteMask (EStencilFace faceMask, uint writeMask)
	{
		vkCmdSetStencilWriteMask( _cmdbuf, VEnumCast(faceMask), writeMask );
	}
	
/*
=================================================
	SetStencilReference
=================================================
*/
	void VRenderGraph::RenderContext::SetStencilReference (EStencilFace faceMask, uint reference)
	{
		vkCmdSetStencilReference( _cmdbuf, VEnumCast(faceMask), reference );
	}
	
/*
=================================================
	SetBlendConstants
=================================================
*/
	void VRenderGraph::RenderContext::SetBlendConstants (const RGBA32f &color)
	{
		vkCmdSetBlendConstants( _cmdbuf, color.data() );
	}
	
/*
=================================================
	BindIndexBuffer
=================================================
*/
	void VRenderGraph::RenderContext::BindIndexBuffer (GfxResourceID buffer, BytesU offset, EIndex indexType)
	{
		auto*	buf = _graphicsCtx.ToLocalBuffer( buffer );
		CHECK_ERR( buf, void());

		if ( (_indexBuffer == buf->Handle()) & (_indexOffset == offset) )
			return;

		_indexBuffer	= buf->Handle();
		_indexOffset	= offset;
		vkCmdBindIndexBuffer( _cmdbuf, _indexBuffer, VkDeviceSize(_indexOffset), VEnumCast(indexType) );

		if ( _graphicsCtx._enableBarriers ) {
			_graphicsCtx._CheckBufferAccess( buf, EResourceState::IndexBuffer, VkDeviceSize(offset), VkDeviceSize(buf->Size() - offset) );
		}
	}
	
/*
=================================================
	BindVertexBuffer
=================================================
*/
	void VRenderGraph::RenderContext::BindVertexBuffer (uint index, GfxResourceID buffer, BytesU offset)
	{
		auto*	buf = _graphicsCtx.ToLocalBuffer( buffer );
		CHECK_ERR( buf, void());

		const VkBuffer		handle	= buf->Handle();
		const VkDeviceSize	off		= VkDeviceSize(offset);

		vkCmdBindVertexBuffers( _cmdbuf, index, 1, &handle, &off );
		
		if ( _graphicsCtx._enableBarriers ) {
			_graphicsCtx._CheckBufferAccess( buf, EResourceState::VertexBuffer, VkDeviceSize(offset), VkDeviceSize(buf->Size() - offset) );
		}
	}
		
/*
=================================================
	BindVertexBuffers
=================================================
*/
	void VRenderGraph::RenderContext::BindVertexBuffers (uint firstBinding, ArrayView<Pair<GfxResourceID, BytesU>> bufferAndOffset)
	{
		StaticArray< VkBuffer, GraphicsConfig::MaxVertexBuffers >		buffers;
		StaticArray< VkDeviceSize, GraphicsConfig::MaxVertexBuffers >	offsets;

		for (uint i = 0; i < bufferAndOffset.size(); ++i)
		{
			auto&	buf	= bufferAndOffset[i].first;
			auto&	off = bufferAndOffset[i].second;
			
			auto*	buffer = _graphicsCtx.ToLocalBuffer( buf );
			CHECK_ERR( buffer, void());

			buffers[i]	= buffer->Handle();
			offsets[i]	= VkDeviceSize(off);
			
			if ( _graphicsCtx._enableBarriers ) {
				_graphicsCtx._CheckBufferAccess( buffer, EResourceState::VertexBuffer, VkDeviceSize(off), VkDeviceSize(buffer->Size() - off) );
			}
		}

		vkCmdBindVertexBuffers( _cmdbuf, firstBinding, uint(bufferAndOffset.size()), buffers.data(), offsets.data() );
	}
	
/*
=================================================
	Draw
=================================================
*/
	void VRenderGraph::RenderContext::Draw (uint vertexCount, uint instanceCount, uint firstVertex, uint firstInstance)
	{
		_graphicsCtx._CommitBarriers();

		vkCmdDraw( _cmdbuf, vertexCount, instanceCount, firstVertex, firstInstance );
	}
	
/*
=================================================
	DrawIndexed
=================================================
*/
	void VRenderGraph::RenderContext::DrawIndexed (uint indexCount, uint instanceCount, uint firstIndex, int vertexOffset, uint firstInstance)
	{
		_graphicsCtx._CommitBarriers();

		vkCmdDrawIndexed( _cmdbuf, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance );
	}
	
/*
=================================================
	DrawIndirect
=================================================
*/
	void VRenderGraph::RenderContext::DrawIndirect (GfxResourceID buffer, BytesU offset, uint drawCount, BytesU stride)
	{
		auto*	buf = _graphicsCtx.ToLocalBuffer( buffer );
		CHECK_ERR( buf, void());
		
		if ( _graphicsCtx._enableBarriers ) {
			_graphicsCtx._CheckBufferAccess( buf, EResourceState::IndirectBuffer, VkDeviceSize(offset), VkDeviceSize(stride) * drawCount );
		}
		_graphicsCtx._CommitBarriers();

		vkCmdDrawIndirect( _cmdbuf, buf->Handle(), VkDeviceSize(offset), drawCount, CheckCast<uint>(stride) );
	}
	
/*
=================================================
	DrawIndexedIndirect
=================================================
*/
	void VRenderGraph::RenderContext::DrawIndexedIndirect (GfxResourceID buffer, BytesU offset, uint drawCount, BytesU stride)
	{
		auto*	buf = _graphicsCtx.ToLocalBuffer( buffer );
		CHECK_ERR( buf, void());
		
		if ( _graphicsCtx._enableBarriers ) {
			_graphicsCtx._CheckBufferAccess( buf, EResourceState::IndirectBuffer, VkDeviceSize(offset), VkDeviceSize(stride) * drawCount );
		}
		_graphicsCtx._CommitBarriers();

		vkCmdDrawIndexedIndirect( _cmdbuf, buf->Handle(), VkDeviceSize(offset), drawCount, CheckCast<uint>(stride) );
	}
	
/*
=================================================
	DrawIndirectCount
=================================================
*/
	void VRenderGraph::RenderContext::DrawIndirectCount (GfxResourceID buffer, BytesU offset, GfxResourceID countBuffer, BytesU countBufferOffset, uint maxDrawCount, BytesU stride)
	{
		CHECK_ERR( _graphicsCtx.GetDevice().GetFeatures().drawIndirectCount, void());

		auto*	ibuf = _graphicsCtx.ToLocalBuffer( buffer );
		auto*	cbuf = _graphicsCtx.ToLocalBuffer( countBuffer );
		CHECK_ERR( ibuf and cbuf, void());
		
		if ( _graphicsCtx._enableBarriers )
		{
			_graphicsCtx._CheckBufferAccess( ibuf, EResourceState::IndirectBuffer, VkDeviceSize(offset), VkDeviceSize(stride) * maxDrawCount );
			_graphicsCtx._CheckBufferAccess( cbuf, EResourceState::IndirectBuffer, VkDeviceSize(countBufferOffset), sizeof(uint) );
		}
		_graphicsCtx._CommitBarriers();

		vkCmdDrawIndirectCountKHR( _cmdbuf, ibuf->Handle(), VkDeviceSize(offset), cbuf->Handle(), VkDeviceSize(countBufferOffset), maxDrawCount, CheckCast<uint>(stride) );
	}
	
/*
=================================================
	DrawIndexedIndirectCount
=================================================
*/
	void VRenderGraph::RenderContext::DrawIndexedIndirectCount (GfxResourceID buffer, BytesU offset, GfxResourceID countBuffer, BytesU countBufferOffset, uint maxDrawCount, BytesU stride)
	{
		CHECK_ERR( _graphicsCtx.GetDevice().GetFeatures().drawIndirectCount, void());

		auto*	ibuf = _graphicsCtx.ToLocalBuffer( buffer );
		auto*	cbuf = _graphicsCtx.ToLocalBuffer( countBuffer );
		CHECK_ERR( ibuf and cbuf, void());
		
		if ( _graphicsCtx._enableBarriers )
		{
			_graphicsCtx._CheckBufferAccess( ibuf, EResourceState::IndirectBuffer, VkDeviceSize(offset), VkDeviceSize(stride) * maxDrawCount );
			_graphicsCtx._CheckBufferAccess( cbuf, EResourceState::IndirectBuffer, VkDeviceSize(countBufferOffset), sizeof(uint) );
		}
		_graphicsCtx._CommitBarriers();

		vkCmdDrawIndexedIndirectCountKHR( _cmdbuf, ibuf->Handle(), VkDeviceSize(offset), cbuf->Handle(), VkDeviceSize(countBufferOffset), maxDrawCount, CheckCast<uint>(stride) );
	}
	
/*
=================================================
	DrawMeshTasksNV
=================================================
*/
	void VRenderGraph::RenderContext::DrawMeshTasksNV (uint taskCount, uint firstTask)
	{
	#ifdef VK_NV_mesh_shader
		CHECK_ERR( _graphicsCtx.GetDevice().IsMeshShaderEnabled(), void());

		_graphicsCtx._CommitBarriers();
		
		vkCmdDrawMeshTasksNV( _cmdbuf, taskCount, firstTask );

	#else
		AE_LOGE( "mesh shader is not supported!" );
	#endif
	}
	
/*
=================================================
	DrawMeshTasksIndirectNV
=================================================
*/
	void VRenderGraph::RenderContext::DrawMeshTasksIndirectNV (GfxResourceID buffer, BytesU offset, uint drawCount, BytesU stride)
	{
	#ifdef VK_NV_mesh_shader
		CHECK_ERR( _graphicsCtx.GetDevice().IsMeshShaderEnabled(), void());

		auto*	buf = _graphicsCtx.ToLocalBuffer( buffer );
		CHECK_ERR( buf, void());
		
		if ( _graphicsCtx._enableBarriers ) {
			_graphicsCtx._CheckBufferAccess( buf, EResourceState::IndirectBuffer, VkDeviceSize(offset), VkDeviceSize(stride) * drawCount );
		}
		_graphicsCtx._CommitBarriers();

		vkCmdDrawMeshTasksIndirectNV( _cmdbuf, buf->Handle(), VkDeviceSize(offset), drawCount, CheckCast<uint>(stride) );

	#else
		AE_LOGE( "mesh shader is not supported!" );
	#endif
	}
	
/*
=================================================
	DrawMeshTasksIndirectCountNV
=================================================
*/
	void VRenderGraph::RenderContext::DrawMeshTasksIndirectCountNV (GfxResourceID buffer, BytesU offset, GfxResourceID countBuffer, BytesU countBufferOffset, uint maxDrawCount, BytesU stride)
	{
	#ifdef VK_NV_mesh_shader
		CHECK_ERR( _graphicsCtx.GetDevice().IsMeshShaderEnabled(), void());

		auto*	ibuf = _graphicsCtx.ToLocalBuffer( buffer );
		auto*	cbuf = _graphicsCtx.ToLocalBuffer( countBuffer );
		CHECK_ERR( ibuf and cbuf, void());
		
		if ( _graphicsCtx._enableBarriers )
		{
			_graphicsCtx._CheckBufferAccess( ibuf, EResourceState::IndirectBuffer, VkDeviceSize(offset), VkDeviceSize(stride) * maxDrawCount );
			_graphicsCtx._CheckBufferAccess( cbuf, EResourceState::IndirectBuffer, VkDeviceSize(countBufferOffset), sizeof(uint) );
		}
		_graphicsCtx._CommitBarriers();

		vkCmdDrawMeshTasksIndirectCountNV( _cmdbuf, ibuf->Handle(), VkDeviceSize(offset), cbuf->Handle(), VkDeviceSize(countBufferOffset), maxDrawCount, CheckCast<uint>(stride) );

	#else
		AE_LOGE( "mesh shader is not supported!" );
	#endif
	}


}	// AE::Graphics
