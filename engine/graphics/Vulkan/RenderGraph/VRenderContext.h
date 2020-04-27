// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

namespace AE::Graphics
{
	
	//
	// Render Context
	//

	class VRenderGraph::RenderContext final : public VulkanDeviceFn, public IAsyncRenderContext
	{
	// variables
	private:
		VkCommandBuffer				_cmdbuf			= VK_NULL_HANDLE;
		
		VResourceManager &			_resMngr;
		VLogicalRenderPass const&	_logicalRP;

		VResourceMap				_resources;

		// cached states
		struct {
			VkPipelineLayout			pplnLayout		= VK_NULL_HANDLE;
			VkPipeline					pipeline		= VK_NULL_HANDLE;

			VkBuffer					indexBuffer		= VK_NULL_HANDLE;
			BytesU						indexOffset;
		}							_states;

		UniqueID<BakedCommandBufferID>	_cachedCommands;
		VBakedCommandBuffer const*		_renderCommands	= null;
		VCommandPoolPtr					_cmdPool;

		CmdBatchID					_batchId;

		bool						_isAsync	: 1;
		bool						_isCached	: 1;
		bool						_hasErrors	: 1;


	// methods
	public:
		RenderContext (VResourceManager &resMngr, const VLogicalRenderPass &logicalRP, CmdBatchID batchId);

		void  Begin (VkCommandBuffer primaryCmdBuf);
		void  BeginAsync (const VCommandPoolPtr &pool);

		ND_ UniqueID<BakedCommandBufferID>  EndAsync ();

		ND_ VResourceMap &	GetResourceMap ()	{ return _resources; }


	// IAsyncRenderContext
		UniqueID<BakedCommandBufferID>  CacheCommandBuffer () override;
		bool  ReuseCommandBuffer (const BakedCommandBufferID &) override;
		bool  BeginCommandBuffer () override;
		
		void  Barrier (ArrayView< Pair<GfxResourceID, EResourceState>>) override;


	// IRenderContext
		NativeContext_t    GetNativeContext () override;
		RenderContextInfo  GetContextInfo () override;

		void  ResetStates () override;

		void  BindPipeline (GraphicsPipelineID ppln) override;
		void  BindPipeline (MeshPipelineID ppln) override;
		void  BindDescriptorSet (uint index, DescriptorSetID ds, ArrayView<uint> dynamicOffsets) override;
		void  PushConstant (BytesU offset, BytesU size, const void *values, EShaderStages stages) override;
		
		void  SetScissor (uint first, ArrayView<RectI> scissors) override;
		void  SetDepthBias (float depthBiasConstantFactor, float depthBiasClamp, float depthBiasSlopeFactor) override;
		void  SetLineWidth (float lineWidth) override;
		void  SetDepthBounds (float minDepthBounds, float maxDepthBounds) override;
		void  SetStencilCompareMask (EStencilFace faceMask, uint compareMask) override;
		void  SetStencilWriteMask (EStencilFace faceMask, uint writeMask) override;
		void  SetStencilReference (EStencilFace faceMask, uint reference) override;
		void  SetBlendConstants (const RGBA32f &color) override;

		void  BindIndexBuffer (GfxResourceID buffer, BytesU offset, EIndex indexType) override;
		void  BindVertexBuffer (uint index, GfxResourceID buffer, BytesU offset) override;
		void  BindVertexBuffers (uint firstBinding, ArrayView<Pair<GfxResourceID, BytesU>> bufferAndOffset) override;

		void  Draw (uint vertexCount, uint instanceCount, uint firstVertex, uint firstInstance) override;
		void  DrawIndexed (uint indexCount, uint instanceCount, uint firstIndex, int vertexOffset, uint firstInstance) override;
		void  DrawIndirect (GfxResourceID buffer, BytesU offset, uint drawCount, BytesU stride) override;
		void  DrawIndexedIndirect (GfxResourceID buffer, BytesU offset, uint drawCount, BytesU stride) override;
		void  DrawIndirectCount (GfxResourceID buffer, BytesU offset, GfxResourceID countBuffer, BytesU countBufferOffset, uint maxDrawCount, BytesU stride) override;
		void  DrawIndexedIndirectCount (GfxResourceID buffer, BytesU offset, GfxResourceID countBuffer, BytesU countBufferOffset, uint maxDrawCount, BytesU stride) override;

		void  DrawMeshTasksNV (uint taskCount, uint firstTask) override;
		void  DrawMeshTasksIndirectNV (GfxResourceID buffer, BytesU offset, uint drawCount, BytesU stride) override;
		void  DrawMeshTasksIndirectCountNV (GfxResourceID buffer, BytesU offset, GfxResourceID countBuffer, BytesU countBufferOffset, uint maxDrawCount, BytesU stride) override;

	private:
		template <typename ID>
		ND_ auto*  _AcquireResource (ID id, bool quit = false);

		ND_ bool  _IsAsync ()					const	{ return _isAsync; }
		ND_ bool  _IsCached ()					const	{ return _isCached; }
		ND_ bool  _IsRecordingNewCommands ()	const	{ return _renderCommands != null; }
	};
//-----------------------------------------------------------------------------
	

/*
=================================================
	constructor
=================================================
*/
	VRenderGraph::RenderContext::RenderContext (VResourceManager &resMngr, const VLogicalRenderPass &logicalRP, CmdBatchID batchId) :
		_resMngr{ resMngr },	_logicalRP{ logicalRP },	_batchId{ batchId },
		_isAsync{ false },		_isCached{ false },			_hasErrors{ false }
	{
		VulkanDeviceFn_Init( resMngr.GetDevice() );
	}

/*
=================================================
	Begin
=================================================
*/
	void  VRenderGraph::RenderContext::Begin (VkCommandBuffer primaryCmdBuf)
	{
		_cmdbuf	= primaryCmdBuf;
	}
	
/*
=================================================
	BeginAsync
=================================================
*/
	void  VRenderGraph::RenderContext::BeginAsync (const VCommandPoolPtr &pool)
	{
		_cmdPool	= pool;
		_isAsync	= true;
	}

/*
=================================================
	EndAsync
=================================================
*/
	UniqueID<BakedCommandBufferID>  VRenderGraph::RenderContext::EndAsync ()
	{
		if ( _renderCommands and _cmdbuf )
		{
			VK_CALL( vkEndCommandBuffer( _cmdbuf ));

			_renderCommands->SetResources( std::move(_resources) );
			_resources = Default;
		}
			
		_renderCommands = null;
		_cmdbuf			= VK_NULL_HANDLE;
		
		if ( _hasErrors )
		{
			_resMngr.ReleaseResource( _cachedCommands );
			RETURN_ERR( "command buffer recorded with errors" );
		}

		return std::move(_cachedCommands);
	}

/*
=================================================
	_AcquireResource
=================================================
*/
	template <typename ID>
	inline auto*  VRenderGraph::RenderContext::_AcquireResource (ID id, bool quit)
	{
		return _resMngr.GetResource( id, _resources.Add( id ), quit );
	}

/*
=================================================
	GetNativeContext
=================================================
*/
	IRenderContext::NativeContext_t  VRenderGraph::RenderContext::GetNativeContext ()
	{
		auto*	render_pass	= _AcquireResource( _logicalRP.GetRenderPass() );
		auto*	framebuffer	= _AcquireResource( _logicalRP.GetFramebuffer(), true );

		VulkanRenderContext	vctx;
		vctx.cmdBuffer		= BitCast<CommandBufferVk_t>(_cmdbuf);
		vctx.renderPass		= BitCast<RenderPassVk_t>(render_pass ? render_pass->Handle() : VK_NULL_HANDLE);
		vctx.framebuffer	= BitCast<FramebufferVk_t>(framebuffer ? framebuffer->Handle() : VK_NULL_HANDLE);
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
		result.layerCount	= uint(_logicalRP.GetViewports().size());
		result.batchId		= _batchId;
		return result;
	}
	
/*
=================================================
	ResetStates
=================================================
*/
	void  VRenderGraph::RenderContext::ResetStates ()
	{
		_states.pplnLayout	= VK_NULL_HANDLE;
		_states.pipeline	= VK_NULL_HANDLE;

		_states.indexBuffer	= VK_NULL_HANDLE;
		_states.indexOffset	= 0_b;
	}
	
/*
=================================================
	BindPipeline
=================================================
*/
	void  VRenderGraph::RenderContext::BindPipeline (GraphicsPipelineID ppln)
	{
		ASSERT( not _IsCached() );

		auto*	gppln = _AcquireResource( ppln );
		CHECK_ERR( gppln, void());

		if ( _states.pipeline == gppln->Handle() )
			return;

		_states.pipeline	= gppln->Handle();
		_states.pplnLayout	= gppln->Layout();
		vkCmdBindPipeline( _cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, _states.pipeline );
	}
	
/*
=================================================
	BindPipeline
=================================================
*/
	void  VRenderGraph::RenderContext::BindPipeline (MeshPipelineID ppln)
	{
		ASSERT( not _IsCached() );

		auto*	mppln = _AcquireResource( ppln );
		CHECK_ERR( mppln, void());

		if ( _states.pipeline == mppln->Handle() )
			return;

		_states.pipeline	= mppln->Handle();
		_states.pplnLayout	= mppln->Layout();
		vkCmdBindPipeline( _cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, _states.pipeline );
	}
		
/*
=================================================
	BindDescriptorSet
=================================================
*/
	void  VRenderGraph::RenderContext::BindDescriptorSet (uint index, DescriptorSetID ds, ArrayView<uint> dynamicOffsets)
	{
		ASSERT( not _IsCached() );
		ASSERT( _states.pplnLayout );

		auto*	desc_set = _AcquireResource( ds );
		CHECK_ERR( desc_set, void());

		const VkDescriptorSet	vk_ds[] = { desc_set->Handle() };

		vkCmdBindDescriptorSets( _cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, _states.pplnLayout, index, uint(CountOf(vk_ds)), vk_ds, uint(dynamicOffsets.size()), dynamicOffsets.data() );
	}
	
/*
=================================================
	PushConstant
=================================================
*/
	void  VRenderGraph::RenderContext::PushConstant (BytesU offset, BytesU size, const void *values, EShaderStages stages)
	{
		ASSERT( not _IsCached() );

		vkCmdPushConstants( _cmdbuf, _states.pplnLayout, VEnumCast(stages), uint(offset), uint(size), values );
	}
		
/*
=================================================
	SetScissor
=================================================
*/
	void  VRenderGraph::RenderContext::SetScissor (uint first, ArrayView<RectI> scissors)
	{
		ASSERT( not _IsCached() );

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
	void  VRenderGraph::RenderContext::SetDepthBias (float depthBiasConstantFactor, float depthBiasClamp, float depthBiasSlopeFactor)
	{
		ASSERT( not _IsCached() );

		vkCmdSetDepthBias( _cmdbuf, depthBiasConstantFactor, depthBiasClamp, depthBiasSlopeFactor );
	}
	
/*
=================================================
	SetLineWidth
=================================================
*/
	void  VRenderGraph::RenderContext::SetLineWidth (float lineWidth)
	{
		ASSERT( not _IsCached() );

		vkCmdSetLineWidth( _cmdbuf, lineWidth );
	}
	
/*
=================================================
	SetDepthBounds
=================================================
*/
	void  VRenderGraph::RenderContext::SetDepthBounds (float minDepthBounds, float maxDepthBounds)
	{
		ASSERT( not _IsCached() );

		vkCmdSetDepthBounds( _cmdbuf, minDepthBounds, maxDepthBounds );
	}
	
/*
=================================================
	SetStencilCompareMask
=================================================
*/
	void  VRenderGraph::RenderContext::SetStencilCompareMask (EStencilFace faceMask, uint compareMask)
	{
		ASSERT( not _IsCached() );

		vkCmdSetStencilCompareMask( _cmdbuf, VEnumCast(faceMask), compareMask );
	}
	
/*
=================================================
	SetStencilWriteMask
=================================================
*/
	void  VRenderGraph::RenderContext::SetStencilWriteMask (EStencilFace faceMask, uint writeMask)
	{
		ASSERT( not _IsCached() );

		vkCmdSetStencilWriteMask( _cmdbuf, VEnumCast(faceMask), writeMask );
	}
	
/*
=================================================
	SetStencilReference
=================================================
*/
	void  VRenderGraph::RenderContext::SetStencilReference (EStencilFace faceMask, uint reference)
	{
		ASSERT( not _IsCached() );

		vkCmdSetStencilReference( _cmdbuf, VEnumCast(faceMask), reference );
	}
	
/*
=================================================
	SetBlendConstants
=================================================
*/
	void  VRenderGraph::RenderContext::SetBlendConstants (const RGBA32f &color)
	{
		ASSERT( not _IsCached() );

		vkCmdSetBlendConstants( _cmdbuf, color.data() );
	}
	
/*
=================================================
	BindIndexBuffer
=================================================
*/
	void  VRenderGraph::RenderContext::BindIndexBuffer (GfxResourceID buffer, BytesU offset, EIndex indexType)
	{
		ASSERT( not _IsCached() );
		CHECK_ERR( buffer.ResourceType() == GfxResourceID::EType::Buffer, void());

		auto*	buf = _AcquireResource( VBufferID{ buffer.Index(), buffer.Generation() });
		CHECK_ERR( buf, void());

		if ( (_states.indexBuffer == buf->Handle()) & (_states.indexOffset == offset) )
			return;

		_states.indexBuffer	= buf->Handle();
		_states.indexOffset	= offset;
		vkCmdBindIndexBuffer( _cmdbuf, _states.indexBuffer, VkDeviceSize(_states.indexOffset), VEnumCast(indexType) );
	}
	
/*
=================================================
	BindVertexBuffer
=================================================
*/
	void  VRenderGraph::RenderContext::BindVertexBuffer (uint index, GfxResourceID buffer, BytesU offset)
	{
		ASSERT( not _IsCached() );
		CHECK_ERR( buffer.ResourceType() == GfxResourceID::EType::Buffer, void());

		auto*	buf = _AcquireResource( VBufferID{ buffer.Index(), buffer.Generation() });
		CHECK_ERR( buf, void());

		const VkBuffer		handle	= buf->Handle();
		const VkDeviceSize	off		= VkDeviceSize(offset);

		vkCmdBindVertexBuffers( _cmdbuf, index, 1, &handle, &off );
	}
		
/*
=================================================
	BindVertexBuffers
=================================================
*/
	void  VRenderGraph::RenderContext::BindVertexBuffers (uint firstBinding, ArrayView<Pair<GfxResourceID, BytesU>> bufferAndOffset)
	{
		ASSERT( not _IsCached() );

		StaticArray< VkBuffer, GraphicsConfig::MaxVertexBuffers >		buffers;
		StaticArray< VkDeviceSize, GraphicsConfig::MaxVertexBuffers >	offsets;

		for (uint i = 0; i < bufferAndOffset.size(); ++i)
		{
			auto&	buf	= bufferAndOffset[i].first;
			auto&	off = bufferAndOffset[i].second;
	
			CHECK_ERR( buf.ResourceType() == GfxResourceID::EType::Buffer, void());

			auto*	buffer = _AcquireResource( VBufferID{ buf.Index(), buf.Generation() });
			CHECK_ERR( buffer, void());

			buffers[i]	= buffer->Handle();
			offsets[i]	= VkDeviceSize(off);
		}

		vkCmdBindVertexBuffers( _cmdbuf, firstBinding, uint(bufferAndOffset.size()), buffers.data(), offsets.data() );
	}
	
/*
=================================================
	Draw
=================================================
*/
	void  VRenderGraph::RenderContext::Draw (uint vertexCount, uint instanceCount, uint firstVertex, uint firstInstance)
	{
		ASSERT( not _IsCached() );

		vkCmdDraw( _cmdbuf, vertexCount, instanceCount, firstVertex, firstInstance );
	}
	
/*
=================================================
	DrawIndexed
=================================================
*/
	void  VRenderGraph::RenderContext::DrawIndexed (uint indexCount, uint instanceCount, uint firstIndex, int vertexOffset, uint firstInstance)
	{
		ASSERT( not _IsCached() );

		vkCmdDrawIndexed( _cmdbuf, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance );
	}
	
/*
=================================================
	DrawIndirect
=================================================
*/
	void  VRenderGraph::RenderContext::DrawIndirect (GfxResourceID buffer, BytesU offset, uint drawCount, BytesU stride)
	{
		ASSERT( not _IsCached() );
		CHECK_ERR( buffer.ResourceType() == GfxResourceID::EType::Buffer, void());

		auto*	buf = _AcquireResource( VBufferID{ buffer.Index(), buffer.Generation() });
		CHECK_ERR( buf, void());

		vkCmdDrawIndirect( _cmdbuf, buf->Handle(), VkDeviceSize(offset), drawCount, CheckCast<uint>(stride) );
	}
	
/*
=================================================
	DrawIndexedIndirect
=================================================
*/
	void  VRenderGraph::RenderContext::DrawIndexedIndirect (GfxResourceID buffer, BytesU offset, uint drawCount, BytesU stride)
	{
		ASSERT( not _IsCached() );
		CHECK_ERR( buffer.ResourceType() == GfxResourceID::EType::Buffer, void());

		auto*	buf = _AcquireResource( VBufferID{ buffer.Index(), buffer.Generation() });
		CHECK_ERR( buf, void());

		vkCmdDrawIndexedIndirect( _cmdbuf, buf->Handle(), VkDeviceSize(offset), drawCount, CheckCast<uint>(stride) );
	}
	
/*
=================================================
	DrawIndirectCount
=================================================
*/
	void  VRenderGraph::RenderContext::DrawIndirectCount (GfxResourceID buffer, BytesU offset, GfxResourceID countBuffer, BytesU countBufferOffset, uint maxDrawCount, BytesU stride)
	{
		ASSERT( not _IsCached() );
		CHECK_ERR( buffer.ResourceType() == GfxResourceID::EType::Buffer, void());
		CHECK_ERR( countBuffer.ResourceType() == GfxResourceID::EType::Buffer, void());
		CHECK_ERR( _resMngr.GetDevice().GetFeatures().drawIndirectCount, void());

		auto*	ibuf = _AcquireResource( VBufferID{ buffer.Index(), buffer.Generation() });
		auto*	cbuf = _AcquireResource( VBufferID{ countBuffer.Index(), countBuffer.Generation() });
		CHECK_ERR( ibuf and cbuf, void());

		vkCmdDrawIndirectCountKHR( _cmdbuf, ibuf->Handle(), VkDeviceSize(offset), cbuf->Handle(), VkDeviceSize(countBufferOffset), maxDrawCount, CheckCast<uint>(stride) );
	}
	
/*
=================================================
	DrawIndexedIndirectCount
=================================================
*/
	void  VRenderGraph::RenderContext::DrawIndexedIndirectCount (GfxResourceID buffer, BytesU offset, GfxResourceID countBuffer, BytesU countBufferOffset, uint maxDrawCount, BytesU stride)
	{
		ASSERT( not _IsCached() );
		CHECK_ERR( buffer.ResourceType() == GfxResourceID::EType::Buffer, void());
		CHECK_ERR( countBuffer.ResourceType() == GfxResourceID::EType::Buffer, void());
		CHECK_ERR( _resMngr.GetDevice().GetFeatures().drawIndirectCount, void());

		auto*	ibuf = _AcquireResource( VBufferID{ buffer.Index(), buffer.Generation() });
		auto*	cbuf = _AcquireResource( VBufferID{ countBuffer.Index(), countBuffer.Generation() });
		CHECK_ERR( ibuf and cbuf, void());
		
		vkCmdDrawIndexedIndirectCountKHR( _cmdbuf, ibuf->Handle(), VkDeviceSize(offset), cbuf->Handle(), VkDeviceSize(countBufferOffset), maxDrawCount, CheckCast<uint>(stride) );
	}
	
/*
=================================================
	DrawMeshTasksNV
=================================================
*/
	void  VRenderGraph::RenderContext::DrawMeshTasksNV (uint taskCount, uint firstTask)
	{
		ASSERT( not _IsCached() );

	#ifdef VK_NV_mesh_shader
		CHECK_ERR( _resMngr.GetDevice().GetFeatures().meshShaderNV, void());
		
		vkCmdDrawMeshTasksNV( _cmdbuf, taskCount, firstTask );

	#else
		Unused( taskCount, firstTask );
		AE_LOGE( "mesh shader is not supported!" );
	#endif
	}
	
/*
=================================================
	DrawMeshTasksIndirectNV
=================================================
*/
	void  VRenderGraph::RenderContext::DrawMeshTasksIndirectNV (GfxResourceID buffer, BytesU offset, uint drawCount, BytesU stride)
	{
		ASSERT( not _IsCached() );

	#ifdef VK_NV_mesh_shader
		CHECK_ERR( buffer.ResourceType() == GfxResourceID::EType::Buffer, void());
		CHECK_ERR( _resMngr.GetDevice().GetFeatures().meshShaderNV, void());

		auto*	buf = _AcquireResource( VBufferID{ buffer.Index(), buffer.Generation() });
		CHECK_ERR( buf, void());

		vkCmdDrawMeshTasksIndirectNV( _cmdbuf, buf->Handle(), VkDeviceSize(offset), drawCount, CheckCast<uint>(stride) );

	#else
		Unused( buffer, offset, drawCount, stride );
		AE_LOGE( "mesh shader is not supported!" );
	#endif
	}
	
/*
=================================================
	DrawMeshTasksIndirectCountNV
=================================================
*/
	void  VRenderGraph::RenderContext::DrawMeshTasksIndirectCountNV (GfxResourceID buffer, BytesU offset, GfxResourceID countBuffer, BytesU countBufferOffset, uint maxDrawCount, BytesU stride)
	{
		ASSERT( not _IsCached() );

	#ifdef VK_NV_mesh_shader
		CHECK_ERR( buffer.ResourceType() == GfxResourceID::EType::Buffer, void());
		CHECK_ERR( _resMngr.GetDevice().GetFeatures().meshShaderNV, void());

		auto*	ibuf = _AcquireResource( VBufferID{ buffer.Index(), buffer.Generation() });
		auto*	cbuf = _AcquireResource( VBufferID{ countBuffer.Index(), countBuffer.Generation() });
		CHECK_ERR( ibuf and cbuf, void());
		
		vkCmdDrawMeshTasksIndirectCountNV( _cmdbuf, ibuf->Handle(), VkDeviceSize(offset), cbuf->Handle(), VkDeviceSize(countBufferOffset), maxDrawCount, CheckCast<uint>(stride) );

	#else
		Unused( buffer, offset, countBuffer, countBufferOffset, maxDrawCount, stride );
		AE_LOGE( "mesh shader is not supported!" );
	#endif
	}
	
/*
=================================================
	CacheCommandBuffer
=================================================
*/
	UniqueID<BakedCommandBufferID>  VRenderGraph::RenderContext::CacheCommandBuffer ()
	{
		ASSERT( _IsAsync() );
		CHECK_ERR( not _IsCached() );
		CHECK_ERR( _IsRecordingNewCommands() );
		CHECK_ERR( _cachedCommands );
		
		if ( _renderCommands and _cmdbuf )
		{
			VK_CHECK( vkEndCommandBuffer( _cmdbuf ));

			_renderCommands->SetResources( std::move(_resources) );
			_resources = Default;
		}

		_renderCommands	= null;
		_cmdbuf			= VK_NULL_HANDLE;
		
		if ( _hasErrors )
		{
			_resMngr.ReleaseResource( _cachedCommands );
			RETURN_ERR( "command buffer recorded with errors" );
		}

		_isCached = true;

		return _resMngr.AcquireResource( BakedCommandBufferID{_cachedCommands} );
	}
	
/*
=================================================
	ReuseCommandBuffer
=================================================
*/
	bool  VRenderGraph::RenderContext::ReuseCommandBuffer (const BakedCommandBufferID &cmdBufId)
	{
		ASSERT( _IsAsync() );
		CHECK_ERR( not _IsRecordingNewCommands() );

		_cachedCommands = _resMngr.AcquireResource( cmdBufId );

		if ( not _cachedCommands )
			return false;

		auto*	render_commands = _resMngr.GetResource( _cachedCommands );

		if ( _logicalRP.GetRenderPass() != render_commands->GetRenderPass() )
		{
			_resMngr.ReleaseResource( _cachedCommands );
			return false;
		}

		_isCached = true;
		return true;
	}
	
/*
=================================================
	BeginCommandBuffer
=================================================
*/
	bool  VRenderGraph::RenderContext::BeginCommandBuffer ()
	{
		ASSERT( _IsAsync() );
		CHECK_ERR( not _IsCached() );

		_hasErrors = true;

		_cachedCommands = _resMngr.CreateCommandBuffer( _cmdPool, _logicalRP.GetRenderPass() );
		_renderCommands = _resMngr.GetResource( _cachedCommands );
		CHECK_ERR( _renderCommands );

		_cmdbuf = _renderCommands->GetCommands();

		// begin recording
		{
			auto*	render_pass	= _AcquireResource( _logicalRP.GetRenderPass() );
			CHECK_ERR( render_pass );

			VkCommandBufferInheritanceInfo	inheritance = {};
			inheritance.sType		= VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
			inheritance.renderPass	= render_pass->Handle();
			inheritance.subpass		= _logicalRP.GetSubpassIndex();

			VkCommandBufferBeginInfo	info = {};
			info.sType				= VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			info.pInheritanceInfo	= &inheritance;
			info.flags				= VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;

			VK_CHECK( vkBeginCommandBuffer( _cmdbuf, &info ));
		}
		
		// initial states
		{
			if ( auto viewports = _logicalRP.GetViewports(); viewports.size() )
				vkCmdSetViewport( _cmdbuf, 0, uint(viewports.size()), viewports.data() );
		
			if ( auto scissors = _logicalRP.GetScissors(); scissors.size() )
				vkCmdSetScissor( _cmdbuf, 0, uint(scissors.size()), scissors.data() );
		}

		_hasErrors = false;
		return true;
	}

/*
=================================================
	Barrier
=================================================
*/
	void  VRenderGraph::RenderContext::Barrier (ArrayView< Pair<GfxResourceID, EResourceState>> barriers)
	{
		ASSERT( _IsAsync() );
		CHECK_ERR( _IsRecordingNewCommands(), void());

		_renderCommands->AddBarriers( barriers );
	}


}	// AE::Graphics
