// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef AE_ENABLE_VULKAN
# include "graphics/Vulkan/RenderGraph/VCommandPoolManager.h"
# include "graphics/Vulkan/RenderGraph/VBakedCommands.h"

namespace AE::Graphics
{

	//
	// Vulkan Command Batch
	//

	class VCommandBatch final : public IFence
	{
		friend class VRenderGraph;
		friend class VRenderTask;

	// types
	public:

		struct CmdBufPool
		{
		// types
		public:
			union Cmdbuf
			{
				VkCommandBuffer		vk;
				VBakedCommands		baked;

				Cmdbuf () {}
			};
			using Pool_t = StaticArray< Cmdbuf, GraphicsConfig::MaxCmdBufPerBatch >;


		// variables
		private:
			Atomic<uint>	_ready;		// 1 - commands recording have been completed and added to pool
			Atomic<uint>	_cmdTypes;	// 0 - vulkan cmd buffer, 1 - backed commands
			Atomic<uint>	_counter;	// index in '_pool'
			uint			_count;		// number of commands in '_pool'
			Pool_t			_pool;

			DEBUG_ONLY(
				Threading::RWDataRaceCheck	_drCheck;	// for '_pool' and '_count'
			)


		// methods
		public:
			CmdBufPool ();

			// user api
			ND_ uint  Acquire ();
				void  Add (INOUT uint& idx, VkCommandBuffer cmdbuf);
				void  Add (INOUT uint& idx, VBakedCommands &&ctx);
				void  Complete (INOUT uint& idx);

			// owner api
				void  Lock ();
				void  Reset ();
			ND_ bool  IsReady ();
			ND_ bool  IsLocked ();
			
				void  GetCommands (OUT Array<VkCommandBuffer> &cmdbufs);
				bool  CommitIndirectBuffers (VCommandPoolManager &cmdPoolMngr, EQueueType queue, ECommandBufferType cmdbufType);
		};


	private:
		using GpuDependencies_t	= FixedArray< RC<VCommandBatch>, 8 >;

		enum class EStatus : uint
		{
			Initial,		// after _Create()
			Pending,		// after Submit()		// command batch is ready to be submitted to the GPU
			Submitted,		// after _OnSubmit()	// command batch is already submitted to the GPU
			Complete,		// after _OnComlete()	// command batch has been executed in the GPU
		};


	// variables
	private:
		// for render tasks
		CmdBufPool			_cmdPool;

		// gpu to cpu dependency
		VkFence				_fence			= VK_NULL_HANDLE;
		Atomic<EStatus>		_status			{EStatus::Initial};

		// dependencies from another batches
		Threading::SpinLock	_depsQuard;
		GpuDependencies_t	_dependsOn;

		EQueueType			_queueType		= Default;
		ubyte				_frameId		= UMax;
		
		const ubyte			_indexInPool;

		DEBUG_ONLY(
			uint			_frameUID;
			String			_dbgName;
		)
			

	// methods
	public:
		~VCommandBatch ();


	// user api (thread safe)
	public:
		template <typename TaskType, typename ...Ctor, typename ...Deps>
		AsyncTask	Add (const Tuple<Ctor...>&	ctor	= Default,
						 const Tuple<Deps...>&	deps	= Default,
						 StringView				dbgName	= Default);

		bool  AddDependency (RC<VCommandBatch> batch);

		bool  Submit (ESubmitMode mode = ESubmitMode::Deferred);

		ND_ EQueueType	GetQueueType ()		const	{ return _queueType; }
		ND_ uint		GetFrameIndex ()	const	{ return _frameId; }
		ND_ bool		IsSubmitted ();


	// IFence
	public:
		ND_ bool  Wait (nanoseconds timeout) override;
		ND_ bool  IsComplete () override;


	// render graph api
	private:
		explicit VCommandBatch (uint indexInPool);

		bool  _Create (EQueueType queue, uint frameId, uint frameUID, StringView dbgName);
		void  _OnSubmit ();
		void  _OnComplete ();

		void  _ReleaseObject () override;
	};
	


	//
	// Vulkan Render Task interface
	//

	class VRenderTask : public Threading::IAsyncTask
	{
		friend class VCommandBatch;

	// variables
	private:
		RC<VCommandBatch>			_batch;
		VCommandBatch::CmdBufPool&	_pool;
		uint						_submitIdx	= UMax;
		
		DEBUG_ONLY(
			const String			_dbgName;
		)


	// methods
	public:
		VRenderTask (RC<VCommandBatch> batch, StringView dbgName) :
			IAsyncTask{ EThread::Renderer },
			_batch{ RVRef(batch) },
			_pool{ _batch->_cmdPool },
			_submitIdx{ _pool.Acquire() }
			DEBUG_ONLY(, _dbgName{ dbgName })
		{}
		
		ND_ RC<VCommandBatch>	GetBatch ()	const	{ return _batch; }


	// IAsyncTask
	public:
		void  OnCancel () override final;

		DEBUG_ONLY( NtStringView  DbgName () const override final { return _dbgName; })


	protected:
		void  OnFailure ();

		template <typename CmdBufType>
		void  Execute (CmdBufType &cmdbuf);
	};
	


/*
=================================================
	Add
=================================================
*/
	template <typename TaskType, typename ...Ctor, typename ...Deps>
	inline AsyncTask  VCommandBatch::Add (const Tuple<Ctor...>&	ctorArgs,
										  const Tuple<Deps...>&	deps,
										  StringView			dbgName)
	{
		ASSERT( not IsSubmitted() );

		auto	task = ctorArgs.Apply([this, dbgName] (auto&& ...args) { return MakeRC<TaskType>( FwdArg<decltype(args)>(args)..., GetRC(), dbgName ); });

		if ( task->_submitIdx != UMax and Threading::Scheduler().Run( task, deps ))
			return task;
		else
			return null;
	}
//-----------------------------------------------------------------------------


		
/*
=================================================
	Execute
=================================================
*/
	template <typename CmdBufType>
	inline void  VRenderTask::Execute (CmdBufType &cmdbuf)
	{
		if constexpr( CmdBufType::IsIndirectContext )
			_pool.Add( INOUT _submitIdx, cmdbuf.Prepare() );
		else
			_pool.Add( INOUT _submitIdx, cmdbuf.ReleaseCommandBuffer() );
	}

/*
=================================================
	OnCancel
=================================================
*/
	inline void  VRenderTask::OnCancel ()
	{
		_pool.Complete( INOUT _submitIdx );
	}
	
/*
=================================================
	OnFailure
=================================================
*/
	inline void  VRenderTask::OnFailure ()
	{
		_pool.Complete( INOUT _submitIdx );
		IAsyncTask::OnFailure();
	}


}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
