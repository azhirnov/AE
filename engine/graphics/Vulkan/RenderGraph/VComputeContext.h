// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef AE_ENABLE_VULKAN
# include "graphics/Vulkan/RenderGraph/VTransferContext.h"

namespace AE::Graphics::_hidden_
{
	
	//
	// Vulkan Direct Compute Context implementation
	//

	template <typename ResMngrType>
	class _VDirectComputeCtx : public _VDirectTransferCtx<ResMngrType>
	{
	// variables
	protected:
		struct {
			VkPipelineLayout	pplnLayout		= VK_NULL_HANDLE;
			VkPipeline			pipeline		= VK_NULL_HANDLE;
		}					_computeStates;


	// methods
	public:
		explicit _VDirectComputeCtx (RC<VCommandBatch> batch) : _VDirectTransferCtx<ResMngrType>{batch} {}
		~_VDirectComputeCtx () override {}

		void  BindDescriptorSet (uint index, VkDescriptorSet ds, ArrayView<uint> dynamicOffsets);
		void  DispatchIndirect (VkBuffer buffer, Bytes offset);

	protected:
		void  _BindComputePipeline (VkPipeline ppln, VkPipelineLayout layout);
		void  _PushComputeConstant (Bytes offset, Bytes size, const void *values, EShaderStages stages);

		void  _Dispatch (const uint3 &groupCount);
		void  _DispatchBase (const uint3 &baseGroup, const uint3 &groupCount);
	};



	//
	// Vulkan Indirect Compute Context implementation
	//
	
	class _VBaseIndirectComputeCtx : public _VBaseIndirectTrasferCtx
	{
	// types
	protected:
		struct BindComputeDescriptorSetCmd : BaseCmd
		{
			VkPipelineLayout	layout;
			VkDescriptorSet		destSet;
			ushort				index;
			ushort				dynamicOffsetCount;
			//uint				dynamicOffsets[];
		};

		struct BindComputePipelineCmd : BaseCmd
		{
			VkPipeline			pipeline;
		};

		struct PushComputeConstantCmd : BaseCmd
		{
			VkPipelineLayout	layout;
			uint				offset;
			uint				size;
			VkShaderStageFlags	stages;
			//ubyte				data[];
		};

		struct DispatchCmd : BaseCmd
		{
			uint		groupCount[3];
		};

		struct DispatchBaseCmd : BaseCmd
		{
			uint		baseGroup[3];
			uint		groupCount[3];
		};

		struct DispatchIndirectCmd : BaseCmd
		{
			VkBuffer	buffer;
			Bytes		offset;
		};
		
		using ComputeCommands_t = TypeListUtils::Merge< TransferCommands_t,
										TypeList< BindComputeDescriptorSetCmd, BindComputePipelineCmd, PushComputeConstantCmd, DispatchCmd, DispatchBaseCmd, DispatchIndirectCmd >>;

		static constexpr usize	_MaxCmdAlign2	= ComputeCommands_t::ForEach_Max< TypeListUtils::GetTypeAlign >();
		STATIC_ASSERT( _MaxCmdAlign1 == _MaxCmdAlign2 );
		STATIC_ASSERT( VBakedCommands::Allocator_t::Align >= _MaxCmdAlign2 );


	// variables
	protected:
		struct {
			VkPipelineLayout	pplnLayout		= VK_NULL_HANDLE;
			VkPipeline			pipeline		= VK_NULL_HANDLE;
		}					_computeStates;


	// methods
	public:
		explicit _VBaseIndirectComputeCtx (RC<VCommandBatch> batch) : _VBaseIndirectTrasferCtx{batch} {}
		~_VBaseIndirectComputeCtx () override {}

		void  BindDescriptorSet (uint index, VkDescriptorSet ds, ArrayView<uint> dynamicOffsets);
		void  DispatchIndirect (VkBuffer buffer, Bytes offset);


	protected:
		void  _BindComputePipeline (VkPipeline ppln, VkPipelineLayout layout);
		void  _PushComputeConstant (Bytes offset, Bytes size, const void *values, EShaderStages stages);

		void  _Dispatch (const uint3 &groupCount);
		void  _DispatchBase (const uint3 &baseGroup, const uint3 &groupCount);

		static bool  _Execute (VulkanDeviceFn fn, VkCommandBuffer cmdbuf, void* root);
		static bool  _ProcessComputeCmd (VulkanDeviceFn fn, VkCommandBuffer cmdbuf, const BaseCmd* cmd);

		template <typename CmdType, typename ...DynamicTypes>
		ND_ CmdType&  _CreateCmd (usize dynamicArraySize = 0);
		
		template <usize I, typename TL>
		static constexpr Bytes  _CalcCmdSize (Bytes size, usize dynamicArraySize);
	};



	//
	// Vulkan Compute Context implementation
	//
	
	template <typename CtxImpl, typename BaseInterface>
	class _VComputeContextImpl : public _VTransferContextImpl<CtxImpl, BaseInterface>
	{
	// types
	private:
		using BaseCtx	= _VTransferContextImpl< CtxImpl, BaseInterface >;
		using RawCtx	= CtxImpl;

	protected:
		static constexpr uint	_LocalArraySize			= BaseCtx::_LocalArraySize;

	public:
		static constexpr bool	IsComputeContext		= true;
		static constexpr bool	IsVulkanComputeContext	= true;
		static constexpr bool	HasAutoSync				= CtxImpl::HasAutoSync;
		

	// methods
	public:
		explicit _VComputeContextImpl (RC<VCommandBatch> batch) : BaseCtx{batch} {}
		~_VComputeContextImpl () override {}

		using RawCtx::BindDescriptorSet;

		void  BindPipeline (ComputePipelineID ppln) override final;
		void  DescriptorsBarrier (DescriptorSetID ds, ArrayView<uint> dynamicOffsets) override final;
		void  BindDescriptorSet (uint index, DescriptorSetID ds, ArrayView<uint> dynamicOffsets) override final;
		void  PushConstant (Bytes offset, Bytes size, const void *values, EShaderStages stages) override final;

		using RawCtx::DispatchIndirect;

		void  Dispatch (const uint3 &groupCount) override final;
		void  DispatchIndirect (BufferID buffer, Bytes offset) override final;
		void  DispatchBase (const uint3 &baseGroup, const uint3 &groupCount) override final;
	};
//-----------------------------------------------------------------------------


	
/*
=================================================
	BindPipeline
=================================================
*/
	template <typename C, typename I>
	void  _VComputeContextImpl<C,I>::BindPipeline (ComputePipelineID ppln)
	{
		auto*	cppln = this->_resMngr.Get( ppln );
		CHECK_ERRV( cppln );

		RawCtx::_BindComputePipeline( cppln->Handle(), cppln->Layout() );
	}
	
/*
=================================================
	DescriptorsBarrier
=================================================
*/
	template <typename C, typename I>
	void  _VComputeContextImpl<C,I>::DescriptorsBarrier (DescriptorSetID ds, ArrayView<uint> dynamicOffsets)
	{
		this->_resMngr.DescriptorsBarrier( ds, dynamicOffsets );
	}
	
/*
=================================================
	BindDescriptorSet
=================================================
*/
	template <typename C, typename I>
	void  _VComputeContextImpl<C,I>::BindDescriptorSet (uint index, DescriptorSetID ds, ArrayView<uint> dynamicOffsets)
	{
		auto*	desc_set = this->_resMngr.Get( ds );
		CHECK_ERRV( desc_set );
		
		this->_resMngr.DescriptorsBarrier( ds, dynamicOffsets );

		RawCtx::BindDescriptorSet( index, desc_set->Handle(), dynamicOffsets );
	}

/*
=================================================
	PushConstant
=================================================
*/
	template <typename C, typename I>
	void  _VComputeContextImpl<C,I>::PushConstant (Bytes offset, Bytes size, const void *values, EShaderStages stages)
	{
		RawCtx::_PushComputeConstant( offset, size, values, stages );
	}
	
/*
=================================================
	Dispatch
=================================================
*/
	template <typename C, typename I>
	void  _VComputeContextImpl<C,I>::Dispatch (const uint3 &groupCount)
	{
		this->CommitBarriers();
		RawCtx::_Dispatch( groupCount );
	}
	
/*
=================================================
	DispatchIndirect
=================================================
*/
	template <typename C, typename I>
	void  _VComputeContextImpl<C,I>::DispatchIndirect (BufferID buffer, Bytes offset)
	{
		auto*	buf = this->_resMngr.Get( buffer );
		CHECK_ERRV( buf );
		ASSERT( offset + sizeof(VDispatchIndirectCommand) <= buf->Size() );

		if constexpr( HasAutoSync )
		{
			this->_resMngr.AddBuffer( buf, EResourceState::IndirectBuffer, offset, SizeOf<VDispatchIndirectCommand> );
			this->_resMngr.FlushBarriers();
		}
		this->CommitBarriers();

		RawCtx::DispatchIndirect( buf->Handle(), offset );
	}

/*
=================================================
	DispatchBase
=================================================
*/
	template <typename C, typename I>
	void  _VComputeContextImpl<C,I>::DispatchBase (const uint3 &baseGroup, const uint3 &groupCount)
	{
		this->CommitBarriers();
		RawCtx::_DispatchBase( baseGroup, groupCount );
	}
//-----------------------------------------------------------------------------

	

/*
=================================================
	BindDescriptorSet
=================================================
*/
	template <typename RM>
	void  _VDirectComputeCtx<RM>::BindDescriptorSet (uint index, VkDescriptorSet ds, ArrayView<uint> dynamicOffsets)
	{
		ASSERT( _computeStates.pplnLayout );
		ASSERT( ds != VK_NULL_HANDLE );

		this->vkCmdBindDescriptorSets( this->_cmdbuf.Get(), VK_PIPELINE_BIND_POINT_COMPUTE, _computeStates.pplnLayout, index, 1, &ds, uint(dynamicOffsets.size()), dynamicOffsets.data() );
	}
	
/*
=================================================
	_BindComputePipeline
=================================================
*/
	template <typename RM>
	void  _VDirectComputeCtx<RM>::_BindComputePipeline (VkPipeline ppln, VkPipelineLayout layout)
	{
		if ( _computeStates.pipeline == ppln )
			return;

		_computeStates.pipeline		= ppln;
		_computeStates.pplnLayout	= layout;
		this->vkCmdBindPipeline( this->_cmdbuf.Get(), VK_PIPELINE_BIND_POINT_COMPUTE, _computeStates.pipeline );
	}
	
/*
=================================================
	_PushComputeConstant
=================================================
*/
	template <typename RM>
	void  _VDirectComputeCtx<RM>::_PushComputeConstant (Bytes offset, Bytes size, const void *values, EShaderStages stages)
	{
		ASSERT( _computeStates.pplnLayout );
		ASSERT( size > 0_b and values != null );

		this->vkCmdPushConstants( this->_cmdbuf.Get(), _computeStates.pplnLayout, VEnumCast(stages), uint(offset), uint(size), values );
	}
	
/*
=================================================
	_Dispatch
=================================================
*/
	template <typename RM>
	void  _VDirectComputeCtx<RM>::_Dispatch (const uint3 &groupCount)
	{
		ASSERT( _computeStates.pipeline );
		ASSERT(All( groupCount >= uint(1) ));

		this->vkCmdDispatch( this->_cmdbuf.Get(), groupCount.x, groupCount.y, groupCount.z );
	}
	
/*
=================================================
	DispatchIndirect
=================================================
*/
	template <typename RM>
	void  _VDirectComputeCtx<RM>::DispatchIndirect (VkBuffer buffer, Bytes offset)
	{
		ASSERT( _computeStates.pipeline );
		ASSERT( buffer != VK_NULL_HANDLE );

		this->vkCmdDispatchIndirect( this->_cmdbuf.Get(), buffer, VkDeviceSize(offset) );
	}
	
/*
=================================================
	_DispatchBase
=================================================
*/
	template <typename RM>
	void  _VDirectComputeCtx<RM>::_DispatchBase (const uint3 &baseGroup, const uint3 &groupCount)
	{
		ASSERT( this->_resMngr.GetDevice().GetFeatures().dispatchBase );
		ASSERT( _computeStates.pipeline );
		ASSERT(All( groupCount >= uint(1) ));
		
		this->vkCmdDispatchBaseKHR( this->_cmdbuf.Get(), baseGroup.x, baseGroup.y, baseGroup.z, groupCount.x, groupCount.y, groupCount.z );
	}


}	// AE::Graphics::_hidden_

#endif	// AE_ENABLE_VULKAN
