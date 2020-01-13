// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

namespace AE::Graphics
{

	//
	// Local Buffer
	//

	class VRenderGraph::LocalBuffer
	{
	// types
	private:
		//using BufferRange = ResourceDataRange< VkDeviceSize >;
		
		struct BufferAccess
		{
		// variables
			VkPipelineStageFlags	stages		= 0;
			VkAccessFlags			access		= 0;
			ExeOrderIndex			index		= ExeOrderIndex::Initial;
			bool					isReadable	: 1;
			bool					isWritable	: 1;

		// methods
			BufferAccess () : isReadable{false}, isWritable{false} {}

			ND_ bool IsEnabled () const	{ return stages | access; }
		};


	// variables
	private:
		Ptr<VBuffer const>		_bufferData;	// readonly access is thread safe
		mutable bool			_isMutable	= true;
		mutable BufferAccess	_pendingAccesses;
		mutable BufferAccess	_accessForWrite;
		mutable BufferAccess	_accessForRead;


	// methods
	public:
		LocalBuffer () {}

		bool Create (const VBuffer *buf);
		
		void AddPendingState (EResourceState state, ExeOrderIndex order) const;
		void CommitBarrier (BarrierManager &) const;
		
		ND_ bool				IsMutable ()		const	{ return _isMutable; }
		ND_ VkBuffer			Handle ()			const	{ return _bufferData->Handle(); }
		ND_ VBuffer const*		ToGlobal ()			const	{ return _bufferData.get(); }
		//ND_ BufferDesc const&	Description ()		const	{ return _bufferData->Description(); }
		ND_ BytesU				Size ()				const	{ return _bufferData->Description().size; }
		ND_ EBufferUsage		Usage ()			const	{ return _bufferData->Description().usage; }
		ND_ bool				IsHostVisible ()	const	{ return EnumAny( _bufferData->Description().memType, EMemoryType::HostCocherent | EMemoryType::HostCached ); }
		ND_ bool				IsHostCached ()		const	{ return EnumEq( _bufferData->Description().memType, EMemoryType::HostCached ); }
	};


/*
=================================================
	Create
=================================================
*/
	bool  VRenderGraph::LocalBuffer::Create (const VBuffer *bufferData)
	{
		CHECK_ERR( _bufferData == null );
		CHECK_ERR( bufferData );

		_bufferData			= bufferData;
		_isMutable			= not _bufferData->IsReadOnly();
		_accessForWrite		= Default;
		_accessForRead		= Default;
		_pendingAccesses	= Default;

		return true;
	}
	
/*
=================================================
	AddPendingState
=================================================
*/
	void  VRenderGraph::LocalBuffer::AddPendingState (EResourceState state, ExeOrderIndex order) const
	{
		ASSERT( _isMutable );
		
		_pendingAccesses.stages		|= EResourceState_ToPipelineStages( state );
		_pendingAccesses.access		|= EResourceState_ToAccess( state );
		_pendingAccesses.isReadable	|= EResourceState_IsReadable( state );
		_pendingAccesses.isWritable	|= EResourceState_IsWritable( state );
		_pendingAccesses.index		 = Max( _pendingAccesses.index, order );
	}
	
/*
=================================================
	CommitBarrier
=================================================
*/
	void  VRenderGraph::LocalBuffer::CommitBarrier (BarrierManager &barrierMngr) const
	{
		ASSERT( _isMutable );
		
		if ( _pendingAccesses.isWritable or _pendingAccesses.isReadable )
		{
			ASSERT( _accessForWrite.index < _pendingAccesses.index );
			ASSERT( _accessForRead.index < _pendingAccesses.index );
		}

		VkBufferMemoryBarrier	barrier = {};
		barrier.sType				= VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		barrier.buffer				= Handle();
		barrier.offset				= 0;
		barrier.size				= VK_WHOLE_SIZE;
		barrier.srcQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;

		// write after write or write after read
		if ( _pendingAccesses.isWritable )
		{
			barrier.srcAccessMask	= _accessForWrite.access | _accessForRead.access;
			barrier.dstAccessMask	= _pendingAccesses.access;

			barrierMngr.AddBufferBarrier( _accessForWrite.stages | _accessForRead.stages, _pendingAccesses.stages, barrier );
			_accessForWrite = _pendingAccesses;
			_accessForRead	= Default;
		}
		else
		// read after write
		if ( _pendingAccesses.isReadable & _accessForWrite.IsEnabled() )
		{
			barrier.srcAccessMask	= _accessForWrite.access;
			barrier.dstAccessMask	= _pendingAccesses.access;

			barrierMngr.AddBufferBarrier( _accessForWrite.stages, _pendingAccesses.stages, barrier );
			_accessForRead	= _pendingAccesses;
			_accessForWrite = Default;
		}
		else
		// parallel reading
		if ( _pendingAccesses.isReadable )
		{
			_accessForRead.stages	|= _pendingAccesses.stages;
			_accessForRead.access	|= _pendingAccesses.access;
			_accessForRead.index	 = Max( _accessForRead.index, _pendingAccesses.index );
		}
	}


}	// AE::Graphics
