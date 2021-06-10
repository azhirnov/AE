// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef AE_ENABLE_VULKAN
# include "graphics/Vulkan/RenderGraph/VComputeContext.h"

namespace AE::Graphics::_hidden_
{
	
/*
=================================================
	_CreateCmd
=================================================
*/
	template <typename CmdType, typename ...DynamicTypes>
	CmdType&  _VBaseIndirectComputeCtx::_CreateCmd (usize dynamicArraySize)
	{
		Bytes	size	= AlignToLarger( _CalcCmdSize< 0, TypeList<DynamicTypes...> >( SizeOf<CmdType>, dynamicArraySize ), _MaxCmdAlign2 );
		auto*	cmd		= Cast<CmdType>( _allocator.Alloc( size, Bytes{_MaxCmdAlign2} ));
		DEBUG_ONLY(
			cmd->_magicNumber = BaseCmd::MAGIC;
		)

		cmd->_commandID = CheckCast<ushort>( ComputeCommands_t::Index< CmdType >);
		cmd->_size		= CheckCast<ushort>( size );
		return *cmd;
	}
	
/*
=================================================
	_CalcCmdSize
=================================================
*/
	template <usize I, typename TL>
	constexpr Bytes  _VBaseIndirectComputeCtx::_CalcCmdSize (Bytes size, usize dynamicArraySize)
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
	BindDescriptorSet
=================================================
*/
	void  _VBaseIndirectComputeCtx::BindDescriptorSet (uint index, VkDescriptorSet ds, ArrayView<uint> dynamicOffsets)
	{
		ASSERT( _computeStates.pplnLayout );
		ASSERT( ds != VK_NULL_HANDLE );
		
		auto&	cmd		= _CreateCmd< BindComputeDescriptorSetCmd, uint >( dynamicOffsets.size() );
		auto*	offsets	= Cast<uint>( AlignToLarger( static_cast<void*>(&cmd + 1), AlignOf<uint> ));

		cmd.layout	= _computeStates.pplnLayout;
		cmd.destSet	= ds;
		cmd.index	= ushort(index);
		MemCopy( OUT offsets, dynamicOffsets.data(), ArraySizeOf(dynamicOffsets) );
	}
	
/*
=================================================
	_BindComputePipeline
=================================================
*/
	void  _VBaseIndirectComputeCtx::_BindComputePipeline (VkPipeline ppln, VkPipelineLayout layout)
	{
		if ( _computeStates.pipeline == ppln )
			return;

		_computeStates.pipeline		= ppln;
		_computeStates.pplnLayout	= layout;
		
		auto&	cmd = _CreateCmd< BindComputePipelineCmd >();
		cmd.pipeline = ppln;
	}
	
/*
=================================================
	_PushComputeConstant
=================================================
*/
	void  _VBaseIndirectComputeCtx::_PushComputeConstant (Bytes offset, Bytes size, const void *values, EShaderStages stages)
	{
		ASSERT( _computeStates.pplnLayout );
		ASSERT( size > 0_b and values != null );
		
		auto&	cmd = _CreateCmd< PushComputeConstantCmd, ubyte >( usize(size) );
		auto*	dst	= Cast<ubyte>( AlignToLarger( static_cast<void*>(&cmd + 1), AlignOf<ubyte> ));

		cmd.layout	= _computeStates.pplnLayout;
		cmd.offset	= uint(offset);
		cmd.size	= uint(size);
		cmd.stages	= VEnumCast( stages );
		MemCopy( OUT dst, values, size );
	}
	
/*
=================================================
	_Dispatch
=================================================
*/
	void  _VBaseIndirectComputeCtx::_Dispatch (const uint3 &groupCount)
	{
		ASSERT( _computeStates.pipeline );
		ASSERT(All( groupCount >= uint(1) ));
		
		auto&	cmd = _CreateCmd< DispatchCmd >();
		MemCopy( OUT cmd.groupCount, &groupCount, Bytes::SizeOf( cmd.groupCount ));
	}
	
/*
=================================================
	DispatchIndirect
=================================================
*/
	void  _VBaseIndirectComputeCtx::DispatchIndirect (VkBuffer buffer, Bytes offset)
	{
		ASSERT( _computeStates.pipeline );
		ASSERT( buffer != VK_NULL_HANDLE );
		
		auto&	cmd = _CreateCmd< DispatchIndirectCmd >();
		cmd.buffer	= buffer;
		cmd.offset	= offset;
	}
	
/*
=================================================
	_DispatchBase
=================================================
*/
	void  _VBaseIndirectComputeCtx::_DispatchBase (const uint3 &baseGroup, const uint3 &groupCount)
	{
		ASSERT( _computeStates.pipeline );
		ASSERT(All( groupCount >= uint(1) ));
		
		auto&	cmd = _CreateCmd< DispatchBaseCmd >();
		MemCopy( OUT cmd.baseGroup,  &baseGroup,  Bytes::SizeOf( cmd.baseGroup ));
		MemCopy( OUT cmd.groupCount, &groupCount, Bytes::SizeOf( cmd.groupCount ));
	}

/*
=================================================
	_Execute
=================================================
*/
	bool  _VBaseIndirectComputeCtx::_Execute (VulkanDeviceFn fn, VkCommandBuffer cmdbuf, void* root)
	{
		BaseCmd const*	base = Cast<BaseCmd>( root );

		if ( base == null )
			return false;

		for (; base;)
		{
			if ( _ProcessTransferCmd( fn, cmdbuf, base ) or
				 _ProcessComputeCmd(  fn, cmdbuf, base ))
			{
				base = static_cast<const BaseCmd *>( static_cast<const void *>(base) + Bytes{base->_size} );
			}
			else
			if ( base->_commandID == ComputeCommands_t::Index<EndCmd> )
			{
				break;
			}
			else
			{
				AE_LOGE( "unknown indirect buffer command" );
				return false;
			}
		}
		return true;
	}
	
/*
=================================================
	_ProcessComputeCmd
=================================================
*/
	bool  _VBaseIndirectComputeCtx::_ProcessComputeCmd (VulkanDeviceFn fn, VkCommandBuffer cmdbuf, const BaseCmd* base)
	{
		switch ( base->_commandID )
		{
			case ComputeCommands_t::Index<BindComputeDescriptorSetCmd> :
			{
				auto&	cmd		= *Cast<BindComputeDescriptorSetCmd>( base );
				auto*	offsets = Cast<uint>( AlignToLarger( static_cast<const void *>(&cmd + 1), AlignOf<uint> ));
				fn.vkCmdBindDescriptorSets( cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, cmd.layout, cmd.index, 1, &cmd.destSet, cmd.dynamicOffsetCount, offsets );
				return true;
			}

			case ComputeCommands_t::Index<BindComputePipelineCmd> :
			{
				auto&	cmd = *Cast<BindComputePipelineCmd>( base );
				fn.vkCmdBindPipeline( cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, cmd.pipeline );
				return true;
			}

			case ComputeCommands_t::Index<PushComputeConstantCmd> :
			{
				auto&	cmd		= *Cast<PushComputeConstantCmd>( base );
				auto*	values	= Cast<ubyte>( AlignToLarger( static_cast<const void *>(&cmd + 1), AlignOf<ubyte> ));
				fn.vkCmdPushConstants( cmdbuf, cmd.layout, cmd.stages, cmd.offset, cmd.size, values );
				return true;
			}

			case ComputeCommands_t::Index<DispatchCmd> :
			{
				auto&	cmd = *Cast<DispatchCmd>( base );
				fn.vkCmdDispatch( cmdbuf, cmd.groupCount[0], cmd.groupCount[1], cmd.groupCount[2] );
				return true;
			}

			case ComputeCommands_t::Index<DispatchBaseCmd> :
			{
				auto&	cmd = *Cast<DispatchBaseCmd>( base );
				fn.vkCmdDispatchBaseKHR( cmdbuf, cmd.baseGroup[0], cmd.baseGroup[1], cmd.baseGroup[2], cmd.groupCount[0], cmd.groupCount[1], cmd.groupCount[2] );
				return true;
			}

			case ComputeCommands_t::Index<DispatchIndirectCmd> :
			{
				auto&	cmd = *Cast<DispatchIndirectCmd>( base );
				fn.vkCmdDispatchIndirect( cmdbuf, cmd.buffer, VkDeviceSize(cmd.offset) );
				return true;
			}
		}
		return false;
	}


}	// AE::Graphics::_hidden_

#endif	// AE_ENABLE_VULKAN
