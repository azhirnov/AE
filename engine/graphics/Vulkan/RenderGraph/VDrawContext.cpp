// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#ifdef AE_ENABLE_VULKAN
# include "graphics/Vulkan/RenderGraph/VDrawContext.h"

namespace AE::Graphics::_hidden_
{

/*
=================================================
	constructor
=================================================
*/
	_VBaseIndirectDrawCtx::_VBaseIndirectDrawCtx (const VCommandBatch& batch)
	{
		_allocator.SetBlockSize( 4_Kb );
	}
	
/*
=================================================
	Prepare
=================================================
*/
	VBakedCommands  _VBaseIndirectDrawCtx::Prepare ()
	{
		Bytes	size;
		for (auto& block : _allocator.GetBlocks()) {
			size += block.size;
		}

		void*	ptr	= VBakedCommands::Allocator_t::Allocate( size );
		Bytes	offset;
		
		for (auto& block : _allocator.GetBlocks())
		{
			MemCopy( ptr + offset, block.ptr, block.size );
			offset += block.size;
			ASSERT( IsAligned( offset, _MaxCmdAlign ));
		}
		ASSERT( offset == size );

		return VBakedCommands{ ptr, &_VBaseIndirectDrawCtx::_Execute };
	}

/*
=================================================
	_CreateCmd
=================================================
*/
	template <typename CmdType, typename ...DynamicTypes>
	CmdType&  _VBaseIndirectDrawCtx::_CreateCmd (usize dynamicArraySize)
	{
		Bytes	size	= AlignToLarger( _CalcCmdSize< 0, TypeList<DynamicTypes...> >( SizeOf<CmdType>, dynamicArraySize ), _MaxCmdAlign );
		auto*	cmd		= Cast<CmdType>( _allocator.Alloc( size, Bytes{_MaxCmdAlign} ));
		DEBUG_ONLY(
			cmd->_magicNumber = BaseCmd::MAGIC;
		)

		cmd->_commandID = CheckCast<ushort>( AllCommands_t::Index< CmdType >);
		cmd->_size		= CheckCast<ushort>( size );
		return *cmd;
	}
	
/*
=================================================
	_CalcCmdSize
=================================================
*/
	template <usize I, typename TL>
	constexpr Bytes  _VBaseIndirectDrawCtx::_CalcCmdSize (Bytes size, usize dynamicArraySize)
	{
		if constexpr( I < TL::Count )
		{
			using T = typename TL::template Get<I>;
			return _CalcCmdSize< I+1, TL >( AlignToLarger( size, alignof(T) ) + sizeof(T) * dynamicArraySize, dynamicArraySize ); 
		}
		else
			return size;
	}

/*
=================================================
	_ResetStates
=================================================
*/
	void  _VBaseIndirectDrawCtx::_ResetStates ()
	{
		_states.pplnLayout	= VK_NULL_HANDLE;
		_states.pipeline	= VK_NULL_HANDLE;

		_states.indexBuffer	= VK_NULL_HANDLE;
		_states.indexOffset	= 0;
	}
	
/*
=================================================
	_BindGraphicsPipeline
=================================================
*/
	void  _VBaseIndirectDrawCtx::_BindGraphicsPipeline (VkPipeline ppln, VkPipelineLayout layout)
	{
		if ( _states.pipeline == ppln )
			return;

		_states.pipeline	= ppln;
		_states.pplnLayout	= layout;
	}
	
/*
=================================================
	BindDescriptorSet
=================================================
*/
	void  _VBaseIndirectDrawCtx::BindDescriptorSet (uint index, VkDescriptorSet ds, ArrayView<uint> dynamicOffsets)
	{
		CHECK_ERRV( _states.pplnLayout );

		auto&	cmd		= _CreateCmd< BindDescriptorSetCmd, uint >( dynamicOffsets.size() );
		auto*	offsets = Cast<uint>( AlignToLarger( static_cast<void*>(&cmd + 1), AlignOf<uint> ));

		cmd.ds					= ds;
		cmd.index				= ushort(index);
		cmd.dynamicOffsetsCount	= ushort(dynamicOffsets.size());
		MemCopy( OUT offsets, dynamicOffsets.data(), ArraySizeOf(dynamicOffsets) );
	}

/*
=================================================
	_PushGraphicsConstant
=================================================
*/
	void  _VBaseIndirectDrawCtx::_PushGraphicsConstant (Bytes offset, Bytes size, const void *values, VkShaderStageFlags stages)
	{
		ASSERT( IsAligned( size, sizeof(uint) ));

		auto&	cmd		= _CreateCmd< PushConstantCmd, ubyte >( usize(size) );
		auto*	data	= Cast<uint>( AlignToLarger( static_cast<void*>(&cmd + 1), AlignOf<uint> ));

		cmd.stages	= stages;
		cmd.offset	= CheckCast<ushort>( offset );
		cmd.size	= CheckCast<ushort>( size );
		MemCopy( OUT data, values, size );
	}

/*
=================================================
	_SetScissor
=================================================
*/
	void  _VBaseIndirectDrawCtx::_SetScissor (uint first, ArrayView<VkRect2D> scissors)
	{
		auto&	cmd	= _CreateCmd< SetScissorCmd, VkRect2D >( scissors.size() );
		auto*	dst	= Cast<VkRect2D>( AlignToLarger( static_cast<void*>(&cmd + 1), AlignOf<VkRect2D> ));

		cmd.first	= ushort(first);
		cmd.count	= ushort(scissors.size());
		MemCopy( OUT dst, scissors.data(), ArraySizeOf(scissors) );
	}
	
/*
=================================================
	_SetDepthBias
=================================================
*/
	void  _VBaseIndirectDrawCtx::_SetDepthBias (float depthBiasConstantFactor, float depthBiasClamp, float depthBiasSlopeFactor)
	{
		auto&	cmd	= _CreateCmd< SetDepthBiasCmd >();
		cmd.depthBiasConstantFactor	= depthBiasConstantFactor;
		cmd.depthBiasClamp			= depthBiasClamp;
		cmd.depthBiasSlopeFactor	= depthBiasSlopeFactor;
	}
	
/*
=================================================
	_SetDepthBounds
=================================================
*/
	void  _VBaseIndirectDrawCtx::_SetDepthBounds (float minDepthBounds, float maxDepthBounds)
	{
		auto&	cmd	= _CreateCmd< SetDepthBoundsCmd >();
		cmd.minDepthBounds	= minDepthBounds;
		cmd.maxDepthBounds	= maxDepthBounds;
	}
	
/*
=================================================
	_SetStencilCompareMask
=================================================
*/
	void  _VBaseIndirectDrawCtx::_SetStencilCompareMask (VkStencilFaceFlagBits faceMask, uint compareMask)
	{
		auto&	cmd	= _CreateCmd< SetStencilCompareMaskCmd >();
		cmd.faceMask	= faceMask;
		cmd.compareMask	= compareMask;
	}
	
/*
=================================================
	_SetStencilWriteMask
=================================================
*/
	void  _VBaseIndirectDrawCtx::_SetStencilWriteMask (VkStencilFaceFlagBits faceMask, uint writeMask)
	{
		auto&	cmd	= _CreateCmd< SetStencilWriteMaskCmd >();
		cmd.faceMask	= faceMask;
		cmd.writeMask	= writeMask;
	}
	
/*
=================================================
	_SetStencilReference
=================================================
*/
	void  _VBaseIndirectDrawCtx::_SetStencilReference (VkStencilFaceFlagBits faceMask, uint reference)
	{
		auto&	cmd	= _CreateCmd< SetStencilReferenceCmd >();
		cmd.faceMask	= faceMask;
		cmd.reference	= reference;
	}
	
/*
=================================================
	_SetBlendConstants
=================================================
*/
	void  _VBaseIndirectDrawCtx::_SetBlendConstants (const RGBA32f &color)
	{
		auto&	cmd	= _CreateCmd< SetBlendConstantsCmd >();
		cmd.color	= color;
	}
	
/*
=================================================
	BindIndexBuffer
=================================================
*/
	void  _VBaseIndirectDrawCtx::BindIndexBuffer (VkBuffer buffer, Bytes offset, VkIndexType indexType)
	{
		if ( (_states.indexBuffer == buffer) & (_states.indexOffset == offset) )
			return;
		
		auto&	cmd	= _CreateCmd< BindIndexBufferCmd >();

		cmd.buffer		= _states.indexBuffer	= buffer;
		cmd.offset		= _states.indexOffset	= VkDeviceSize(offset);
		cmd.indexType	= indexType;
	}
	
/*
=================================================
	_BindVertexBuffers
=================================================
*/
	void  _VBaseIndirectDrawCtx::_BindVertexBuffers (uint firstBinding, ArrayView<VkBuffer> buffers, ArrayView<VkDeviceSize> offsets)
	{
		ASSERT( buffers.size() );
		ASSERT( buffers.size() == offsets.size() );

		auto&	cmd			= _CreateCmd< BindVertexBuffersCmd, VkBuffer, VkDeviceSize >( buffers.size() );
		auto*	dst_buffers	= Cast<VkBuffer>( AlignToLarger( static_cast<void*>(&cmd + 1), AlignOf<VkBuffer> ));
		auto*	dst_offsets	= Cast<VkDeviceSize>( AlignToLarger( static_cast<void*>(dst_buffers + buffers.size()), AlignOf<VkDeviceSize> ));

		cmd.firstBinding	= ushort(firstBinding);
		cmd.count			= ushort(buffers.size());
		MemCopy( OUT dst_buffers, buffers.data(), ArraySizeOf(buffers) );
		MemCopy( OUT dst_offsets, offsets.data(), ArraySizeOf(offsets) );
	}

/*
=================================================
	_Draw
=================================================
*/
	void  _VBaseIndirectDrawCtx::_Draw (uint vertexCount,
										uint instanceCount,
										uint firstVertex,
										uint firstInstance)
	{
		auto&	cmd	= _CreateCmd< DrawCmd >();
		cmd.vertexCount		= vertexCount;
		cmd.instanceCount	= instanceCount;
		cmd.firstVertex		= firstVertex;
		cmd.firstInstance	= firstInstance;
	}
	
/*
=================================================
	_DrawIndexed
=================================================
*/
	void  _VBaseIndirectDrawCtx::_DrawIndexed (uint indexCount,
											   uint instanceCount,
											   uint firstIndex,
											   int  vertexOffset,
											   uint firstInstance)
	{
		auto&	cmd	= _CreateCmd< DrawIndexedCmd >();
		cmd.indexCount		= indexCount;
		cmd.instanceCount	= instanceCount;
		cmd.firstIndex		= firstIndex;
		cmd.vertexOffset	= vertexOffset;
		cmd.firstInstance	= firstInstance;
	}
	
/*
=================================================
	DrawIndirect
=================================================
*/
	void  _VBaseIndirectDrawCtx::DrawIndirect (VkBuffer	indirectBuffer,
											   Bytes	indirectBufferOffset,
											   uint		drawCount,
											   Bytes	stride)
	{
		auto&	cmd	= _CreateCmd< DrawIndirectCmd >();
		cmd.indirectBuffer		= indirectBuffer;
		cmd.indirectBufferOffset= VkDeviceSize(indirectBufferOffset);
		cmd.drawCount			= drawCount;
		cmd.stride				= uint(stride);
	}
	
/*
=================================================
	DrawIndexedIndirect
=================================================
*/
	void  _VBaseIndirectDrawCtx::DrawIndexedIndirect (VkBuffer	indirectBuffer,
													  Bytes		indirectBufferOffset,
													  uint		drawCount,
													  Bytes		stride)
	{
		auto&	cmd	= _CreateCmd< DrawIndexedIndirectCmd >();
		cmd.indirectBuffer		= indirectBuffer;
		cmd.indirectBufferOffset= VkDeviceSize(indirectBufferOffset);
		cmd.drawCount			= drawCount;
		cmd.stride				= uint(stride);
	}
	
/*
=================================================
	DrawIndexed
=================================================
*/
	void  _VBaseIndirectDrawCtx::DrawIndirectCount (VkBuffer	indirectBuffer,
														 Bytes		indirectBufferOffset,
														 VkBuffer	countBuffer,
														 Bytes		countBufferOffset,
														 uint		maxDrawCount,
														 Bytes		stride)
	{
		auto&	cmd	= _CreateCmd< DrawIndirectCountCmd >();
		cmd.indirectBuffer		= indirectBuffer;
		cmd.indirectBufferOffset= VkDeviceSize(indirectBufferOffset);
		cmd.countBuffer			= countBuffer;
		cmd.countBufferOffset	= VkDeviceSize(countBufferOffset);
		cmd.maxDrawCount		= maxDrawCount;
		cmd.stride				= uint(stride);
	}
	
/*
=================================================
	DrawIndexed
=================================================
*/
	void  _VBaseIndirectDrawCtx::DrawIndexedIndirectCount (VkBuffer	indirectBuffer,
															    Bytes		indirectBufferOffset,
															    VkBuffer	countBuffer,
															    Bytes		countBufferOffset,
															    uint		maxDrawCount,
															    Bytes		stride)
	{
		auto&	cmd	= _CreateCmd< DrawIndexedIndirectCountCmd >();
		cmd.indirectBuffer		= indirectBuffer;
		cmd.indirectBufferOffset= VkDeviceSize(indirectBufferOffset);
		cmd.countBuffer			= countBuffer;
		cmd.countBufferOffset	= VkDeviceSize(countBufferOffset);
		cmd.maxDrawCount		= maxDrawCount;
		cmd.stride				= uint(stride);
	}

/*
=================================================
	_DrawMeshTasksNV
=================================================
*/
	void  _VBaseIndirectDrawCtx::_DrawMeshTasksNV (uint taskCount, uint firstTask)
	{
	#ifdef VK_NV_mesh_shader
		
		auto&	cmd	= _CreateCmd< DrawMeshTasksNVCmd >();
		cmd.taskCount	= taskCount;
		cmd.firstTask	= firstTask;

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
	void  _VBaseIndirectDrawCtx::DrawMeshTasksIndirectNV (VkBuffer	indirectBuffer,
														  Bytes		indirectBufferOffset,
														  uint		drawCount,
														  Bytes		stride)
	{
	#ifdef VK_NV_mesh_shader

		auto&	cmd	= _CreateCmd< DrawMeshTasksIndirectNVCmd >();
		cmd.indirectBuffer		= indirectBuffer;
		cmd.indirectBufferOffset= VkDeviceSize(indirectBufferOffset);
		cmd.drawCount			= drawCount;
		cmd.stride				= uint(stride);

	#else
		Unused( indirectBuffer, indirectBufferOffset, drawCount, stride );
		AE_LOGE( "mesh shader is not supported!" );
	#endif
	}
	
/*
=================================================
	DrawMeshTasksIndirectCountNV
=================================================
*/
	void  _VBaseIndirectDrawCtx::DrawMeshTasksIndirectCountNV (VkBuffer	indirectBuffer,
															   Bytes	indirectBufferOffset,
															   VkBuffer	countBuffer,
															   Bytes	countBufferOffset,
															   uint		maxDrawCount,
															   Bytes	stride)
	{
	#ifdef VK_NV_mesh_shader

		auto&	cmd	= _CreateCmd< DrawMeshTasksIndirectCountNVCmd >();
		cmd.indirectBuffer		= indirectBuffer;
		cmd.indirectBufferOffset= VkDeviceSize(indirectBufferOffset);
		cmd.countBuffer			= countBuffer;
		cmd.countBufferOffset	= VkDeviceSize(countBufferOffset);
		cmd.maxDrawCount		= maxDrawCount;
		cmd.stride				= uint(stride);

	#else
		Unused( indirectBuffer, indirectBufferOffset, countBuffer, countBufferOffset, maxDrawCount, stride );
		AE_LOGE( "mesh shader is not supported!" );
	#endif
	}
//-----------------------------------------------------------------------------

	

/*
=================================================
	_Execute
=================================================
*/
	bool  _VBaseIndirectDrawCtx::_Execute (VulkanDeviceFn fn, VkCommandBuffer cmdbuf, void* root)
	{
		BaseCmd const *		base		= Cast<BaseCmd>( root );
		VkPipelineLayout	ppln_layout	= VK_NULL_HANDLE;

		if ( base == null )
			return false;

		for (; base;)
		{
			switch ( base->_commandID )
			{
				case AllCommands_t::Index<BindPipelineCmd> :
				{
					auto&	cmd = *Cast<BindPipelineCmd>( base );
					fn.vkCmdBindPipeline( cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, cmd.ppln );
					ppln_layout = cmd.layout;
					break;
				}
				case AllCommands_t::Index<BindDescriptorSetCmd> :
				{
					auto&	cmd		= *Cast<BindDescriptorSetCmd>( base );
					auto*	offsets = Cast<uint>( AlignToLarger( static_cast<const void *>(&cmd + 1), AlignOf<uint> ));
					fn.vkCmdBindDescriptorSets( cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, ppln_layout, cmd.index, 1, &cmd.ds, cmd.dynamicOffsetsCount, offsets );
					break;
				}
				case AllCommands_t::Index<PushConstantCmd> :
				{
					auto&	cmd		= *Cast<PushConstantCmd>( base );
					auto*	data	= Cast<uint>( AlignToLarger( static_cast<const void *>(&cmd + 1), AlignOf<uint> ));
					fn.vkCmdPushConstants( cmdbuf, ppln_layout, cmd.stages, cmd.offset, cmd.size, data );
					break;
				}
				case AllCommands_t::Index<SetScissorCmd> :
				{
					auto&	cmd		= *Cast<SetScissorCmd>( base );
					auto*	scissor	= Cast<VkRect2D>( AlignToLarger( static_cast<const void *>(&cmd + 1), AlignOf<VkRect2D> ));
					fn.vkCmdSetScissor( cmdbuf, cmd.first, cmd.count, scissor );
					break;
				}
				case AllCommands_t::Index<SetDepthBiasCmd> :
				{
					auto&	cmd = *Cast<SetDepthBiasCmd>( base );
					fn.vkCmdSetDepthBias( cmdbuf, cmd.depthBiasConstantFactor, cmd.depthBiasClamp, cmd.depthBiasSlopeFactor );
					break;
				}
				case AllCommands_t::Index<SetDepthBoundsCmd> :
				{
					auto&	cmd = *Cast<SetDepthBoundsCmd>( base );
					fn.vkCmdSetDepthBounds( cmdbuf, cmd.minDepthBounds, cmd.maxDepthBounds );
					break;
				}
				case AllCommands_t::Index<SetStencilCompareMaskCmd> :
				{
					auto&	cmd = *Cast<SetStencilCompareMaskCmd>( base );
					fn.vkCmdSetStencilCompareMask( cmdbuf, cmd.faceMask, cmd.compareMask );
					break;
				}
				case AllCommands_t::Index<SetStencilWriteMaskCmd> :
				{
					auto&	cmd = *Cast<SetStencilWriteMaskCmd>( base );
					fn.vkCmdSetStencilWriteMask( cmdbuf, cmd.faceMask, cmd.writeMask );
					break;
				}
				case AllCommands_t::Index<SetStencilReferenceCmd> :
				{
					auto&	cmd = *Cast<SetStencilReferenceCmd>( base );
					fn.vkCmdSetStencilReference( cmdbuf, cmd.faceMask, cmd.reference );
					break;
				}
				case AllCommands_t::Index<SetBlendConstantsCmd> :
				{
					auto&	cmd = *Cast<SetBlendConstantsCmd>( base );
					fn.vkCmdSetBlendConstants( cmdbuf, cmd.color.data() );
					break;
				}
				case AllCommands_t::Index<BindIndexBufferCmd> :
				{
					auto&	cmd = *Cast<BindIndexBufferCmd>( base );
					fn.vkCmdBindIndexBuffer( cmdbuf, cmd.buffer, cmd.offset, cmd.indexType );
					break;
				}
				case AllCommands_t::Index<BindVertexBuffersCmd> :
				{
					auto&	cmd		= *Cast<BindVertexBuffersCmd>( base );
					auto*	buffers	= Cast<VkBuffer>(     AlignToLarger( static_cast<const void *>(&cmd + 1),			 AlignOf<VkBuffer>     ));
					auto*	offsets	= Cast<VkDeviceSize>( AlignToLarger( static_cast<const void *>(buffers + cmd.count), AlignOf<VkDeviceSize> ));
					fn.vkCmdBindVertexBuffers( cmdbuf, cmd.firstBinding, cmd.count, buffers, offsets );
					break;
				}
				case AllCommands_t::Index<DrawCmd> :
				{
					auto&	cmd = *Cast<DrawCmd>( base );
					fn.vkCmdDraw( cmdbuf, cmd.vertexCount, cmd.instanceCount, cmd.firstVertex, cmd.firstInstance );
					break;
				}
				case AllCommands_t::Index<DrawIndexedCmd> :
				{
					auto&	cmd = *Cast<DrawIndexedCmd>( base );
					fn.vkCmdDrawIndexed( cmdbuf, cmd.indexCount, cmd.instanceCount, cmd.firstIndex, cmd.vertexOffset, cmd.firstInstance );
					break;
				}
				case AllCommands_t::Index<DrawIndirectCmd> :
				{
					auto&	cmd = *Cast<DrawIndirectCmd>( base );
					fn.vkCmdDrawIndirect( cmdbuf, cmd.indirectBuffer, cmd.indirectBufferOffset, cmd.drawCount, cmd.stride );
					break;
				}
				case AllCommands_t::Index<DrawIndexedIndirectCmd> :
				{
					auto&	cmd = *Cast<DrawIndexedIndirectCmd>( base );
					fn.vkCmdDrawIndexedIndirect( cmdbuf, cmd.indirectBuffer, cmd.indirectBufferOffset, cmd.drawCount, cmd.stride );
					break;
				}
				case AllCommands_t::Index<DrawIndirectCountCmd> :
				{
					auto&	cmd = *Cast<DrawIndirectCountCmd>( base );
					fn.vkCmdDrawIndirectCountKHR( cmdbuf, cmd.indirectBuffer, cmd.indirectBufferOffset, cmd.countBuffer, cmd.countBufferOffset, cmd.maxDrawCount, cmd.stride );
					break;
				}
				case AllCommands_t::Index<DrawIndexedIndirectCountCmd> :
				{
					auto&	cmd = *Cast<DrawIndexedIndirectCountCmd>( base );
					fn.vkCmdDrawIndexedIndirectCountKHR( cmdbuf, cmd.indirectBuffer, cmd.indirectBufferOffset, cmd.countBuffer, cmd.countBufferOffset, cmd.maxDrawCount, cmd.stride );
					break;
				}

			#ifdef VK_NV_mesh_shader
				case AllCommands_t::Index<DrawMeshTasksNVCmd> :
				{
					auto&	cmd = *Cast<DrawMeshTasksNVCmd>( base );
					fn.vkCmdDrawMeshTasksNV( cmdbuf, cmd.taskCount, cmd.firstTask );
					break;
				}
				case AllCommands_t::Index<DrawMeshTasksIndirectNVCmd> :
				{
					auto&	cmd = *Cast<DrawMeshTasksIndirectNVCmd>( base );
					fn.vkCmdDrawMeshTasksIndirectNV( cmdbuf, cmd.indirectBuffer, cmd.indirectBufferOffset, cmd.drawCount, cmd.stride );
					break;
				}
				case AllCommands_t::Index<DrawMeshTasksIndirectCountNVCmd> :
				{
					auto&	cmd = *Cast<DrawMeshTasksIndirectCountNVCmd>( base );
					fn.vkCmdDrawMeshTasksIndirectCountNV( cmdbuf, cmd.indirectBuffer, cmd.indirectBufferOffset, cmd.countBuffer, cmd.countBufferOffset, cmd.maxDrawCount, cmd.stride );
					break;
				}
			#endif // VK_NV_mesh_shader

				default :
					AE_LOGE( "unknown indirect buffer command" );
					break;
			}
			base = static_cast<const BaseCmd *>( static_cast<const void *>(base) + Bytes{base->_size} );
		}

		return true;
	}


}	// AE::Graphics::_hidden_

#endif	// AE_ENABLE_VULKAN
