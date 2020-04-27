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
		class DirectGraphicsContext;
		class RenderContext;

		struct BaseCmd;
		class  RenderTask;

		struct CtxPerQueue
		{
			UniquePtr<GraphicsContext>			indirect;	// direct write to vulkan command buffer
			UniquePtr<DirectGraphicsContext>	direct;		// writes to software command buffer and then to vulkan cmdbuf
		};

		struct VirtualResInfo
		{
			EVirtualResourceUsage		usage	= Zero;
			UniqueID<GfxResourceID>		real;
		};

		using VirtualResWrite_t		= HashMap< GfxResourceID, BaseCmd* /*,
												std::hash<GfxResourceID>, std::equal_to<GfxResourceID>, StdLinearAllocator<Pair<const GfxResourceID, BaseCmd*>>*/ >;

		using VirtualToReal_t		= HashMap< GfxResourceID, VirtualResInfo /*,
												std::hash<GfxResourceID>, std::equal_to<GfxResourceID>, StdLinearAllocator<Pair<const GfxResourceID, VirtualResInfo>>*/ >;

		using CtxPerQueue_t			= StaticArray< CtxPerQueue, uint(EQueueType::_Count) >;
		using SubmittedBatches_t	= Array< CmdBatchID >;	// TODO: ring buffer
		using CmdBatchPool_t		= Threading::LfIndexedPool< VCommandBatch, uint, 32, 16 >;

		using TaskDepsMngr_t		= SharedPtr< VCmdBatchDepsManager >;
		using CmdPoolMngr_t			= UniquePtr< VCommandPoolManager >;


	// variables
	private:
		LinearAllocator<>		_allocator;

		Mutex					_cmdGuard;
		Array< BaseCmd *>		_commands;			// TODO: use LfIndexedPool + UnassignAll()
		Array<AsyncTask>		_asyncCommands;
		VirtualResWrite_t		_resWriteCmd;		// TODO: lock-free ?
		VirtualToReal_t			_virtualToReal;

		CtxPerQueue_t			_contexts;

		CmdBatchPool_t			_cmdBatchPool;
		SubmittedBatches_t		_submitted;

		VResourceManager &		_resMngr;

		TaskDepsMngr_t			_taskDepsMngr;
		CmdPoolMngr_t			_cmdPoolMngr;
		
		RWDataRaceCheck			_drCheck;


	// methods
	public:
		explicit VRenderGraph (VResourceManager &);
		~VRenderGraph ();

		bool  Initialize ();
		void  Deinitialize ();
		
		EQueueMask  GetPresentQueues () override;

		bool _AddSync (EQueueType				queue,
					   VirtualResources_t		input,
					   VirtualResources_t		output,
					   RenderPassDesc const&	rpDesc,
					   RenderPassDrawFn_t &&	draw,
					   StringView				dbgName) override;
		
		bool _AddAsync (EQueueType				queue,
						VirtualResources_t		input,
						VirtualResources_t		output,
						RenderPassDesc const&	rpDesc,
						AsyncDrawFn_t &&		asyncDraw,
						StringView				dbgName) override;

		bool Add (EQueueType				queue,
				  VirtualResources_t		input,
				  VirtualResources_t		output,
				  GraphicsCommandFn_t &&	pass,
				  StringView				dbgName) override;

		bool Add (EQueueType				queue,
				  VirtualResources_t		input,
				  VirtualResources_t		output,
				  ComputeCommandFn_t &&		pass,
				  StringView				dbgName) override;

		bool Add (EQueueType				queue,
				  VirtualResources_t		input,
				  VirtualResources_t		output,
				  TransferCommandFn_t &&	pass,
				  StringView				dbgName) override;
		
		CmdBatchID  Submit () override;
		
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
		bool  _CreateFramebuffer (ArrayView<VLogicalRenderPass*> passes);

		void  _ResolveDependencies ();
		void  _SortCommands (OUT Array<BaseCmd*> &ordered);
		bool  _MergeRenderPasses (ArrayView<BaseCmd*> ordered);
		bool  _AcquireNextBatch (OUT CmdBatchID &, OUT VCommandBatch* &batch);
		bool  _ExecuteCommands (ArrayView<BaseCmd*> ordered, VCommandBatch *batch, CmdBatchID batchId);
		void  _RecycleCmdBatches ();
	};


}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
