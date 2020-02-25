// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

namespace AE::Graphics
{
namespace
{

	//
	// Vulkan Local Image
	//

	class VLocalImage
	{
	// types
	private:
		struct PendingAccess
		{
			VkPipelineStageFlagBits	stages		= Zero;
			VkAccessFlagBits		access		= Zero;
			VkImageLayout			layout		= Zero;
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
			VkImageLayout			layout			= VK_IMAGE_LAYOUT_UNDEFINED;
			ExeOrderIndex			index			= ExeOrderIndex::Initial;
			VkAccessFlagBits		unavailable		= Zero;		// access flags that must be invalidated before reading
		};


	// variables
	private:
		Ptr<VImage const>		_imageData;	// readonly access is thread safe
		mutable PendingAccess	_pendingAccess;
		mutable CurrentAccess	_currentState;
		mutable bool			_isMutable	= true;


	// methods
	public:
		VLocalImage () {}

		bool  Create (const VImage *);
		void  Destroy (VBarrierManager &);
		
		void  AddPendingState (EResourceState state, VkImageLayout layout, ExeOrderIndex order) const;
		void  CommitBarrier (VBarrierManager &) const;
		void  SetInitialState (EResourceState state) const;
		void  SetInitialState (EResourceState state, EImageLayout layout) const;

		ND_ bool				IsMutable ()	const	{ return true; } // TODO
		ND_ VkImage				Handle ()		const	{ return _imageData->Handle(); }
		ND_ VImage const*		ToGlobal ()		const	{ return _imageData.get(); }
		ND_ ImageDesc const&	Description ()	const	{ return _imageData->Description(); }
		ND_ uint3 const			Dimension ()	const	{ return _imageData->Description().dimension; }
		ND_ VkImageAspectFlags	AspectMask ()	const	{ return _imageData->AspectMask(); }
	};


/*
=================================================
	Create
=================================================
*/
	bool  VLocalImage::Create (const VImage *imageData)
	{
		CHECK_ERR( _imageData == null );
		CHECK_ERR( imageData );

		_imageData		= imageData;
		_isMutable		= true; //not _imageData->IsReadOnly();
		_pendingAccess	= Default;
		_currentState	= Default;
		_currentState.layout = _imageData->DefaultLayout();

		return true;
	}
	
/*
=================================================
	Destroy
=================================================
*/
	void  VLocalImage::Destroy (VBarrierManager &barrierMngr)
	{
		ASSERT( _isMutable );

		if ( _currentState.layout == _imageData->DefaultLayout() and _currentState.unavailable == Zero )
			return;
		
		VkImageMemoryBarrier	barrier = {};
		barrier.sType				= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.image				= Handle();
		barrier.oldLayout			= _currentState.layout;
		barrier.newLayout			= _imageData->DefaultLayout();
		barrier.subresourceRange	= { AspectMask(), 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS };
		barrier.srcQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
		barrier.srcAccessMask		= _currentState.writeAccess;
		barrier.dstAccessMask		= 0;
		
		barrierMngr.AddImageBarrier( _currentState.readStages | _currentState.writeStages, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, Zero, barrier );
	}
	
/*
=================================================
	AddPendingState
=================================================
*/
	void  VLocalImage::AddPendingState (EResourceState state, VkImageLayout layout, ExeOrderIndex order) const
	{
		ASSERT( _isMutable );
		CHECK( _pendingAccess.layout == layout or _pendingAccess.layout == VK_IMAGE_LAYOUT_UNDEFINED );

		_pendingAccess.stages		|= EResourceState_ToPipelineStages( state );
		_pendingAccess.access		|= EResourceState_ToAccess( state );
		_pendingAccess.layout		 = layout;
		_pendingAccess.isReadable	|= EResourceState_IsReadable( state );
		_pendingAccess.isWritable	|= EResourceState_IsWritable( state );
		_pendingAccess.index		 = Max( _pendingAccess.index, order );
	}

/*
=================================================
	CommitBarrier
=================================================
*/
	void  VLocalImage::CommitBarrier (VBarrierManager &barrierMngr) const
	{
		ASSERT( _isMutable );
		ASSERT( _currentState.index < _pendingAccess.index );

		VkImageMemoryBarrier	barrier = {};
		barrier.sType				= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.image				= Handle();
		barrier.oldLayout			= _currentState.layout;
		barrier.newLayout			= _pendingAccess.layout;
		barrier.subresourceRange	= { AspectMask(), 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS };
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

			barrierMngr.AddImageBarrier( stages, _pendingAccess.stages, Zero, barrier );

			_currentState.writeStages	= _pendingAccess.stages;
			_currentState.writeAccess	= _pendingAccess.access;
			_currentState.readStages	= Zero;
			_currentState.readAccess	= Zero;
			_currentState.unavailable	= VReadAccessMask;	// all subsequent reads must invalidate cache before reading
			_currentState.index			= _pendingAccess.index;
			_currentState.layout		= _pendingAccess.layout;
		}
		else
		// layout transition
		if ( _currentState.layout != _pendingAccess.layout )
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

			barrierMngr.AddImageBarrier( stages, _pendingAccess.stages, Zero, barrier );

			_currentState.unavailable	&= ~_pendingAccess.access;
			_currentState.readStages	 = _pendingAccess.stages;
			_currentState.readAccess	 = _pendingAccess.access;
			_currentState.index			 = _pendingAccess.index;
			_currentState.layout		 = _pendingAccess.layout;
		}
		else
		// read after write
		if ( EnumAny( _currentState.unavailable, _pendingAccess.access ))
		{
			barrier.srcAccessMask	= _currentState.writeAccess;	// read access has no effect
			barrier.dstAccessMask	= _pendingAccess.access;
			
			barrierMngr.AddImageBarrier( _currentState.writeStages, _pendingAccess.stages, Zero, barrier );

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
	void  VLocalImage::SetInitialState (EResourceState state) const
	{
		_currentState			= Default;
		_currentState.layout	= EResourceState_ToImageLayout( state, AspectMask() );

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

	void  VLocalImage::SetInitialState (EResourceState state, EImageLayout layout) const
	{
		_currentState			= Default;
		_currentState.layout	= VEnumCast( layout );

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
