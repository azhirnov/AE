// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef AE_ENABLE_VULKAN

# include "graphics/Vulkan/VRenderGraph.h"

namespace AE::Graphics
{
	
/*
=================================================
	Recycle
=================================================
*/
	void  VRenderGraph::CommandBatchApi::Recycle (uint indexInPool)
	{
		auto&	rg = RenderGraph();
		rg._batchPool.Unassign( indexInPool );
	}

/*
=================================================
	Submit
=================================================
*/
	bool  VRenderGraph::CommandBatchApi::Submit (VCommandBatch &batch, ESubmitMode mode)
	{
		auto&	rg = RenderGraph();

		// add to pending batches
		{
			EXLOCK( rg._queueGuard );
			rg._queueMap[ uint(batch.GetQueueType()) ].pending.push_back( batch.GetRC() );
		}

		// try to submit
		if ( mode == ESubmitMode::Immediately )
		{
			rg._FlushQueue( batch.GetQueueType(), batch.GetFrameIndex(), 1 );
		}

		return true;
	}
//-----------------------------------------------------------------------------
	

	
/*
=================================================
	Resolve
=================================================
*/
	bool  VRenderGraph::FenceDepsManager::Resolve (AnyTypeCRef dep, const AsyncTask &task, INOUT uint &bitIndex)
	{
		if ( auto* batch = dep.GetIf< RC<VCommandBatch> >() )
		{
			if ( *batch == null )
				return false;

			EXLOCK( _depsListGuard );

			_depsList.emplace_back( *batch, task, bitIndex );
			++bitIndex;

			return true;
		}
		else
		if ( auto* fence = dep.GetIf< FencePtr >() )
		{
			if ( *fence == null )
				return false;

			EXLOCK( _depsListGuard );

			_depsList.emplace_back( *fence, task, bitIndex );
			++bitIndex;

			return true;
		}
		return false;
	}
	
/*
=================================================
	Update
----
	TODO: remove Update, add batch dependencies to batch
=================================================
*/
	void  VRenderGraph::FenceDepsManager::Update ()
	{
		EXLOCK( _depsListGuard );
		
		for (auto iter = _depsList.begin(); iter != _depsList.end();)
		{
			if ( iter->fence->IsComplete() )
			{
				_SetDependencyCompletionStatus( iter->task, iter->bitIndex, false );
				iter = _depsList.erase( iter );
			}
			else
				++iter;
		}
	}
//-----------------------------------------------------------------------------
	

	
/*
=================================================
	BeginFrameTask::Run
=================================================
*/
	void  VRenderGraph::BeginFrameTask::Run ()
	{
		auto&	rg = RenderGraph();
		
		// check frame UID
		{
			EXLOCK( rg._frameGuard );

			if_unlikely( rg._perFrameUID[ _frameId ] != _frameUID )
			{
				AE_LOGE( "Invalid frame UID, task will be canceled" );
				return OnFailure();
			}
		}

		if ( not rg._WaitForFrame( _frameId, nanoseconds{_WaitFenceTime} ))
		{
			Continue();
			return;
		}

		if_unlikely( not rg._SetState( EState::BeginFrame, EState::RecordFrame ))
		{
			AE_LOGE( "incorrect render graph state, must be 'EState::BeginFrame'" );
			return OnFailure();
		}

		CHECK( rg._cmdPoolMngr->NextFrame( _frameId ));

		rg._resMngr->GetStagingManager().OnNextFrame( _frameId );
	}
//-----------------------------------------------------------------------------

	

/*
=================================================
	EndFrameTask::Run
=================================================
*/
	void  VRenderGraph::EndFrameTask::Run ()
	{
		auto&	rg = RenderGraph();
		
		// check frame UID
		{
			EXLOCK( rg._frameGuard );
			
			if_unlikely( rg._perFrameUID[ _frameId ] != _frameUID )
			{
				AE_LOGE( "Invalid frame UID, task will be canceled" );
				return OnFailure();
			}
		}

		for (uint q = 0; q < uint(EQueueType::_Count); ++q)
		{
			rg._FlushQueue( EQueueType(q), _frameId, 1 );
		}

		if_unlikely( not rg._SetState( EState::RecordFrame, EState::Idle ))
		{
			AE_LOGE( "incorrect render graph state, must be 'EState::RecordFrame'" );
			return OnFailure();
		}
	}
//-----------------------------------------------------------------------------



/*
=================================================
	constructor
=================================================
*/
	VRenderGraph::VRenderGraph (const VDevice &dev) :
		_state{ EState::Initial },
		_frameId{ 0 },
		_maxFrames{ GraphicsConfig::MaxFrames },
		_frameUID{ 0 },
		_device{ dev },
		_depMngr{ MakeRC<FenceDepsManager>() }
	{
	}
	
/*
=================================================
	destructor
=================================================
*/
	VRenderGraph::~VRenderGraph ()
	{
		Deinitialize();
	}
	
/*
=================================================
	CreateInstance
=================================================
*/
	void  VRenderGraph::CreateInstance (const VDevice &dev)
	{
		new( _Instance() ) VRenderGraph{ dev };
		
		ThreadFence( EMemoryOrder::Release );
	}
	
/*
=================================================
	DestroyInstance
=================================================
*/
	void  VRenderGraph::DestroyInstance ()
	{
		ThreadFence( EMemoryOrder::Acquire );

		_Instance()->~VRenderGraph();

		ThreadFence( EMemoryOrder::Release );
	}
	
/*
=================================================
	_Instance
=================================================
*/
	VRenderGraph*  VRenderGraph::_Instance ()
	{
		static std::aligned_storage_t< sizeof(VRenderGraph), alignof(VRenderGraph) >	render_graph;
		return Cast<VRenderGraph>( &render_graph );
	}
	
/*
=================================================
	Initialize
=================================================
*/
	bool  VRenderGraph::Initialize (const GraphicsCreateInfo &info)
	{
		CHECK_ERR( _SetState( EState::Initial, EState::Initialization ));

		CHECK_ERR( info.maxFrames > 1 );
		CHECK_ERR( info.maxFrames <= GraphicsConfig::MaxFrames );

		_frameUID	= 0;
		_frameId	= 0;					// TODO: protect frame id
		_maxFrames	= info.maxFrames;

		_resMngr.reset( new VResourceManagerImpl{ _device });
		CHECK_ERR( _resMngr->Initialize( info ));

		_cmdPoolMngr.reset( new VCommandPoolManager{ _device });

		Threading::Scheduler().RegisterDependency< RC<VCommandBatch> >( _depMngr );
		Threading::Scheduler().RegisterDependency< FencePtr >( _depMngr );
		
		CHECK_ERR( _SetState( EState::Initialization, EState::Idle ));
		return true;
	}
	
/*
=================================================
	Deinitialize
=================================================
*/
	void  VRenderGraph::Deinitialize ()
	{
		CHECK_ERRV( _SetState( EState::Idle, EState::Destroyed ));

		_WaitAll();

		_batchPool.Release();

		_cmdPoolMngr = null;

		if ( _resMngr )
		{
			_resMngr->Deinitialize();
			_resMngr = null;
		}

		Threading::Scheduler().UnregisterDependency< RC<VCommandBatch> >();
		Threading::Scheduler().UnregisterDependency< FencePtr >();
	}
	
/*
=================================================
	_FlushQueue
=================================================
*/
	bool  VRenderGraph::_FlushQueue (EQueueType queueType, uint frameId, uint maxIter)
	{
		auto&			dev			= GetDevice();
		auto			queue		= dev.GetQueue( queueType );
		BatchQueue_t	pending;

		// find batches that can be submitted
		/*for (size_t b = 0, b_max = Min( maxIter, curr_queue.pending.size()), changed = 1;
			 changed and (b < b_max);
			 ++b)
		{
			changed = 0;

			for (auto iter = curr_queue.pending.begin(); iter != curr_queue.pending.end();)
			{
				auto&		batch	 = *iter;
				bool		is_ready = true;
				EQueueMask	q_mask2	 = Default;
				
				for (auto& dep : batch->GetDependencies())
				{
					const auto	min_state = (dep->GetQueueType() == queueIndex ? EBatchState::Ready : EBatchState::Submitted);

					q_mask2		|= dep->GetQueueType();
					is_ready	&= (dep->GetState() >= min_state);
				}

				if ( is_ready )
				{
					if ( pending.size() == pending.capacity() )
						break;

					wait_idle |= batch->IsQueueSyncRequired();
					batch->OnReadyToSubmit();
					pending.push_back( std::move(batch) );
							
					changed	= 1;
					iter	= curr_queue.pending.erase( iter );
					q_mask	|= q_mask2;
				}
				else
					++iter;
			}
		}
		
		if ( pending.empty() )
		{
			return false;
		}*/

		// extract batches
		{
			EXLOCK( _queueGuard );

			auto&	curr_queue	= _queueMap[ uint(queueType) ];
			std::swap( pending, curr_queue.pending );	// TODO: optimize
		}

		if ( pending.empty() )
			return false;

		for (auto& batch : pending)
		{
			Array<VkCommandBuffer>	cmdbufs;
			batch->_cmdPool.GetCommands( OUT cmdbufs );

			EXLOCK( queue->guard );
		
			VkSubmitInfo	submit = {};
			submit.sType				= VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submit.pCommandBuffers		= cmdbufs.data();
			submit.commandBufferCount	= uint(cmdbufs.size());

			VK_CHECK( dev.vkQueueSubmit( queue->handle, 1, &submit, batch->_fence ));

			batch->_OnSubmit();
		}

		// add to submitted batches
		{
			auto&	frame = _perFrame[ frameId ];
			EXLOCK( frame.quard );
			frame.submitted.insert( frame.submitted.end(), pending.begin(), pending.end() );
		}

		return true;
	}

/*
=================================================
	_WaitForFrame
=================================================
*/
	bool  VRenderGraph::_WaitForFrame (uint frameId, nanoseconds timeout)
	{
		using TempFences_t = FixedArray< VkFence, 16 >;

		// wait for submitted batches
		TempFences_t	tmp_fences;
		auto&			dev			= GetDevice();
		auto&			frame		= _perFrame[ frameId ];

		EXLOCK( frame.quard );

		for (; frame.submitted.size(); )
		{
			for (usize i = 0, cnt = Min( frame.submitted.size(), tmp_fences.capacity() ); i < cnt; ++i)
			{
				tmp_fences.push_back( frame.submitted[i]->_fence );
			}
			
			VkResult	err = dev.vkWaitForFences( dev.GetVkDevice(), uint(tmp_fences.size()), tmp_fences.data(), VK_TRUE, timeout.count() );

			if ( err == VK_TIMEOUT )
				return false;

			VK_CHECK( err );

			for (usize i = 0; i < tmp_fences.size(); ++i)
			{
				frame.submitted[i]->_OnComplete();
			}
				
			frame.submitted.erase( frame.submitted.begin(), frame.submitted.begin() + tmp_fences.size() );

			VK_CHECK( dev.vkResetFences( dev.GetVkDevice(), uint(tmp_fences.size()), tmp_fences.data() ));
			tmp_fences.clear();
		}
		
		_depMngr->Update();
		return true;
	}
	
/*
=================================================
	CreateBatch
=================================================
*/
	RC<VCommandBatch>  VRenderGraph::CreateBatch (EQueueType queue, StringView dbgName)
	{
		CHECK_ERR( AnyEqual( _GetState(), EState::Idle, EState::BeginFrame, EState::RecordFrame ));

		uint	index;
		CHECK_ERR( _batchPool.Assign( OUT index, [](auto* ptr, uint idx) { new (ptr) VCommandBatch{ idx }; }));

		auto&	batch = _batchPool[ index ];

		if ( batch._Create( queue, _frameId, _frameUID, dbgName ))		// TODO: protect frame id
			return RC<VCommandBatch>{ &batch };

		_batchPool.Unassign( index );
		RETURN_ERR( "failed to allocate command batch" );
	}
	
/*
=================================================
	WaitAll
=================================================
*/
	bool  VRenderGraph::WaitAll ()
	{
		CHECK_ERR( AnyEqual( _GetState(), EState::Idle, EState::BeginFrame, EState::RecordFrame ));

		return _WaitAll();
	}
	
/*
=================================================
	_WaitAll
=================================================
*/
	bool  VRenderGraph::_WaitAll ()
	{
		// TODO: wait for begin/end tasks ?

		// wait for frames
		{
			uint	frame_id;
			{
				EXLOCK( _frameGuard );
				frame_id = _frameId;
			}

			for (uint i = 0; i < _maxFrames; ++i)
			{
				frame_id = ++frame_id % _maxFrames;
				CHECK_ERR( _WaitForFrame( frame_id, nanoseconds{~0ull} ));
			}

			// TODO: what if new batches was added ?
		}

		return true;
	}
	
/*
=================================================
	_SetState
=================================================
*/
	bool  VRenderGraph::_SetState (EState expected, EState newState)
	{
		return _state.compare_exchange_strong( INOUT expected, newState, EMemoryOrder::Relaxed );
	}
	
/*
=================================================
	_GetState
=================================================
*/
	VRenderGraph::EState  VRenderGraph::_GetState ()
	{
		return _state.load( EMemoryOrder::Relaxed );
	}


}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
