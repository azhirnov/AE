// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#ifdef AE_ENABLE_VULKAN

# include "graphics/Vulkan/RenderGraph/VLocalResRangesManager.h"

namespace AE::Graphics
{

/*
=================================================
	destructor
=================================================
*/
	VLocalResRangesManager::LocalBuffer::~LocalBuffer ()
	{
		ASSERT( _bufferData == null );
	}

/*
=================================================
	Create
=================================================
*/
	bool  VLocalResRangesManager::LocalBuffer::Create (const VBuffer *bufferData)
	{
		CHECK_ERR( _bufferData == null );
		CHECK_ERR( bufferData );

		_bufferData		= bufferData;
		_isMutable		= true; //_bufferData->IsReadOnly();

		return true;
	}
	
/*
=================================================
	Destroy
=================================================
*/
	void  VLocalResRangesManager::LocalBuffer::Destroy (VBarrierManager &barrierMngr)
	{
		ASSERT( _pendingAccesses.empty() and "you must commit all pending states before reseting" );
		
		if ( _isMutable )
		{
			// add full range barrier
			{
				BufferAccess		pending;
				pending.range		= BufferRange{ 0, VkDeviceSize(Size()) };
				pending.stages		= VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
				//pending.access	= _bufferData->GetAllReadAccessMask();
				pending.isReadable	= true;
				pending.isWritable	= false;
				pending.cmdIndex	= UMax;

				_pendingAccesses.push_back( std::move(pending) );
			}

			CommitBarrier( barrierMngr );
		}

		_accessForWrite.clear();
		_accessForRead.clear();

		_bufferData	= null;
	}
	
/*
=================================================
	GetMemoryInfo
=================================================
*/
	bool  VLocalResRangesManager::LocalBuffer::GetMemoryInfo (OUT VulkanMemoryObjInfo &info) const
	{
		return _bufferData->GetMemoryInfo( OUT info );
	}

/*
=================================================
	_FindFirstAccess
=================================================
*/
	inline VLocalResRangesManager::LocalBuffer::AccessIter_t
		VLocalResRangesManager::LocalBuffer::_FindFirstAccess (AccessRecords_t &arr, const BufferRange &otherRange)
	{
		usize	left	= 0;
		usize	right	= arr.size();

		for (; left < right; )
		{
			usize	mid = (left + right) >> 1;

			if ( arr[mid].range.end < otherRange.begin )
				left = mid + 1;
			else
				right = mid;
		}

		if ( left < arr.size() and arr[left].range.end >= otherRange.begin )
			return arr.begin() + left;
		
		return arr.end();
	}
	
/*
=================================================
	_ReplaceAccessRecords
=================================================
*/
	inline void  VLocalResRangesManager::LocalBuffer::_ReplaceAccessRecords (INOUT AccessRecords_t &arr, AccessIter_t iter, const BufferAccess &barrier)
	{
		ASSERT( iter >= arr.begin() and iter <= arr.end() );

		bool replaced = false;

		for (; iter != arr.end();)
		{
			if ( iter->range.begin <  barrier.range.begin and
				 iter->range.end   <= barrier.range.end )
			{
				//	|1111111|22222|
				//     |bbbbb|			+
				//  |11|....			=
				iter->range.end = barrier.range.begin;
				++iter;
				continue;
			}

			if ( iter->range.begin < barrier.range.begin and
				 iter->range.end   > barrier.range.end )
			{
				//  |111111111111111|
				//      |bbbbb|			+
				//  |111|bbbbb|11111|	=

				const auto	src = *iter;

				iter->range.end = barrier.range.begin;

				iter = arr.insert( iter+1, barrier );
				replaced = true;

				iter = arr.insert( iter+1, src );

				iter->range.begin = barrier.range.end;
				break;
			}

			if ( iter->range.begin >= barrier.range.begin and
				 iter->range.begin <  barrier.range.end )
			{
				if ( iter->range.end > barrier.range.end )
				{
					//  ...|22222222222|
					//   |bbbbbbb|			+
					//  ...bbbbbb|22222|	=
					iter->range.begin = barrier.range.end;

					if ( not replaced )
					{
						arr.insert( iter, barrier );
						replaced = true;
					}
					break;
				}

				if ( replaced )
				{
					//  ...|22222|33333|
					//   |bbbbbbbbbbb|		+
					//  ...|bbbbbbbbb|...	=
					iter = arr.erase( iter );
				}
				else
				{
					*iter = barrier;
					++iter;
					replaced = true;
				}
				continue;
			}

			break;
		}
			
		if ( not replaced )
		{
			arr.insert( iter, barrier );
		}
	}
	
/*
=================================================
	_EraseAccessRecords
=================================================
*/
	inline VLocalResRangesManager::LocalBuffer::AccessIter_t
		VLocalResRangesManager::LocalBuffer::_EraseAccessRecords (INOUT AccessRecords_t &arr, AccessIter_t iter, const BufferRange &range)
	{
		if ( arr.empty() )
			return iter;

		ASSERT( iter >= arr.begin() and iter <= arr.end() );

		for (; iter != arr.end();)
		{
			if ( iter->range.begin < range.begin and
				 iter->range.end   > range.end )
			{
				const auto	src = *iter;
				iter->range.end = range.begin;

				iter = arr.insert( iter+1, src );
				iter->range.begin = range.end;
				break;
			}

			if ( iter->range.begin < range.begin )
			{
				ASSERT( iter->range.end >= range.begin );

				iter->range.end = range.begin;
				++iter;
				continue;
			}

			if ( iter->range.end > range.end )
			{
				ASSERT( iter->range.begin <= range.end );

				iter->range.begin = range.end;
				break;
			}

			iter = arr.erase( iter );
		}

		return iter;
	}
	
/*
=================================================
	SetInitialState
=================================================
*/
	void  VLocalResRangesManager::LocalBuffer::SetInitialState (EResourceState state) const
	{
		// buffer must be in initial state
		ASSERT( _pendingAccesses.empty() );
		ASSERT( _accessForWrite.empty() );
		ASSERT( _accessForRead.empty() );
		
		if ( EResourceState_IsWritable( state ))
		{
			auto&	curr = _accessForWrite.front();
			curr.range		= BufferRange{ 0, VkDeviceSize(Size()) };
			curr.stages		= EResourceState_ToPipelineStages( state );
			curr.access		= EResourceState_ToAccess( state );
			curr.isReadable	= EResourceState_IsReadable( state );
			curr.isWritable	= EResourceState_IsWritable( state );
		}
		else
		{
			auto&	curr = _accessForRead.front();
			curr.range		= BufferRange{ 0, VkDeviceSize(Size()) };
			curr.stages		= EResourceState_ToPipelineStages( state );
			curr.access		= EResourceState_ToAccess( state );
			curr.isReadable	= EResourceState_IsReadable( state );
			curr.isWritable	= EResourceState_IsWritable( state );
		}

		_isMutable = true;
	}

/*
=================================================
	AddPendingState
=================================================
*/
	void  VLocalResRangesManager::LocalBuffer::AddPendingState (const BufferState &bs) const
	{
		ASSERT( bs.range.begin < Size() and bs.range.end <= Size() );

		if ( not _isMutable )
			return;

		BufferAccess		pending;
		pending.range		= bs.range;
		pending.stages		= EResourceState_ToPipelineStages( bs.state );
		pending.access		= EResourceState_ToAccess( bs.state );
		pending.isReadable	= EResourceState_IsReadable( bs.state );
		pending.isWritable	= EResourceState_IsWritable( bs.state );
		pending.cmdIndex	= bs.cmdIndex;
		
		
		// merge with pending
		BufferRange	range	= pending.range;
		auto		iter	= _FindFirstAccess( _pendingAccesses, range );

		if ( iter != _pendingAccesses.end() and iter->range.begin > range.begin )
		{
			pending.range = BufferRange{ range.begin, iter->range.begin };
			
			iter = _pendingAccesses.insert( iter, pending );
			++iter;

			range.begin = iter->range.begin;
		}

		for (; iter != _pendingAccesses.end() and iter->range.IsIntersects( range ); ++iter)
		{
			ASSERT( iter->cmdIndex == pending.cmdIndex and "something goes wrong - resource has uncommited state from another task" );

			iter->range.begin	= Min( iter->range.begin, range.begin );
			range.begin			= iter->range.end;
							
			iter->stages		|= pending.stages;
			iter->access		|= pending.access;
			iter->isReadable	|= pending.isReadable;
			iter->isWritable	|= pending.isWritable;
		}

		if ( not range.IsEmpty() )
		{
			pending.range = range;
			_pendingAccesses.insert( iter, pending );
		}
	}

/*
=================================================
	CommitBarrier
=================================================
*/
	void  VLocalResRangesManager::LocalBuffer::CommitBarrier (VBarrierManager &barrierMngr) const
	{
		if ( not _isMutable )
			return;

		const auto	AddBarrier	= [this, &barrierMngr] (const BufferRange &sharedRange, const BufferAccess &src, const BufferAccess &dst)
		{
			if ( not sharedRange.IsEmpty() )
			{
				VkBufferMemoryBarrier	barrier = {};
				barrier.sType				= VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
				barrier.pNext				= null;
				barrier.buffer				= Handle();
				barrier.offset				= sharedRange.begin;
				barrier.size				= sharedRange.Count();
				barrier.srcAccessMask		= src.access;
				barrier.dstAccessMask		= dst.access;
				barrier.srcQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
				barrier.dstQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;

				barrierMngr.AddBufferBarrier( src.stages, dst.stages, barrier );
			}
		};

		for (const auto& pending : _pendingAccesses)
		{
			const auto	w_iter	= _FindFirstAccess( _accessForWrite, pending.range );
			const auto	r_iter	= _FindFirstAccess( _accessForRead, pending.range );

			if ( pending.isWritable )
			{
				// write -> write, read -> write barriers
				bool	ww_barrier	= true;

				if ( w_iter != _accessForWrite.end() and r_iter != _accessForRead.end() )
				{
					ww_barrier = (w_iter->cmdIndex >= r_iter->cmdIndex);
				}

				if ( ww_barrier and w_iter != _accessForWrite.end() )
				{
					// write -> write barrier
					for (auto iter = w_iter; iter != _accessForWrite.end() and iter->range.begin < pending.range.end; ++iter)
					{
						AddBarrier( iter->range.Intersect( pending.range ), *iter, pending );
					}
				}
				else
				if ( r_iter != _accessForRead.end() )
				{
					// read -> write barrier
					for (auto iter = r_iter; iter != _accessForRead.end() and iter->range.begin < pending.range.end; ++iter)
					{
						AddBarrier( iter->range.Intersect( pending.range ), *iter, pending );
					}
				}
				
				// store to '_accessForWrite'
				_ReplaceAccessRecords( _accessForWrite, w_iter, pending );
				_EraseAccessRecords( _accessForRead, r_iter, pending.range );
			}
			else
			{
				// write -> read barrier only
				for (auto iter = w_iter; iter != _accessForWrite.end() and iter->range.begin < pending.range.end; ++iter)
				{
					AddBarrier( iter->range.Intersect( pending.range ), *iter, pending );
				}

				// store to '_accessForRead'
				_ReplaceAccessRecords( _accessForRead, r_iter, pending );
			}
		}

		_pendingAccesses.clear();
	}
//-----------------------------------------------------------------------------


	
/*
=================================================
	destructor
=================================================
*/
	VLocalResRangesManager::LocalImage::~LocalImage ()
	{
		ASSERT( _imageData == null );
	}

/*
=================================================
	Create
=================================================
*/
	bool  VLocalResRangesManager::LocalImage::Create (const VImage *imageData)
	{
		CHECK_ERR( _imageData == null );
		CHECK_ERR( imageData );

		_imageData		= imageData;
		_finalLayout	= _imageData->DefaultLayout();
		_isMutable		= true; //_imageData->IsReadOnly();
		
		// set initial state
		{
			ImageAccess		pending;
			pending.isReadable	= false;
			pending.isWritable	= false;
			pending.stages		= VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			pending.access		= Zero;
			pending.layout		= _imageData->DefaultLayout();
			pending.cmdIndex	= 0;
			pending.range		= SubRange{ 0, ArrayLayers() * MipmapLevels() };

			_accessForReadWrite.push_back( std::move(pending) );
		}

		return true;
	}
	
/*
=================================================
	Destroy
=================================================
*/
	void  VLocalResRangesManager::LocalImage::Destroy (VBarrierManager &barrierMngr)
	{
		ASSERT( _pendingAccesses.empty() and "you must commit all pending states before reseting" );
		
		if ( _isMutable )
		{
			// add full range barrier
			{
				ImageAccess		pending;
				pending.isReadable		= true;
				pending.isWritable		= false;
				pending.invalidateBefore= false;
				pending.invalidateAfter	= false;
				pending.stages			= VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
				//pending.access		= _imageData->GetAllReadAccessMask();
				pending.layout			= _finalLayout;
				pending.cmdIndex		= UMax;
				pending.range			= SubRange{ 0, ArrayLayers() * MipmapLevels() };

				_pendingAccesses.push_back( std::move(pending) );
			}

			CommitBarrier( barrierMngr );
		}

		_pendingAccesses.clear();
		_accessForReadWrite.clear();

		_imageData = null;
	}

/*
=================================================
	_FindFirstAccess
=================================================
*/
	inline VLocalResRangesManager::LocalImage::AccessIter_t
		VLocalResRangesManager::LocalImage::_FindFirstAccess (AccessRecords_t &arr, const SubRange &otherRange)
	{
		usize	left	= 0;
		usize	right	= arr.size();

		for (; left < right; )
		{
			usize	mid = (left + right) >> 1;

			if ( arr[mid].range.end < otherRange.begin )
				left = mid + 1;
			else
				right = mid;
		}

		if ( left < arr.size() and arr[left].range.end >= otherRange.begin )
			return arr.begin() + left;
		
		return arr.end();
	}
	
/*
=================================================
	_ReplaceAccessRecords
=================================================
*/
	inline void  VLocalResRangesManager::LocalImage::_ReplaceAccessRecords (INOUT AccessRecords_t &arr, AccessIter_t iter, const ImageAccess &barrier)
	{
		ASSERT( iter >= arr.begin() and iter <= arr.end() );

		bool replaced = false;

		for (; iter != arr.end();)
		{
			if ( iter->range.begin <  barrier.range.begin and
				 iter->range.end   <= barrier.range.end )
			{
				//	|1111111|22222|
				//     |bbbbb|			+
				//  |11|....			=
				iter->range.end = barrier.range.begin;
				++iter;
				continue;
			}

			if ( iter->range.begin < barrier.range.begin and
				 iter->range.end   > barrier.range.end )
			{
				//  |111111111111111|
				//      |bbbbb|			+
				//  |111|bbbbb|11111|	=

				const auto	src = *iter;

				iter->range.end = barrier.range.begin;

				iter = arr.insert( iter+1, barrier );
				replaced = true;

				iter = arr.insert( iter+1, src );

				iter->range.begin = barrier.range.end;
				break;
			}

			if ( iter->range.begin >= barrier.range.begin and
				 iter->range.begin <  barrier.range.end )
			{
				if ( iter->range.end > barrier.range.end )
				{
					//  ...|22222222222|
					//   |bbbbbbb|			+
					//  ...bbbbbb|22222|	=
					iter->range.begin = barrier.range.end;

					if ( not replaced )
					{
						arr.insert( iter, barrier );
						replaced = true;
					}
					break;
				}

				if ( replaced )
				{
					//  ...|22222|33333|
					//   |bbbbbbbbbbb|		+
					//  ...|bbbbbbbbb|...	=
					iter = arr.erase( iter );
				}
				else
				{
					*iter = barrier;
					++iter;
					replaced = true;
				}
				continue;
			}

			break;
		}
			
		if ( not replaced )
		{
			arr.insert( iter, barrier );
		}
	}
	
/*
=================================================
	SetInitialState
=================================================
*/
	void  VLocalResRangesManager::LocalImage::SetInitialState (EResourceState state) const
	{
		// image must be in initial state
		ASSERT( _accessForReadWrite.size() == 1 );
		ASSERT( _accessForReadWrite.front().cmdIndex == 0 );
		
		auto&	curr = _accessForReadWrite.front();

		curr.isReadable			= EResourceState_IsReadable( state );
		curr.isWritable			= EResourceState_IsWritable( state );
		curr.invalidateBefore	= false;
		curr.invalidateAfter	= false;
		curr.stages				= EResourceState_ToPipelineStages( state );
		curr.access				= EResourceState_ToAccess( state );
		curr.layout				= EResourceState_ToImageLayout( state, AspectMask() );

		_isMutable = true;
	}
	
	void  VLocalResRangesManager::LocalImage::SetInitialState (EResourceState state, EImageLayout layout) const
	{
		// image must be in initial state
		ASSERT( _accessForReadWrite.size() == 1 );
		ASSERT( _accessForReadWrite.front().cmdIndex == 0 );
		
		auto&	curr = _accessForReadWrite.front();

		curr.isReadable			= EResourceState_IsReadable( state );
		curr.isWritable			= EResourceState_IsWritable( state );
		curr.invalidateBefore	= false;
		curr.invalidateAfter	= false;
		curr.stages				= EResourceState_ToPipelineStages( state );
		curr.access				= EResourceState_ToAccess( state );
		curr.layout				= VEnumCast( layout );

		_isMutable = true;
	}

/*
=================================================
	AddPendingState
=================================================
*/
	void  VLocalResRangesManager::LocalImage::AddPendingState (const ImageState &is) const
	{
		ASSERT( is.range.Mipmaps().begin < MipmapLevels() and is.range.Mipmaps().end <= MipmapLevels() );
		ASSERT( is.range.Layers().begin < ArrayLayers() and is.range.Layers().end <= ArrayLayers() );

		if ( not _isMutable )
			return;

		ImageAccess		pending;
		pending.isReadable		= EResourceState_IsReadable( is.state );
		pending.isWritable		= EResourceState_IsWritable( is.state );
		pending.invalidateBefore= false; //AllBits( is.state, EResourceState::InvalidateBefore );
		pending.invalidateAfter	= false; //AllBits( is.state, EResourceState::InvalidateAfter );
		pending.stages			= EResourceState_ToPipelineStages( is.state );
		pending.access			= EResourceState_ToAccess( is.state );
		pending.layout			= is.layout;
		pending.cmdIndex		= is.cmdIndex;
		

		// extract sub ranges
		const uint		arr_layers	= ArrayLayers();
		const uint		mip_levels	= MipmapLevels();
		Array<SubRange>	sub_ranges;
		SubRange		layer_range	 { is.range.Layers().begin,  Min( is.range.Layers().end,  arr_layers )};
		SubRange		mipmap_range { is.range.Mipmaps().begin, Min( is.range.Mipmaps().end, mip_levels )};

		if ( is.range.IsWholeLayers() and is.range.IsWholeMipmaps() )
		{
			sub_ranges.push_back(SubRange{ 0, arr_layers * mip_levels });
		}
		else
		if ( is.range.IsWholeLayers() )
		{
			uint	begin = mipmap_range.begin   * arr_layers + layer_range.begin;
			uint	end   = (mipmap_range.end-1) * arr_layers + layer_range.end;
				
			sub_ranges.push_back(SubRange{ begin, end });
		}
		else
		for (uint mip = mipmap_range.begin; mip < mipmap_range.end; ++mip)
		{
			uint	begin = mip * arr_layers + layer_range.begin;
			uint	end   = mip * arr_layers + layer_range.end;

			sub_ranges.push_back(SubRange{ begin, end });
		}


		// merge with pending
		for (auto& range : sub_ranges)
		{
			auto	iter = _FindFirstAccess( _pendingAccesses, range );

			if ( iter != _pendingAccesses.end() and iter->range.begin > range.begin )
			{
				pending.range = { range.begin, iter->range.begin };

				iter = _pendingAccesses.insert( iter, pending );
				++iter;

				range.begin = iter->range.begin;
			}

			for (; iter != _pendingAccesses.end() and iter->range.IsIntersects( range ); ++iter)
			{
				ASSERT( iter->cmdIndex == pending.cmdIndex and "something goes wrong - resource has uncommited state from another task" );
				ASSERT( iter->layout == pending.layout and "can't use different layouts inside single task" );

				iter->range.begin		= Min( iter->range.begin, range.begin );
				range.begin				= iter->range.end;
				iter->stages			|= pending.stages;
				iter->access			|= pending.access;
				iter->isReadable		|= pending.isReadable;
				iter->isWritable		|= pending.isWritable;
				iter->invalidateBefore	&= pending.invalidateBefore;
				iter->invalidateAfter	&= pending.invalidateAfter;
			}

			if ( not range.IsEmpty() )
			{
				pending.range = range;
				_pendingAccesses.insert( iter, pending );
			}
		}
	}

/*
=================================================
	CommitBarrier
=================================================
*/
	void  VLocalResRangesManager::LocalImage::CommitBarrier (VBarrierManager &barrierMngr) const
	{
		VkPipelineStageFlags	dst_stages = 0;

		for (const auto& pending : _pendingAccesses)
		{
			const auto	first = _FindFirstAccess( _accessForReadWrite, pending.range );

			for (auto iter = first; iter != _accessForReadWrite.end() and iter->range.begin < pending.range.end; ++iter)
			{
				const SubRange	range		= iter->range.Intersect( pending.range );
				const uint		arr_layers	= ArrayLayers();

				const bool		is_modified = (iter->layout != pending.layout)			or		// layout -> layout 
											  (iter->isReadable and pending.isWritable)	or		// read -> write
											  iter->isWritable;									// write -> read/write

				if ( not range.IsEmpty() and is_modified )
				{
					VkImageMemoryBarrier	barrier = {};
					barrier.sType				= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
					barrier.pNext				= null;
					barrier.image				= Handle();
					barrier.oldLayout			= (iter->invalidateAfter or pending.invalidateBefore) ? VK_IMAGE_LAYOUT_UNDEFINED : iter->layout;
					barrier.newLayout			= pending.layout;
					barrier.srcAccessMask		= iter->access;
					barrier.dstAccessMask		= pending.access;
					barrier.srcQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
					barrier.dstQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
			
					barrier.subresourceRange.aspectMask		= AspectMask();
					barrier.subresourceRange.baseMipLevel	= (range.begin / arr_layers);
					barrier.subresourceRange.levelCount		= (range.end - range.begin - 1) / arr_layers + 1;	// TODO: use power of 2 values?
					barrier.subresourceRange.baseArrayLayer	= (range.begin % arr_layers);
					barrier.subresourceRange.layerCount		= (range.end - range.begin - 1) % arr_layers + 1;

					ASSERT( barrier.subresourceRange.levelCount > 0 );
					ASSERT( barrier.subresourceRange.layerCount > 0 );

					dst_stages |= pending.stages;
					barrierMngr.AddImageBarrier( iter->stages, pending.stages, barrier );
				}
			}

			_ReplaceAccessRecords( _accessForReadWrite, first, pending );
		}

		_pendingAccesses.clear();
	}
//-----------------------------------------------------------------------------


	
/*
=================================================
	constructor
=================================================
*/
	VLocalResRangesManager::VLocalResRangesManager (const VCommandBatch& batch) :
		VBarrierManager{ batch },
		_resMngr{ RenderGraph().GetResourceManager() }
	{
	}

/*
=================================================
	destructor
=================================================
*/
	VLocalResRangesManager::~VLocalResRangesManager ()
	{
		ASSERT( _pendingBarriers.images.empty() );
		ASSERT( _pendingBarriers.buffers.empty() );
		ASSERT( _localRes.images.toLocal.empty() );
		ASSERT( _localRes.buffers.toLocal.empty() );
	}

/*
=================================================
	FlushBarriers
=================================================
*/
	void  VLocalResRangesManager::FlushBarriers ()
	{
		++_cmdCounter;

		for (auto& buf : _pendingBarriers.buffers) {
			buf->CommitBarrier( *this );
		}
		_pendingBarriers.buffers.clear();
		
		for (auto& img : _pendingBarriers.images) {
			img->CommitBarrier( *this );
		}
		_pendingBarriers.images.clear();
	}

/*
=================================================
	_FlushLocalResourceStates
=================================================
*/
	template <typename ID, typename ResPool>
	inline void  VLocalResRangesManager::_FlushLocalResourceStates (ResPool &localRes)
	{
		for (auto&[global_idx, local_idx] : localRes.toLocal)
		{
			auto&	res = localRes.pool[ local_idx ];

			res.Destroy( *this );
			localRes.pool.Unassign( local_idx );
		}

		localRes.toLocal.clear();
	}
	
/*
=================================================
	TransitToDefaultState
=================================================
*/
	void  VLocalResRangesManager::TransitToDefaultState ()
	{
		_FlushLocalResourceStates<BufferID>( _localRes.buffers );
		_FlushLocalResourceStates<ImageID>( _localRes.images );
	}
	
/*
=================================================
	AddBuffer
=================================================
*/
	void  VLocalResRangesManager::AddBuffer (const LocalBuffer *buf, EResourceState state, Bytes offsetInBytes, Bytes sizeInBytes)
	{
		if ( not buf->IsMutable() )
			return;

		const VkDeviceSize	buf_size	= VkDeviceSize(buf->Size());
		const VkDeviceSize	offset		= VkDeviceSize(offsetInBytes);
		VkDeviceSize		size		= VkDeviceSize(sizeInBytes);

		size = Min( buf_size, (size == VK_WHOLE_SIZE ? buf_size - offset : offset + size) );
		
		buf->AddPendingState( LocalBuffer::BufferState{ state, offset, size, _cmdCounter });
		
		_pendingBarriers.buffers.insert( buf );
	}

	void  VLocalResRangesManager::AddBuffer (const LocalBuffer *buf, EResourceState state, ArrayView<BufferImageCopy> copy, const LocalImage* img)
	{
		if ( not buf->IsMutable() )
			return;

		for (auto& reg : copy)
		{
			const uint			bits_per_px	= EPixelFormat_BitPerPixel( img->Description().format, reg.imageSubresource.aspectMask );
			const VkDeviceSize	row_pitch	= (reg.bufferRowLength * bits_per_px) / 8;
			const VkDeviceSize	slice_pitch	= reg.bufferImageHeight * row_pitch;
			const uint			dim_z		= Max( reg.imageSubresource.layerCount, reg.imageExtent.z );
		
			buf->AddPendingState( LocalBuffer::BufferState{ state, VkDeviceSize(reg.bufferOffset), slice_pitch * dim_z, _cmdCounter });
		}
		_pendingBarriers.buffers.insert( buf );
	}
	
/*
=================================================
	AddImage
=================================================
*/
	void  VLocalResRangesManager::AddImage (const LocalImage *img, EResourceState state, VkImageLayout layout, StructView<ImageSubresourceRange> subresRanges)
	{
		if ( not img->IsMutable() )
			return;

		for (auto& subres : subresRanges)
		{
			img->AddPendingState( LocalImage::ImageState{
				state, layout,
				LocalImage::ImageRange{ subres.baseArrayLayer, subres.layerCount, subres.baseMipLevel, subres.levelCount },
				VEnumCast( subres.aspectMask ),
				_cmdCounter
			});
		}
		_pendingBarriers.images.insert( img );
	}

	void  VLocalResRangesManager::AddImage (const LocalImage *img, EResourceState state, VkImageLayout layout, StructView<ImageSubresourceLayers> subresLayers)
	{
		if ( not img->IsMutable() )
			return;

		for (auto& subres : subresLayers)
		{
			img->AddPendingState( LocalImage::ImageState{
				state, layout,
				LocalImage::ImageRange{ subres.baseArrayLayer, subres.layerCount, subres.mipLevel, 1 },
				VEnumCast( subres.aspectMask ),
				_cmdCounter
			});
		}
		_pendingBarriers.images.insert( img );
	}

	void  VLocalResRangesManager::_AddImage (const LocalImage *img, EResourceState state, VkImageLayout layout, const ImageViewDesc &desc)
	{
		ASSERT( desc.layerCount > 0 and desc.levelCount > 0 );
		
		img->AddPendingState( LocalImage::ImageState{
			state, layout,
			LocalImage::ImageRange{ desc.baseLayer, desc.layerCount, desc.baseLevel, desc.levelCount },
			(EPixelFormat_HasDepth( desc.format )   ? VK_IMAGE_ASPECT_DEPTH_BIT   :
				EPixelFormat_HasStencil( desc.format ) ? VK_IMAGE_ASPECT_STENCIL_BIT : VK_IMAGE_ASPECT_COLOR_BIT),
			_cmdCounter
		});

		_pendingBarriers.images.insert( img );
	}
//-----------------------------------------------------------------------------


	
	//
	// Descriptor Set Barriers
	//
	class VLocalResRangesManager::DescriptorSetBarriers
	{
	// types
	private:
		using EDescriptorType = DescriptorSet::EDescriptorType;

	// variables
	private:
		VLocalResRangesManager &	_resMngr;
		ArrayView<uint>				_dynamicOffsets;

	// methods
	public:
		DescriptorSetBarriers (VLocalResRangesManager &rm, ArrayView<uint> offsets) : _resMngr{rm}, _dynamicOffsets{offsets} {}

		// ResourceGraph //
		void operator () (const UniformName &, EDescriptorType, const DescriptorSet::Buffer &);
		void operator () (const UniformName &, EDescriptorType, const DescriptorSet::TexelBuffer &);
		void operator () (const UniformName &, EDescriptorType, const DescriptorSet::Image &);
		void operator () (const UniformName &, EDescriptorType, const DescriptorSet::Texture &);
		void operator () (const UniformName &, EDescriptorType, const DescriptorSet::Sampler &) {}
	};

/*
=================================================
	operator (Buffer)
=================================================
*/
	void  VLocalResRangesManager::DescriptorSetBarriers::operator () (const UniformName &, EDescriptorType, const DescriptorSet::Buffer &buf)
	{
		for (uint i = 0; i < buf.elementCount; ++i)
		{
			auto&				elem	= buf.elements[i];
			LocalBuffer const*	buffer	= _resMngr.Get( elem.bufferId );

			if ( buffer )
			{
				// validation
				const VkDeviceSize	offset	= VkDeviceSize(elem.offset) + (buf.dynamicOffsetIndex < _dynamicOffsets.size() ? _dynamicOffsets[buf.dynamicOffsetIndex] : 0);		
				const VkDeviceSize	size	= VkDeviceSize(elem.size == ~0_b ? buffer->Size() - offset : elem.size);

				if ( buf.arrayStride == 0 ) {
					ASSERT( size == buf.staticSize );
				} else {
					ASSERT( size >= buf.staticSize );
					ASSERT( (size - buf.staticSize) % buf.arrayStride == 0 );
				}

				ASSERT( offset < buffer->Size() );
				ASSERT( offset + size <= buffer->Size() );

				auto&	limits	= _resMngr.GetDevice().GetProperties().properties.limits;
				Unused( limits );

				if ( (buf.state & EResourceState::_StateMask) == EResourceState::UniformRead )
				{
					ASSERT( (offset % limits.minUniformBufferOffsetAlignment) == 0 );
					ASSERT( size <= limits.maxUniformBufferRange );
				}else{
					ASSERT( (offset % limits.minStorageBufferOffsetAlignment) == 0 );
					ASSERT( size <= limits.maxStorageBufferRange );
				}

				_resMngr.AddBuffer( buffer, buf.state, Bytes{offset}, Bytes{size} );
			}
		}
	}

/*
=================================================
	operator (TexelBuffer)
=================================================
*/
	void  VLocalResRangesManager::DescriptorSetBarriers::operator () (const UniformName &, EDescriptorType, const DescriptorSet::TexelBuffer &texbuf)
	{
		for (uint i = 0; i < texbuf.elementCount; ++i)
		{
			auto&				elem	= texbuf.elements[i];
			LocalBuffer const*	buffer	= _resMngr.Get( elem.bufferId );

			if ( buffer )
			{
				const Bytes	offset	= elem.desc.offset;		
				const Bytes	size	= (elem.desc.size == ~0_b ? (buffer->Size() - offset) : elem.desc.size);

				_resMngr.AddBuffer( buffer, texbuf.state, offset, size );
			}
		}
	}

/*
=================================================
	operator (Image / Texture / SubpassInput)
=================================================
*/
	void  VLocalResRangesManager::DescriptorSetBarriers::operator () (const UniformName &, EDescriptorType, const DescriptorSet::Image &img)
	{
		for (uint i = 0; i < img.elementCount; ++i)
		{
			auto&				elem	= img.elements[i];
			LocalImage const*	image	= _resMngr.Get( elem.imageId );

			if ( image )
			{
				_resMngr._AddImage( image, img.state, EResourceState_ToImageLayout( img.state, image->AspectMask() ), elem.desc );
			}
		}
	}
	
	void  VLocalResRangesManager::DescriptorSetBarriers::operator () (const UniformName &, EDescriptorType, const DescriptorSet::Texture &tex)
	{
		for (uint i = 0; i < tex.elementCount; ++i)
		{
			auto&				elem	= tex.elements[i];
			LocalImage const*	image	= _resMngr.Get( elem.imageId );

			if ( image )
			{
				_resMngr._AddImage( image, tex.state, EResourceState_ToImageLayout( tex.state, image->AspectMask() ), elem.desc );
			}
		}
	}
	
/*
=================================================
	DescriptorsBarrier
=================================================
*/
	void  VLocalResRangesManager::DescriptorsBarrier (DescriptorSetID ds, ArrayView<uint> dynamicOffsets)
	{
		auto*	desc_set = Get( ds );
		CHECK_ERRV( desc_set );
		
		DescriptorSetBarriers	visitor{ *this, dynamicOffsets };
		desc_set->ForEachUniform( visitor );
	}


}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
