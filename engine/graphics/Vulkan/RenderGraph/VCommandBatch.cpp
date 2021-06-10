// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#ifdef AE_ENABLE_VULKAN
# include "graphics/Vulkan/RenderGraph/VCommandBatch.h"
# include "graphics/Vulkan/VRenderGraph.h"

namespace AE::Graphics
{
	
/*
=================================================
	constructor
=================================================
*/
	VCommandBatch::CmdBufPool::CmdBufPool ()
	{
		Reset();
	}

/*
=================================================
	Acquire
----
	thread safe
=================================================
*/
	uint  VCommandBatch::CmdBufPool::Acquire ()
	{
		ASSERT( not IsReady() );

		uint	idx = _counter.fetch_add( 1, EMemoryOrder::Relaxed );
		if ( idx < _pool.size() )
			return idx;

		return UMax;
	}
	
/*
=================================================
	Add
----
	thread safe for unique id
=================================================
*/
	void  VCommandBatch::CmdBufPool::Add (INOUT uint& idx, VkCommandBuffer cmdbuf)
	{
		if ( idx < _pool.size() )
		{
			DBG_SHAREDLOCK( _drCheck );

			_pool[idx].vk = cmdbuf;

			uint	old_bits = _ready.fetch_or( 1u << idx, EMemoryOrder::Release );
			ASSERT( (old_bits & (1u << idx)) == 0 );
		}
		idx = UMax;
	}
	
/*
=================================================
	Add
----
	thread safe for unique id
=================================================
*/
	void  VCommandBatch::CmdBufPool::Add (INOUT uint& idx, VBakedCommands &&ctx)
	{
		if ( idx < _pool.size() )
		{
			DBG_SHAREDLOCK( _drCheck );

			PlacementNew<VBakedCommands>( &_pool[idx].baked, RVRef( ctx ));
			_cmdTypes.fetch_or( 1u << idx, EMemoryOrder::Relaxed );

			uint	old_bits = _ready.fetch_or( 1u << idx, EMemoryOrder::Release );
			ASSERT( (old_bits & (1u << idx)) == 0 );
		}
		idx = UMax;
	}
	
/*
=================================================
	Complete
----
	thread safe for unique id
=================================================
*/
	void  VCommandBatch::CmdBufPool::Complete (INOUT uint& idx)
	{
		if ( idx < _pool.size() )
		{
			uint	old_bits = _ready.fetch_or( 1u << idx, EMemoryOrder::Relaxed );
			ASSERT( (old_bits & (1u << idx)) == 0 );
		}
		idx = UMax;
	}

/*
=================================================
	Lock
----
	call once.
	not thread safe !!!
=================================================
*/
	void  VCommandBatch::CmdBufPool::Lock ()
	{
		DBG_SHAREDLOCK( _drCheck );

		const uint	count = _counter.exchange( uint(_pool.size()), EMemoryOrder::Relaxed );
		_count = Min( count, uint(_pool.size()) );

		// set unused bits to 1
		uint	mask = ToBitMask<uint>(_pool.size()) - ToBitMask<uint>(_count);
		if ( mask )
			_ready.fetch_or( mask, EMemoryOrder::Relaxed );
	}

/*
=================================================
	Reset
----
	unlock and reset.
	not thread safe !!!
=================================================
*/
	void  VCommandBatch::CmdBufPool::Reset ()
	{
		DBG_EXLOCK( _drCheck );

		DEBUG_ONLY({
			uint	types = _cmdTypes.load( EMemoryOrder::Relaxed );
			while ( types != 0 )
			{
				int	i = IntLog2( ExtractBit( INOUT types ));
				ASSERT( not _pool[i].baked.IsValid() );
			}
		})

		std::memset( _pool.data(), 0, sizeof(_pool) );
		_count = 0;
		
		_counter.store( 0, EMemoryOrder::Relaxed );
		_ready.store( 0, EMemoryOrder::Relaxed );
		_cmdTypes.store( 0, EMemoryOrder::Relaxed );

		ThreadFence( EMemoryOrder::Release );
	}
	
/*
=================================================
	IsReady
=================================================
*/
	bool  VCommandBatch::CmdBufPool::IsReady ()
	{
		return _ready.load( EMemoryOrder::Relaxed ) == UMax;
	}

/*
=================================================
	GetCommands
----
	not thread safe !!!
=================================================
*/
	void  VCommandBatch::CmdBufPool::GetCommands (OUT Array<VkCommandBuffer> &cmdbufs)
	{
		DBG_EXLOCK( _drCheck );
		ThreadFence( EMemoryOrder::Acquire );

		ASSERT( _cmdTypes.load( EMemoryOrder::Relaxed ) == 0 );	// software command buffers is not supported yet
		ASSERT( IsReady() );

		for (uint i = 0; i < _count; ++i)
		{
			// command buffer can be null
			if ( _pool[i].vk != VK_NULL_HANDLE )
			{
				cmdbufs.push_back( _pool[i].vk );
			}
		}

		DEBUG_ONLY( Reset() );
	}
	
/*
=================================================
	CommitIndirectBuffers
----
	not thread safe !!!
=================================================
*/
	bool  VCommandBatch::CmdBufPool::CommitIndirectBuffers (VCommandPoolManager &cmdPoolMngr, EQueueType queue, ECommandBufferType cmdbufType)
	{
		uint	cmd_types = _cmdTypes.exchange( 0, EMemoryOrder::Relaxed );
		if ( cmd_types == 0 )
			return true;

		DBG_EXLOCK( _drCheck );
		ThreadFence( EMemoryOrder::Acquire );

		auto&	dev = cmdPoolMngr.GetDevice();
		
		while ( cmd_types != 0 )
		{
			int		i		= IntLog2( ExtractBit( INOUT cmd_types ));
			auto&	item	= _pool[i];
			auto	cmdbuf	= cmdPoolMngr.GetCommandBuffer( queue, cmdbufType );

			CHECK_ERR( item.baked.Execute( dev, cmdbuf.Get() ));
			item.baked.Destroy();

			VK_CHECK( dev.vkEndCommandBuffer( cmdbuf.Get() ));

			item.vk = cmdbuf.Get();

			cmdbuf.Release();
		}

		return true;
	}
//-----------------------------------------------------------------------------

	
	
/*
=================================================
	constructor
=================================================
*/
	VCommandBatch::VCommandBatch (uint indexInPool) :
		_indexInPool{ CheckCast<ubyte>( indexInPool )}
	{
		_fence = RenderGraph().GetResourceManager().CreateFence();
	}
	
/*
=================================================
	destructor
=================================================
*/
	VCommandBatch::~VCommandBatch ()
	{
		//RenderGraph().GetResourceManager().ReleaseFences({ _fence });		// TODO: check
	}

/*
=================================================
	_Create
=================================================
*/
	bool  VCommandBatch::_Create (EQueueType queue, uint frameId, uint frameUID, StringView dbgName)
	{
		_queueType	= queue;
		_frameId	= CheckCast<ubyte>( frameId );

		DEBUG_ONLY(
			_dbgName	= dbgName;
			_frameUID	= frameUID;
		)

		_status.store( EStatus::Initial, EMemoryOrder::Relaxed );

		return true;
	}

/*
=================================================
	Wait
=================================================
*/
	bool  VCommandBatch::Wait (nanoseconds timeout)
	{
		if ( IsComplete() )
			return true;

		auto&		rg	= RenderGraph();
		auto&		dev	= rg.GetDevice();
		VkResult	res	= dev.vkWaitForFences( dev.GetVkDevice(), 1, &_fence, VK_TRUE, timeout.count() );
		
		return (res == VK_SUCCESS);
	}
	
/*
=================================================
	IsComplete
=================================================
*/
	bool  VCommandBatch::IsComplete ()
	{
		return _status.load( EMemoryOrder::Relaxed ) == EStatus::Complete;
	}
	
/*
=================================================
	IsSubmitted
=================================================
*/
	bool  VCommandBatch::IsSubmitted ()
	{
		return _status.load( EMemoryOrder::Relaxed ) >= EStatus::Pending;
	}

/*
=================================================
	_ReleaseObject
=================================================
*/
	void  VCommandBatch::_ReleaseObject ()
	{
		ASSERT( IsComplete() );
		VRenderGraph::CommandBatchApi::Recycle( _indexInPool );
	}
	
/*
=================================================
	Submit
=================================================
*/
	bool  VCommandBatch::Submit (ESubmitMode mode)
	{
		CHECK_ERR( _status.exchange( EStatus::Pending ) == EStatus::Initial );

		_cmdPool.Lock();
		CHECK_ERR( _cmdPool.IsReady() );

		CHECK_ERR( _cmdPool.CommitIndirectBuffers( RenderGraph().GetCommandPoolManager(), GetQueueType(), ECommandBufferType::Primary_OneTimeSubmit ));

		return VRenderGraph::CommandBatchApi::Submit( *this, mode );
	}
	
/*
=================================================
	AddDependency
=================================================
*/
	bool  VCommandBatch::AddDependency (RC<VCommandBatch> batch)
	{
		CHECK_ERR( not IsSubmitted() );
		EXLOCK( _depsQuard );

		for (auto& dep : _dependsOn)
		{
			if ( dep == batch )
				return true;
		}

		return _dependsOn.try_emplace_back( RVRef(batch) );
	}
	
/*
=================================================
	_OnSubmit
=================================================
*/
	void  VCommandBatch::_OnSubmit ()
	{
		CHECK( _status.exchange( EStatus::Submitted ) == EStatus::Pending );
		_cmdPool.Reset();
	}
	
/*
=================================================
	_OnComplete
=================================================
*/
	void  VCommandBatch::_OnComplete ()
	{
		{
			EXLOCK( _depsQuard );
			_dependsOn.clear();
		}
		CHECK( _status.exchange( EStatus::Complete ) == EStatus::Submitted );
	}


}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
