// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#ifdef AE_ENABLE_VULKAN

# include "graphics/Vulkan/VRenderGraph.h"
# include "graphics/Vulkan/VDevice.h"
# include "graphics/Vulkan/VEnumCast.h"
# include "graphics/Vulkan/VResourceManager.h"
# include "graphics/Vulkan/Resources/VLogicalRenderPass.h"
# include "graphics/Vulkan/Resources/VFramebuffer.h"
# include "graphics/Vulkan/Resources/VRenderPass.h"

# include "graphics/Private/EnumUtils.h"
# include "graphics/Private/ResourceDataRange.h"

# include "stl/Memory/LinearAllocator.h"
# include "stl/Containers/StructView.h"

# include "threading/Containers/IndexedPool.h"

# include "graphics/Vulkan/VRenderGraph.BarrierManager.h"
# include "graphics/Vulkan/VRenderGraph.LocalBuffer.h"
# include "graphics/Vulkan/VRenderGraph.LocalImage.h"
# include "graphics/Vulkan/VRenderGraph.GraphicsContext.h"
# include "graphics/Vulkan/VRenderGraph.RenderContext.h"

namespace AE::Graphics
{
	
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

		Function<bool (GraphicsContext &)>	pass;
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
	constructor
=================================================
*/
	VRenderGraph::VRenderGraph (VResourceManager &resMngr) :
		_resMngr{ resMngr }
	{}
	
/*
=================================================
	destructor
=================================================
*/
	VRenderGraph::~VRenderGraph ()
	{
	}

/*
=================================================
	Initialize
=================================================
*/
	bool VRenderGraph::Initialize ()
	{
		EXLOCK( _drCheck );

		for (size_t i = 0; i < _contexts.size(); ++i)
		{
			auto	q = _resMngr.GetDevice().GetQueue( EQueueType(i) );
			if ( not q )
				continue;

			_contexts[i] = MakeUnique<GraphicsContext>( _resMngr, q );
			CHECK_ERR( _contexts[i]->Create() );
		}

		return true;
	}
	
/*
=================================================
	Deinitialize
=================================================
*/
	void VRenderGraph::Deinitialize ()
	{
		EXLOCK( _drCheck );

		for (auto& ctx : _contexts) {
			ctx.reset();
		}
	}
	
/*
=================================================
	GetPresentQueues
=================================================
*/
	EQueueMask  VRenderGraph::GetPresentQueues ()
	{
		SHAREDLOCK( _drCheck );
		return Default;	// TODO
	}

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
		// TODO: lock-free
		EXLOCK( _cmdGuard );

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
		SHAREDLOCK( _drCheck );

		auto*	cmd = _Add<RenderCmd>( queue, input, output, dbgName );
		CHECK_ERR( cmd );

		// TODO
		//PlacementNew<RenderPassSetupFn_t>( &cmd->setup, std::move(setup) );
		//PlacementNew<RenderPassFn_t>( &cmd->draw, std::move(draw) );
		
		PlacementNew< Function< void (GraphicsContext &) >>(
			&cmd->pass,
			[this, setup = std::move(setup), draw = std::move(draw), cmd] (GraphicsContext &ctx) -> bool
			{
				RenderPassDesc	rp_desc;
				setup( ctx, ArrayView{cmd->input, cmd->inputCount}, ArrayView{cmd->output, cmd->outputCount}, OUT rp_desc );

				auto*	logical_rp = _CreateLogicalPass( rp_desc );
				CHECK_ERR( logical_rp );
				CHECK_ERR( _CreateRenderPass( {logical_rp} ));

				RenderContext	rctx{ ctx, *logical_rp, _resMngr };
				draw( rctx, ArrayView{cmd->input, cmd->inputCount}, ArrayView{cmd->output, cmd->outputCount} );
				return true;
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
		SHAREDLOCK( _drCheck );

		auto*	cmd = _Add<GraphicsCmd>( queue, input, output, dbgName );
		CHECK_ERR( cmd );
		
		PlacementNew< Function< void (GraphicsContext &) >>(
			&cmd->pass,
			[fn = std::move(pass), cmd] (GraphicsContext &ctx)
			{
				fn( ctx, ArrayView{cmd->input, cmd->inputCount}, ArrayView{cmd->output, cmd->outputCount} );
				return true;
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
		SHAREDLOCK( _drCheck );

		auto*	cmd = _Add<ComputeCmd>( queue, input, output, dbgName );
		CHECK_ERR( cmd );
		
		PlacementNew< Function< void (GraphicsContext &) >>(
			&cmd->pass,
			[fn = std::move(pass), cmd] (GraphicsContext &ctx)
			{
				fn( ctx, ArrayView{cmd->input, cmd->inputCount}, ArrayView{cmd->output, cmd->outputCount} );
				return true;
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
		SHAREDLOCK( _drCheck );

		auto*	cmd = _Add<TransferCmd>( queue, input, output, dbgName );
		CHECK_ERR( cmd );
		
		PlacementNew< Function< void (GraphicsContext &) >>(
			&cmd->pass,
			[fn = std::move(pass), cmd] (GraphicsContext &ctx)
			{
				fn( ctx, ArrayView{cmd->input, cmd->inputCount}, ArrayView{cmd->output, cmd->outputCount} );
				return true;
			});
		return true;
	}
	
/*
=================================================
	Flush
----
	TODO: optimize sorting?, multithreading execution
=================================================
*/
	bool VRenderGraph::Flush ()
	{
		SHAREDLOCK( _drCheck );
		EXLOCK( _cmdGuard );

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
		for (auto* cmd : ordered)
		{
			ASSERT( cmd->state == BaseCmd::EState::Pending );
			
			CHECK( cmd->pass( *_contexts[uint(EQueueType::Graphics)] ));

			/*
			ASSERT( cmd->queue < EQueueType::_Count );

			if ( prev_queue != cmd->queue )
			{
				for (auto& ctx : _contexts) {
					CHECK( ctx->Submit() );
				}
				_contexts[ uint(cmd->queue) ]->Begin();
			}

			CHECK( cmd->pass( *_contexts[uint(cmd->queue)] ));
			*/
		}

		for (auto& ctx : _contexts) {
			CHECK( ctx->Submit() );
		}

		_commands.clear();
		_resWriteCmd.fill( null );
		_resUsage.fill( EVirtualResourceUsage::Unknown );
		_allocator.Discard();

		return true;
	}
	
/*
=================================================
	_CreateLogicalPass
=================================================
*/
	VLogicalRenderPass*  VRenderGraph::_CreateLogicalPass (const RenderPassDesc &desc)
	{
		VLogicalRenderPass*	ptr = null;
		
		// allocate
		{
			EXLOCK( _cmdGuard );
			ptr = _allocator.Alloc< VLogicalRenderPass >();
			CHECK_ERR( ptr );
		}
		
		PlacementNew< VLogicalRenderPass >( ptr );

		CHECK_ERR( ptr->Create( _resMngr, desc ));
		return ptr;
	}
	
/*
=================================================
	_CreateRenderPass
=================================================
*/
	bool  VRenderGraph::_CreateRenderPass (ArrayView<VLogicalRenderPass*> passes)
	{
		RenderPassID	pass_id = _resMngr.CreateRenderPass( passes );
		CHECK_ERR( pass_id );

		StaticArray< Pair<VImageID, ImageViewDesc>, GraphicsConfig::MaxAttachments >	attachments;

		uint2	dimension;
		uint	layers			= 0;
		bool	initialized		= false;
		uint	viewport_count	= 0;

		for (auto* pass : passes)
		{
			if ( pass == passes.front() )
				viewport_count = uint(pass->GetViewports().size());
			else
				CHECK_ERR( viewport_count == pass->GetViewports().size() );


			for (auto&[name, rt] : pass->GetColorTargets())
			{
				auto&	dst = attachments[rt.index];
				
				if ( dst.first == Default )
				{
					// initialize attachment
					dst.first	= rt.imageId;
					dst.second	= rt.viewDesc;

					// compare dimension and layer count
					{
						auto&	desc = _resMngr.GetDescription( rt.imageId );

						if ( initialized )
						{
							CHECK_ERR( All( dimension == uint2{ desc.dimension.x, desc.dimension.y } ));
							CHECK_ERR( layers == rt.viewDesc.layerCount );
						}
						else
						{
							dimension	= uint2{ desc.dimension.x, desc.dimension.y };
							layers		= rt.viewDesc.layerCount;
						}
					}
				}
				else
				{
					CHECK_ERR( dst.first  == rt.imageId );
					CHECK_ERR( dst.second == rt.viewDesc );
				}
			}
		}

		VFramebufferID	framebuffer_id = _resMngr.CreateFramebuffer( attachments, pass_id, dimension, layers );
		CHECK_ERR( framebuffer_id );

		for (size_t i = 0; i < passes.size(); ++i)
		{
			CHECK_ERR( passes[i]->SetRenderPass( _resMngr, pass_id, uint(i), framebuffer_id ));
		}
		return true;
	}


}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
