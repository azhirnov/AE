// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

namespace AE::Graphics
{
namespace
{

	//
	// Vulkan Local Buffer
	//

	class VLocalBuffer
	{
	// types
	private:
		struct PendingAccess
		{
			VkPipelineStageFlagBits	stages		= Zero;
			VkAccessFlagBits		access		= Zero;
			ExeOrderIndex			index		= ExeOrderIndex::Initial;
			bool					isReadable	: 1;
			bool					isWritable	: 1;

			PendingAccess () : isReadable{false}, isWritable{false} {}
		};

		struct CurrentAccess
		{
			VkPipelineStageFlagBits	writeStages		= Zero;
			VkAccessFlagBits		writeAccess		= Zero;
			VkPipelineStageFlagBits	readStages		= VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			VkAccessFlagBits		readAccess		= Zero;
			ExeOrderIndex			index			= ExeOrderIndex::Initial;
			VkAccessFlagBits		unavailable		= Zero;		// access flags that must be invalidated before reading
		};


	// variables
	private:
		Ptr<VBuffer const>		_bufferData;	// readonly access is thread safe
		mutable PendingAccess	_pendingAccess;
		mutable CurrentAccess	_currentState;
		mutable bool			_isMutable	= true;


	// methods
	public:
		VLocalBuffer () {}

		bool  Create (const VBuffer *buf);
		void  Destroy (VBarrierManager &);
		
		void  AddPendingState (EResourceState state, ExeOrderIndex order) const;
		void  CommitBarrier (VBarrierManager &) const;
		void  SetInitialState (EResourceState state) const;
		
		ND_ bool				IsMutable ()		const	{ return _isMutable; }
		ND_ VkBuffer			Handle ()			const	{ return _bufferData->Handle(); }
		ND_ VBuffer const*		ToGlobal ()			const	{ return _bufferData.get(); }
		ND_ BytesU				Size ()				const	{ return _bufferData->Description().size; }
		ND_ EBufferUsage		Usage ()			const	{ return _bufferData->Description().usage; }
		ND_ bool				IsHostVisible ()	const	{ return AnyBits( _bufferData->Description().memType, EMemoryType::HostCocherent | EMemoryType::HostCached ); }
		ND_ bool				IsHostCached ()		const	{ return AllBits( _bufferData->Description().memType, EMemoryType::HostCached ); }
	};


/*
=================================================
	Create
=================================================
*/
	bool  VLocalBuffer::Create (const VBuffer *bufferData)
	{
		CHECK_ERR( _bufferData == null );
		CHECK_ERR( bufferData );

		_bufferData		= bufferData;
		_isMutable		= true; //not _bufferData->IsReadOnly();
		_currentState	= Default;
		_pendingAccess	= Default;

		return true;
	}
	
/*
=================================================
	Destroy
=================================================
*/
	void  VLocalBuffer::Destroy (VBarrierManager &barrierMngr)
	{
		ASSERT( _isMutable );

		if ( _currentState.unavailable == Zero )
			return;
		
		VkBufferMemoryBarrier	barrier = {};
		barrier.sType				= VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		barrier.buffer				= Handle();
		barrier.offset				= 0;
		barrier.size				= VK_WHOLE_SIZE;
		barrier.srcQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
		barrier.srcAccessMask		= _currentState.writeAccess;
		barrier.dstAccessMask		= 0;

		barrierMngr.AddBufferBarrier( _currentState.readStages | _currentState.writeStages, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, barrier );
	}
	
/*
=================================================
	AddPendingState
=================================================
*/
	void  VLocalBuffer::AddPendingState (EResourceState state, ExeOrderIndex order) const
	{
		ASSERT( _isMutable );
		
		_pendingAccess.stages		|= EResourceState_ToPipelineStages( state );
		_pendingAccess.access		|= EResourceState_ToAccess( state );
		_pendingAccess.isReadable	|= EResourceState_IsReadable( state );
		_pendingAccess.isWritable	|= EResourceState_IsWritable( state );
		_pendingAccess.index		 = Max( _pendingAccess.index, order );
	}
	
/*
=================================================
	CommitBarrier
=================================================
*/
	void  VLocalBuffer::CommitBarrier (VBarrierManager &barrierMngr) const
	{
		ASSERT( _isMutable );
		ASSERT( _currentState.index < _pendingAccess.index );

		VkBufferMemoryBarrier	barrier = {};
		barrier.sType				= VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		barrier.buffer				= Handle();
		barrier.offset				= 0;
		barrier.size				= VK_WHOLE_SIZE;
		barrier.srcQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
		
		// write after write or write after read
		if ( _pendingAccess.isWritable )
		{
			barrier.srcAccessMask	= _currentState.writeAccess;	// read access has no effect
			barrier.dstAccessMask	= _pendingAccess.access;
			auto	stages			= _currentState.writeStages;

			// don't sync with write stages if has read stages because they happens after writing (write -> read -> current)
			if ( _currentState.readStages )
			{
				stages					= _currentState.readStages;
				barrier.srcAccessMask	= Zero;
			}

			barrierMngr.AddBufferBarrier( stages, _pendingAccess.stages, barrier );

			_currentState.writeStages	= _pendingAccess.stages;
			_currentState.writeAccess	= _pendingAccess.access;
			_currentState.readStages	= Zero;
			_currentState.readAccess	= Zero;
			_currentState.unavailable	= VReadAccessMask;	// all subsequent reads must invalidate cache before reading
			_currentState.index			= _pendingAccess.index;
		}
		else
		// read after write
		if ( AnyBits( _currentState.unavailable, _pendingAccess.access ))
		{
			barrier.srcAccessMask	= _currentState.writeAccess;
			barrier.dstAccessMask	= _pendingAccess.access;

			barrierMngr.AddBufferBarrier( _currentState.writeStages, _pendingAccess.stages, barrier );

			_currentState.unavailable	&= ~_pendingAccess.access;
			_currentState.readStages	|= _pendingAccess.stages;
			_currentState.readAccess	|= _pendingAccess.access;
			_currentState.index			 = _pendingAccess.index;
		}
		// parallel reading
		else
		{
			_currentState.readStages	|= _pendingAccess.stages;
			_currentState.readAccess	|= _pendingAccess.access;
			_currentState.index			 = _pendingAccess.index;
		}

		_pendingAccess = Default;
	}

/*
=================================================
	SetInitialState
=================================================
*/
	void  VLocalBuffer::SetInitialState (EResourceState state) const
	{
		_currentState = Default;

		const auto	stages	= EResourceState_ToPipelineStages( state );
		const auto	access	= EResourceState_ToAccess( state );

		if ( EResourceState_IsWritable( state ))
		{
			_currentState.writeStages	= stages;
			_currentState.writeAccess	= access;
			_currentState.unavailable	= VReadAccessMask;
		}
		else
		{
			_currentState.readStages	= stages;
			_currentState.readAccess	= access;
		}
	}


}	// namespace
}	// AE::Graphics
