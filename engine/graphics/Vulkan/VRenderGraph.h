// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef AE_ENABLE_VULKAN

# include "threading/Containers/LfIndexedPool.h"

# include "graphics/Vulkan/RenderGraph/VCommandPoolManager.h"
# include "graphics/Vulkan/RenderGraph/VCommandBatch.h"
# include "graphics/Vulkan/VResourceManager.h"

namespace AE::Graphics
{

	//
	// Vulkan Render Graph
	//

	class VRenderGraph final
	{
	// types
	public:
		class CommandBatchApi
		{
			friend class VCommandBatch;
			static void  Recycle (uint indexInPool);
			static bool  Submit (VCommandBatch &, ESubmitMode);
		};


	private:
		using PerQueueSem_t		= StaticArray< VkSemaphore, uint(EQueueType::_Count) >;
		using BatchQueue_t		= Array< RC<VCommandBatch> >;	// TODO: circular queue ?

		struct QueueData
		{
			BatchQueue_t	pending;
			PerQueueSem_t	semaphores	{};
		};
		using PendingQueueMap_t	= StaticArray< QueueData, uint(EQueueType::_Count) >;


		struct FrameData
		{
			Mutex			quard;
			BatchQueue_t	submitted;
		};
		using PerFrame_t	= StaticArray< FrameData, GraphicsConfig::MaxFrames >;
		using FrameUIDs_t	= StaticArray< uint, GraphicsConfig::MaxFrames >;


		using BatchPool_t	= Threading::LfIndexedPool< VCommandBatch, uint, 64, 1 >;

		static constexpr uint	_FrameBits = 2;
		STATIC_ASSERT( (1u << _FrameBits) == GraphicsConfig::MaxFrames );

		static constexpr uint	_WaitFenceTime	= 1'000;	// in nanoseconds

		class FenceDepsManager;
		class BeginFrameTask;
		class EndFrameTask;

		enum class EState : uint
		{
			Initial,
			Initialization,
			Idle,
			BeginFrame,
			RecordFrame,
			Destroyed,
		};


	// variables
	private:
		Atomic<EState>					_state;

		uint							_frameId		: _FrameBits;
		uint							_maxFrames		: _FrameBits;
		uint							_frameUID		= 0;
		FrameUIDs_t						_perFrameUID	= {};
		Mutex							_frameGuard;			// TODO: lock-free

		BatchPool_t						_batchPool;

		Mutex							_queueGuard;		// TODO: remove global lock
		PendingQueueMap_t				_queueMap;

		PerFrame_t						_perFrame;

		VDevice const&					_device;
		UniquePtr<VCommandPoolManager>	_cmdPoolMngr;
		UniquePtr<VResourceManagerImpl>	_resMngr;
		RC<FenceDepsManager>			_depMngr;


	// methods
	public:
		static void  CreateInstance (const VDevice &dev);
		static void  DestroyInstance ();

		friend VRenderGraph&  RenderGraph ();

		bool  Initialize (const GraphicsCreateInfo &);
		void  Deinitialize ();

		template <typename ...Deps>
		AsyncTask	BeginFrame (const Tuple<Deps...> &deps = Default);
		
		template <typename ...Deps>
		AsyncTask	EndFrame (const Tuple<Deps...> &deps = Default);

		bool  WaitAll ();

		ND_ RC<VCommandBatch>  CreateBatch (EQueueType queue, StringView dbgName = Default);

		ND_ VResourceManagerImpl &	GetResourceManager ()			{ ASSERT( _resMngr );  return *_resMngr; }
		ND_ VDevice const&			GetDevice ()			const	{ return _device; }
		ND_ VCommandPoolManager &	GetCommandPoolManager ()		{ ASSERT( _cmdPoolMngr );  return *_cmdPoolMngr; }


	private:
		explicit VRenderGraph (const VDevice &dev);
		~VRenderGraph ();

		ND_ static VRenderGraph*  _Instance ();

		bool  _WaitForFrame (uint frameId, nanoseconds timeout);
		bool  _FlushQueue (EQueueType q, uint frameId, uint maxIter);
		bool  _WaitAll ();
		
		ND_ bool    _SetState (EState expected, EState newState);
		ND_ EState  _GetState ();
	};

	
	
	//
	// Fence (VCommandBatch) Dependency Manager
	//

	class VRenderGraph::FenceDepsManager final : public Threading::ITaskDependencyManager
	{
	// types
	private:
		struct Dependency
		{
			FencePtr		fence;
			AsyncTask		task;
			uint			bitIndex;

			Dependency (const FencePtr &fence, const AsyncTask &task, uint index) : fence{fence}, task{task}, bitIndex{index} {}
		};
		

	// variables
	private:
		Mutex				_depsListGuard;		// TODO: lock-free ?
		Array<Dependency>	_depsList;			// TODO: ring-buffer
		

	// methods
	public:
		~FenceDepsManager () override {}

		void  Update ();
		bool  Resolve (AnyTypeCRef dep, const AsyncTask &task, INOUT uint &bitIndex) override;
	};



	//
	// Begin Frame Task
	//

	class VRenderGraph::BeginFrameTask final : public Threading::IAsyncTask
	{
	private:
		const uint		_frameId;
		const uint		_frameUID;

	public:
		BeginFrameTask (uint frameId, uint frameUID) :
			IAsyncTask{EThread::Renderer}, _frameId{frameId}, _frameUID{frameUID}
		{}
			
		void  Run () override;
	};



	//
	// End Frame Task
	//

	class VRenderGraph::EndFrameTask final : public Threading::IAsyncTask
	{
	private:
		const uint		_frameId;
		const uint		_frameUID;

	public:
		EndFrameTask (uint frameId, uint frameUID) :
			IAsyncTask{EThread::Renderer}, _frameId{frameId}, _frameUID{frameUID}
		{}
			
		void  Run () override;
	};
//-----------------------------------------------------------------------------



/*
=================================================
	RenderGraph
=================================================
*/
	ND_ forceinline VRenderGraph&  RenderGraph ()
	{
		return *VRenderGraph::_Instance();
	}
	
/*
=================================================
	BeginFrame
=================================================
*/
	template <typename ...Deps>
	inline AsyncTask  VRenderGraph::BeginFrame (const Tuple<Deps...> &deps)
	{
		CHECK_ERR( _SetState( EState::Idle, EState::BeginFrame ));
		EXLOCK( _frameGuard );

		_frameId = ++_frameId % _maxFrames;
		++_frameUID;
		_perFrameUID[_frameId] = _frameUID;

		AsyncTask	task = MakeRC<VRenderGraph::BeginFrameTask>( uint(_frameId), _frameUID );
		if ( Threading::Scheduler().Run( task, deps ))		// TODO: add dependency from previous frame or check that prev frame is submitted ?
		{
			return task;
		}
		else
		{
			CHECK_ERR( _SetState( EState::BeginFrame, EState::Idle ));
			return null;
		}
	}
	
/*
=================================================
	EndFrame
=================================================
*/	
	template <typename ...Deps>
	inline AsyncTask  VRenderGraph::EndFrame (const Tuple<Deps...> &deps)
	{
		CHECK_ERR( AnyEqual( _GetState(), EState::BeginFrame, EState::RecordFrame ));
		EXLOCK( _frameGuard );

		AsyncTask	task = MakeRC<VRenderGraph::EndFrameTask>( uint(_frameId), _frameUID );
		if ( Threading::Scheduler().Run( task, deps ))
			return task;
		else
			return null;
	}


}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
