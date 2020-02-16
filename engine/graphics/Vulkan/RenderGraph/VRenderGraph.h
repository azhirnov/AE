// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef AE_ENABLE_VULKAN

# include "graphics/Public/RenderGraph.h"
# include "graphics/Vulkan/VCommon.h"

# include "stl/Memory/LinearAllocator.h"
# include "threading/Containers/LfIndexedPool.h"

namespace AE::Graphics
{
	class VCommandBatch;
	class VCmdBatchDepsManager;


	//
	// Vulkan Render Graph
	//

	class VRenderGraph final : public IRenderGraph
	{
	// types
	private:
		class GraphicsContext;
		class IndirectGraphicsContext;
		class RenderContext;

		struct BaseCmd;
		struct RenderCmd;
		struct GraphicsCmd;
		struct ComputeCmd;
		struct TransferCmd;

		struct CtxPerQueue
		{
			UniquePtr<GraphicsContext>			direct;		// direct write to vulkan command buffer
			UniquePtr<IndirectGraphicsContext>	indirect;	// writes to software command buffer and then to vulkan cmdbuf
		};

		using VirtualResWrite_t		= HashMap< GfxResourceID, BaseCmd* >;
		using VirtualResUsage_t		= HashMap< GfxResourceID, EVirtualResourceUsage >;
		using CtxPerQueue_t			= StaticArray< CtxPerQueue, uint(EQueueType::_Count) >;
		
		using SubmittedBatches_t	= Array< CmdBatchID >;	// TODO: ring buffer
		using CmdBatchPool_t		= Threading::LfIndexedPool< VCommandBatch, uint, 32, 16 >;

		using TaskDepsMngr_t		= SharedPtr< VCmdBatchDepsManager >;


	// variables
	private:
		LinearAllocator<>		_allocator;

		Mutex					_cmdGuard;
		Array<BaseCmd *>		_commands;			// TODO: use LfIndexedPool + UnassignAll()
		VirtualResWrite_t		_resWriteCmd;		// TODO: lock-free ?
		VirtualResUsage_t		_resUsage;

		CtxPerQueue_t			_contexts;

		CmdBatchPool_t			_cmdBatchPool;
		SubmittedBatches_t		_submitted;

		VResourceManager &		_resMngr;

		TaskDepsMngr_t			_taskDepsMngr;
		
		RWDataRaceCheck			_drCheck;


	// methods
	public:
		explicit VRenderGraph (VResourceManager &);
		~VRenderGraph ();

		bool  Initialize ();
		void  Deinitialize ();
		
		EQueueMask  GetPresentQueues () override;

		bool Add (EQueueType				queue,
				  VirtualResources_t		input,
				  VirtualResources_t		output,
				  RenderPassSetupFn_t&&		setup,
				  RenderPassDrawFn_t&&		pass,
				  StringView				dbgName) override;

		bool Add (EQueueType				queue,
				  VirtualResources_t		input,
				  VirtualResources_t		output,
				  GraphicsCommandFn_t&&		pass,
				  StringView				dbgName) override;

		bool Add (EQueueType				queue,
				  VirtualResources_t		input,
				  VirtualResources_t		output,
				  ComputeCommandFn_t&&		pass,
				  StringView				dbgName) override;

		bool Add (EQueueType				queue,
				  VirtualResources_t		input,
				  VirtualResources_t		output,
				  TransferCommandFn_t&&		pass,
				  StringView				dbgName) override;
		
		CmdBatchID Submit () override;
		
		bool  Wait (ArrayView<CmdBatchID> ids) override;
		bool  WaitIdle () override;
		
		bool  IsComplete (ArrayView<CmdBatchID> batches) override;


	private:
		template <typename T>
		ND_ T*  _Add (EQueueType			queue,
					  VirtualResources_t	input,
					  VirtualResources_t	output,
					  StringView			dbgName);

		ND_ VLogicalRenderPass*  _CreateLogicalPass (const RenderPassDesc &desc);

		bool  _CreateRenderPass (ArrayView<VLogicalRenderPass*> passes);

		void  _ResolveDependencies ();
		void  _SortCommands (OUT Array<BaseCmd*> &ordered);
		void  _MergeRenderPasses ();
		bool  _AcquireNextBatch (OUT CmdBatchID &, OUT VCommandBatch* &batch);
		bool  _ExecuteCommands (ArrayView<BaseCmd*> ordered, VCommandBatch *batch, CmdBatchID batchId);
		void  _RecycleCmdBatches ();
	};


}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
