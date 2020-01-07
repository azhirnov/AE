// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#ifdef AE_ENABLE_VULKAN

# include "graphics/Vulkan/VRenderGraph.h"
# include "graphics/Vulkan/VDevice.h"
# include "graphics/Vulkan/VEnumCast.h"

namespace AE::Graphics
{
	
	//
	// Vulkan Transfer Context
	//

	class VRenderGraph::VTransferContext final : public ITransferContext
	{
	};
//-----------------------------------------------------------------------------



	//
	// Vulkan Compute Context
	//

	class VRenderGraph::VComputeContext final : public IComputeContext
	{
	};
//-----------------------------------------------------------------------------



	//
	// Vulkan Graphics Context
	//

	class VRenderGraph::VGraphicsContext final : public IGraphicsContext
	{
	};
//-----------------------------------------------------------------------------



	//
	// Vulkan Render Context
	//

	class VRenderGraph::VRenderContext final : public VulkanDeviceFn, public IRenderContext
	{
	// variables
	private:
		VkCommandBuffer		_cmdbuf;
		VkRenderPass		_renderPass;
		VkFramebuffer		_framebuffer;
		uint				_subpassIndex;

		VkPipelineLayout	_curLayout;
		VkPipeline			_curPipeline;


	// methods
	public:
		NativeContext_t  GetNativeContext () override
		{
			VulkanContext	vctx;
			vctx.cmdBuffer		= uint64_t(_cmdbuf);
			vctx.renderPass		= uint64_t(_renderPass);
			vctx.framebuffer	= uint64_t(_framebuffer);
			vctx.subpassIndex	= _subpassIndex;
			return vctx;
		}

		void ResetStates () override
		{
		}

		void BindPipeline (GraphicsPipelineID ppln) override
		{
			vkCmdBindPipeline( _cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, _curPipeline );
		}

		void BindPipeline (MeshPipelineID ppln) override
		{
			vkCmdBindPipeline( _cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, _curPipeline );
		}
		
		void BindDescriptorSet (uint index, DescriptorSetID ds, ArrayView<uint> dynamicOffsets) override
		{
			//vkCmdBindDescriptorSets( _cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, _curLayout, index, 1, ds, uint(dynamicOffsets.size()), dynamicOffsets.data() );
		}

		void PushConstant (BytesU offset, BytesU size, const void *values, EShaderStages stages) override
		{
			vkCmdPushConstants( _cmdbuf, _curLayout, VEnumCast(stages), uint(offset), uint(size), values );
		}

		void BindIndexBuffer () override
		{
		}

		void BindVertexBuffer () override
		{
		}

		void Draw (uint vertexCount, uint instanceCount, uint firstVertex, uint firstInstance) override
		{
			vkCmdDraw( _cmdbuf, vertexCount, instanceCount, firstVertex, firstInstance );
		}

		void DrawIndexed (uint indexCount, uint instanceCount, uint firstIndex, int vertexOffset, uint firstInstance) override
		{
			vkCmdDrawIndexed( _cmdbuf, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance );
		}

		void DrawIndirect (GfxResourceID buffer, BytesU offset, uint drawCount, BytesU stride) override
		{
			CHECK( buffer.ResourceType() == GfxResourceID::EType::Buffer );
			//vkCmdDrawIndirect( _cmdbuf, buffer, VkDeviceSize(offset), drawCount, uint(stride) );
		}

		void DrawIndexedIndirect (GfxResourceID buffer, BytesU offset, uint drawCount, BytesU stride) override
		{
			CHECK( buffer.ResourceType() == GfxResourceID::EType::Buffer );
			//vkCmdDrawIndexedIndirect( _cmdbuf, buffer, VkDeviceSize(offset), drawCount, uint(stride) );
		}

		void DrawIndirectCount (GfxResourceID buffer, BytesU offset, GfxResourceID countBuffer, BytesU countBufferOffset, uint maxDrawCount, BytesU stride) override
		{
			CHECK( buffer.ResourceType() == GfxResourceID::EType::Buffer );
			CHECK( countBuffer.ResourceType() == GfxResourceID::EType::Buffer );
			//vkCmdDrawIndirectCount( _cmdbuf, buffer, VkDeviceSize(offset), countBuffer, VkDeviceSize(countBufferOffset), maxDrawCount, uint(stride) );
		}

		void DrawIndexedIndirectCount (GfxResourceID buffer, BytesU offset, GfxResourceID countBuffer, BytesU countBufferOffset, uint maxDrawCount, BytesU stride) override
		{
			CHECK( buffer.ResourceType() == GfxResourceID::EType::Buffer );
			CHECK( countBuffer.ResourceType() == GfxResourceID::EType::Buffer );
			//vkCmdDrawIndexedIndirectCount( _cmdbuf, buffer, VkDeviceSize(offset), countBuffer, VkDeviceSize(countBufferOffset), maxDrawCount, uint(stride) );
		}

		void DrawMeshTasksNV (uint taskCount, uint firstTask) override
		{
			vkCmdDrawMeshTasksNV( _cmdbuf, taskCount, firstTask );
		}

		void DrawMeshTasksIndirectNV (GfxResourceID buffer, BytesU offset, uint drawCount, BytesU stride) override
		{
			CHECK( buffer.ResourceType() == GfxResourceID::EType::Buffer );
			//vkCmdDrawMeshTasksIndirectNV( _cmdbuf, buffer, VkDeviceSize(offset), drawCount, uint(stride) );
		}

		void DrawMeshTasksIndirectCountNV (GfxResourceID buffer, BytesU offset, GfxResourceID countBuffer, BytesU countBufferOffset, uint maxDrawCount, BytesU stride) override
		{
			CHECK( buffer.ResourceType() == GfxResourceID::EType::Buffer );
			CHECK( countBuffer.ResourceType() == GfxResourceID::EType::Buffer );
			//vkCmdDrawMeshTasksIndirectCountNV( _cmdbuf, buffer, VkDeviceSize(offset), countBuffer, VkDeviceSize(countBufferOffset), maxDrawCount, uint(stride) );
		}
	};
//-----------------------------------------------------------------------------


	
	struct VRenderGraph::BaseCmd
	{
		enum class EState : uint8_t {
			Initial,
			Incomplete,
			Complete,
			Pending,
		};

		EState					state;
		EQueueType				queue;
		uint16_t				inputCount;
		uint16_t				outputCount;
		GfxResourceID *			input;
		GfxResourceID *			output;
		BaseCmd **				inputCmd;
		char *					dbgName;
		Function<void (void*)>	pass;
	};

	struct VRenderGraph::RenderCmd : BaseCmd
	{
		//RenderPassSetupFn_t		setup;
		//RenderPassDrawFn_t		draw;
	};

	struct VRenderGraph::GraphicsCmd : BaseCmd
	{
	};

	struct VRenderGraph::ComputeCmd : BaseCmd
	{
	};

	struct VRenderGraph::TransferCmd : BaseCmd
	{
	};
//-----------------------------------------------------------------------------

	
/*
=================================================
	_Add
=================================================
*/
	template <typename T>
	inline T*  VRenderGraph::_Add (EQueueType			queue,
								   VirtualResources_t	input,
								   VirtualResources_t	output,
								   StringView			dbgName)
	{
		auto*	cmd = _allocator.Alloc<T>();
		CHECK_ERR( cmd );

		cmd->state			= BaseCmd::EState::Initial;
		cmd->queue			= queue;
		cmd->inputCount		= uint16_t(input.size());
		cmd->outputCount	= uint16_t(output.size());
		cmd->input			= input.size() ? _allocator.Alloc<GfxResourceID>( input.size() ) : null;
		cmd->output			= output.size() ? _allocator.Alloc<GfxResourceID>( output.size() ) : null;
		cmd->inputCmd		= input.size() ? _allocator.Alloc<BaseCmd*>( input.size() ) : null;
		cmd->dbgName		= dbgName.size() ? _allocator.Alloc<char>( dbgName.length()+1 ) : "";

		for (size_t i = 0; i < input.size(); ++i)
		{
			cmd->input[i] = input[i].first;
			_resUsage[ input[i].first.Index() ] |= input[i].second;
			CHECK_ERR( input[i].first.IsVirtual() );
		}
		for (size_t i = 0; i < output.size(); ++i)
		{
			cmd->output[i] = output[i].first;
			_resUsage[ output[i].first.Index() ] |= output[i].second;
			CHECK_ERR( output[i].first.IsVirtual() );
		}

		std::memcpy( cmd->dbgName, dbgName.data(), dbgName.length()+1 );
		DEBUG_ONLY( std::memset( cmd->inputCmd, 0xCD, input.size() * sizeof(*cmd->inputCmd) ));

		for (auto&[id, usage] : output)
		{
			auto&	dst = _resWriteCmd[ id.Index() ];
			CHECK( dst == null );
			dst = cmd;
		}

		_commands.push_back( cmd );
		return cmd;
	}

/*
=================================================
	Add
=================================================
*/
	bool VRenderGraph::Add (EQueueType				queue,
							VirtualResources_t		input,
							VirtualResources_t		output,
							RenderPassSetupFn_t&&	setup,
							RenderPassDrawFn_t&&	draw,
							StringView				dbgName)
	{
		CHECK_ERR( queue == EQueueType::Graphics );

		auto*	cmd = _Add<RenderCmd>( queue, input, output, dbgName );
		CHECK_ERR( cmd );

		//PlacementNew<RenderPassSetupFn_t>( &cmd->setup, std::move(setup) );
		//PlacementNew<RenderPassFn_t>( &cmd->draw, std::move(draw) );
		
		PlacementNew< Function< void (void *) >>(
			&cmd->pass,
			[setup = std::move(setup), draw = std::move(draw), cmd] (void* ctx)
			{
				setup( *static_cast<IGraphicsContext*>(ctx), ArrayView{cmd->input, cmd->inputCount}, ArrayView{cmd->output, cmd->outputCount} );

				VRenderContext	rctx;
				draw( rctx, ArrayView{cmd->input, cmd->inputCount}, ArrayView{cmd->output, cmd->outputCount} );
			});
		return true;
	}
	
/*
=================================================
	Add
=================================================
*/
	bool VRenderGraph::Add (EQueueType				queue,
							VirtualResources_t		input,
							VirtualResources_t		output,
							GraphicsCommandFn_t&&	pass,
							StringView				dbgName)
	{
		CHECK_ERR( queue == EQueueType::Graphics );

		auto*	cmd = _Add<GraphicsCmd>( queue, input, output, dbgName );
		CHECK_ERR( cmd );
		
		PlacementNew< Function< void (void *) >>(
			&cmd->pass,
			[fn = std::move(pass), cmd] (void* ctx)
			{
				fn( *static_cast<IGraphicsContext*>(ctx), ArrayView{cmd->input, cmd->inputCount}, ArrayView{cmd->output, cmd->outputCount} );
			});
		return true;
	}
	
/*
=================================================
	Add
=================================================
*/
	bool VRenderGraph::Add (EQueueType				queue,
							VirtualResources_t		input,
							VirtualResources_t		output,
							ComputeCommandFn_t&&	pass,
							StringView				dbgName)
	{
		CHECK_ERR( queue == EQueueType::Graphics or queue == EQueueType::AsyncCompute );

		auto*	cmd = _Add<ComputeCmd>( queue, input, output, dbgName );
		CHECK_ERR( cmd );
		
		PlacementNew< Function< void (void *) >>(
			&cmd->pass,
			[fn = std::move(pass), cmd] (void* ctx)
			{
				fn( *static_cast<IComputeContext*>(ctx), ArrayView{cmd->input, cmd->inputCount}, ArrayView{cmd->output, cmd->outputCount} );
			});
		return true;
	}
	
/*
=================================================
	Add
=================================================
*/
	bool VRenderGraph::Add (EQueueType				queue,
							VirtualResources_t		input,
							VirtualResources_t		output,
							TransferCommandFn_t&&	pass,
							StringView				dbgName)
	{
		auto*	cmd = _Add<TransferCmd>( queue, input, output, dbgName );
		CHECK_ERR( cmd );
		
		PlacementNew< Function< void (void *) >>(
			&cmd->pass,
			[fn = std::move(pass), cmd] (void* ctx)
			{
				fn( *static_cast<ITransferContext*>(ctx), ArrayView{cmd->input, cmd->inputCount}, ArrayView{cmd->output, cmd->outputCount} );
			});
		return true;
	}
	
/*
=================================================
	Flush
----
	TODO: optimize
=================================================
*/
	bool VRenderGraph::Flush ()
	{
		// resolve input dependencies
		for (auto& cmd : _commands)
		{
			bool	complete = true;

			for (size_t i = 0; i < cmd->inputCount; ++i)
			{
				auto*	dep = _resWriteCmd[ cmd->input[i].Index() ];
				cmd->inputCmd[i] = dep;
				complete &= (dep != null);
			}

			cmd->state = complete ? BaseCmd::EState::Complete : BaseCmd::EState::Incomplete;
		}

		// sort
		Array<BaseCmd*>	ordered;
		for (; _commands.size();)
		{
			for (auto iter = _commands.begin(); iter != _commands.end();)
			{
				if ( (*iter)->state != BaseCmd::EState::Complete )
				{
					iter = _commands.erase( iter );
					continue;
				}

				bool	failed		= false;
				bool	complete	= true;

				for (size_t i = 0; i < (*iter)->inputCount; ++i)
				{
					auto*	dep = (*iter)->inputCmd[i];
					ASSERT( dep );

					failed	 |= (dep->state == BaseCmd::EState::Incomplete);
					complete &= (dep->state == BaseCmd::EState::Pending);
				}

				if ( complete )
				{
					(*iter)->state = BaseCmd::EState::Pending;
					ordered.push_back( *iter );
					iter = _commands.erase( iter );
					continue;
				}

				if ( failed )
				{
					(*iter)->state = BaseCmd::EState::Incomplete;
					iter = _commands.erase( iter );
					continue;
				}

				++iter;
			}
		}

		// TODO: merge render passes

		// execute
		VGraphicsContext	gctx;
		//VComputeContext	cctx;
		//VTransferContext	tctx;

		for (auto* cmd : ordered)
		{
			ASSERT( cmd->state == BaseCmd::EState::Pending );
			
			cmd->pass( &gctx );

			/*BEGIN_ENUM_CHECKS();
			switch ( cmd->queue )
			{
				case EQueueType::Graphics :			cmd->pass( &gctx );		break;
				case EQueueType::AsyncCompute :		cmd->pass( &cctx );		break;
				case EQueueType::AsyncTransfer :	cmd->pass( &tctx );		break;
				case EQueueType::Unknown :
				case EQueueType::_Count :			ASSERT(false);			break;
			}
			END_ENUM_CHECKS();*/
		}

		_commands.clear();
		_resWriteCmd.fill( null );
		_resUsage.fill( EVirtualResourceUsage::Unknown );
		_allocator.Discard();

		return true;
	}


}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
