// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#ifdef AE_ENABLE_VULKAN

# include "graphics/Vulkan/RenderGraph/VLocalResManager.h"

namespace AE::Graphics
{
	
/*
=================================================
	destructor
=================================================
*/
	VLocalResManager::LocalImage::~LocalImage ()
	{
		ASSERT( _imageData == null );
	}

/*
=================================================
	Create
=================================================
*/
	bool  VLocalResManager::LocalImage::Create (const VImage *imageData)
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
	void  VLocalResManager::LocalImage::Destroy (VBarrierManager &barrierMngr)
	{
		ASSERT( _isMutable );

		if ( _currentState.layout != _imageData->DefaultLayout() or _currentState.unavailable != Zero )
		{
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
		
			barrierMngr.AddImageBarrier( _currentState.readStages | _currentState.writeStages, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, barrier );
		}
		_imageData = null;
	}

/*
=================================================
	CommitBarrier
=================================================
*/
	void  VLocalResManager::LocalImage::CommitBarrier (VBarrierManager &barrierMngr) const
	{
		ASSERT( _isMutable );
		ASSERT( _currentState.cmdIndex < _pendingAccess.cmdIndex );

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

			barrierMngr.AddImageBarrier( stages, _pendingAccess.stages, barrier );

			_currentState.writeStages	= _pendingAccess.stages;
			_currentState.writeAccess	= _pendingAccess.access;
			_currentState.readStages	= Zero;
			_currentState.readAccess	= Zero;
			_currentState.unavailable	= VReadAccessMask;	// all subsequent reads must invalidate cache before reading
			_currentState.cmdIndex		= _pendingAccess.cmdIndex;
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

			barrierMngr.AddImageBarrier( stages, _pendingAccess.stages, barrier );

			_currentState.unavailable	&= ~_pendingAccess.access;
			_currentState.readStages	 = _pendingAccess.stages;
			_currentState.readAccess	 = _pendingAccess.access;
			_currentState.cmdIndex		 = _pendingAccess.cmdIndex;
			_currentState.layout		 = _pendingAccess.layout;
		}
		else
		// read after write
		if ( AnyBits( _currentState.unavailable, _pendingAccess.access ))
		{
			barrier.srcAccessMask	= _currentState.writeAccess;	// read access has no effect
			barrier.dstAccessMask	= _pendingAccess.access;
			
			barrierMngr.AddImageBarrier( _currentState.writeStages, _pendingAccess.stages, barrier );

			_currentState.unavailable	&= ~_pendingAccess.access;
			_currentState.readStages	|= _pendingAccess.stages;
			_currentState.readAccess	|= _pendingAccess.access;
			_currentState.cmdIndex		 = _pendingAccess.cmdIndex;
		}
		// parallel reading
		else
		{
			_currentState.readStages	|= _pendingAccess.stages;
			_currentState.readAccess	|= _pendingAccess.access;
			_currentState.cmdIndex		 = _pendingAccess.cmdIndex;
		}

		_pendingAccess = Default;
	}
	
/*
=================================================
	SetInitialState
=================================================
*/
	void  VLocalResManager::LocalImage::SetInitialState (EResourceState state) const
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

	void  VLocalResManager::LocalImage::SetInitialState (EResourceState state, EImageLayout layout) const
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
//-----------------------------------------------------------------------------
	
	
	
/*
=================================================
	destructor
=================================================
*/
	VLocalResManager::LocalBuffer::~LocalBuffer ()
	{
		ASSERT( _bufferData == null );
	}

/*
=================================================
	Create
=================================================
*/
	bool  VLocalResManager::LocalBuffer::Create (const VBuffer *bufferData)
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
	void  VLocalResManager::LocalBuffer::Destroy (VBarrierManager &barrierMngr)
	{
		ASSERT( _isMutable );

		if ( _currentState.unavailable != Zero )
		{
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
		_bufferData = null;
	}
	
/*
=================================================
	GetMemoryInfo
=================================================
*/
	bool  VLocalResManager::LocalBuffer::GetMemoryInfo (OUT VulkanMemoryObjInfo &info) const
	{
		return _bufferData->GetMemoryInfo( OUT info );
	}
	
/*
=================================================
	CommitBarrier
=================================================
*/
	void  VLocalResManager::LocalBuffer::CommitBarrier (VBarrierManager &barrierMngr) const
	{
		ASSERT( _isMutable );
		ASSERT( _currentState.cmdIndex < _pendingAccess.cmdIndex );

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
			_currentState.cmdIndex		= _pendingAccess.cmdIndex;
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
			_currentState.cmdIndex		 = _pendingAccess.cmdIndex;
		}
		// parallel reading
		else
		{
			_currentState.readStages	|= _pendingAccess.stages;
			_currentState.readAccess	|= _pendingAccess.access;
			_currentState.cmdIndex		 = _pendingAccess.cmdIndex;
		}

		_pendingAccess = Default;
	}

/*
=================================================
	SetInitialState
=================================================
*/
	void  VLocalResManager::LocalBuffer::SetInitialState (EResourceState state) const
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
//-----------------------------------------------------------------------------
	


/*
=================================================
	constructor
=================================================
*/
	VLocalResManager::VLocalResManager (const VCommandBatch& batch) :
		VBarrierManager{ batch },
		_resMngr{ RenderGraph().GetResourceManager() }
	{
	}
	
/*
=================================================
	destructor
=================================================
*/
	VLocalResManager::~VLocalResManager ()
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
	void  VLocalResManager::FlushBarriers ()
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
	inline void  VLocalResManager::_FlushLocalResourceStates (ResPool &localRes)
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
	void  VLocalResManager::TransitToDefaultState ()
	{
		_FlushLocalResourceStates<BufferID>( _localRes.buffers );
		_FlushLocalResourceStates<ImageID>( _localRes.images );
	}
	
/*
=================================================
	AddBuffer
=================================================
*/
	void  VLocalResManager::_AddBuffer (const LocalBuffer *buf, EResourceState state)
	{
		if ( not buf->IsMutable() )
			return;

		buf->AddPendingState( state, _cmdCounter );

		_pendingBarriers.buffers.insert( buf );
	}

	void  VLocalResManager::AddBuffer (const LocalBuffer *buf, EResourceState state, Bytes offset, Bytes size)
	{
		_AddBuffer( buf, state );
		Unused( offset, size );
	}

	void  VLocalResManager::AddBuffer (const LocalBuffer *buf, EResourceState state, ArrayView<BufferImageCopy> copy, const LocalImage* img)
	{
		_AddBuffer( buf, state );
		Unused( copy, img );
	}
	
/*
=================================================
	AddImage
=================================================
*/
	void  VLocalResManager::_AddImage (const LocalImage *img, EResourceState state, VkImageLayout layout)
	{
		if ( not img->IsMutable() )
			return;
		
		img->AddPendingState( state, layout, _cmdCounter );
		
		_pendingBarriers.images.insert( img );
	}

	void  VLocalResManager::AddImage (const LocalImage *img, EResourceState state, VkImageLayout layout, StructView<ImageSubresourceLayers> subresLayers)
	{
		_AddImage( img, state, layout );
		Unused( subresLayers );
	}
	
	void  VLocalResManager::AddImage (const LocalImage *img, EResourceState state, VkImageLayout layout, StructView<ImageSubresourceRange> subres)
	{
		_AddImage( img, state, layout );
		Unused( subres );
	}
//-----------------------------------------------------------------------------


	
	//
	// Descriptor Set Barriers
	//
	class VLocalResManager::DescriptorSetBarriers
	{
	// types
	private:
		using EDescriptorType = DescriptorSet::EDescriptorType;

	// variables
	private:
		VLocalResManager &	_resMngr;
		ArrayView<uint>		_dynamicOffsets;

	// methods
	public:
		DescriptorSetBarriers (VLocalResManager &rm, ArrayView<uint> offsets) : _resMngr{rm}, _dynamicOffsets{offsets} {}

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
	void  VLocalResManager::DescriptorSetBarriers::operator () (const UniformName &, EDescriptorType, const DescriptorSet::Buffer &buf)
	{
		for (uint i = 0; i < buf.elementCount; ++i)
		{
			auto&				elem	= buf.elements[i];
			LocalBuffer const*	buffer	= _resMngr.Get( elem.bufferId );

			if ( buffer )
			{
				// validation
				{
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
				}

				_resMngr._AddBuffer( buffer, buf.state );
			}
		}
	}

/*
=================================================
	operator (TexelBuffer)
=================================================
*/
	void  VLocalResManager::DescriptorSetBarriers::operator () (const UniformName &, EDescriptorType, const DescriptorSet::TexelBuffer &texbuf)
	{
		for (uint i = 0; i < texbuf.elementCount; ++i)
		{
			auto&				elem	= texbuf.elements[i];
			LocalBuffer const*	buffer	= _resMngr.Get( elem.bufferId );

			if ( buffer )
				_resMngr._AddBuffer( buffer, texbuf.state );
		}
	}

/*
=================================================
	operator (Image / Texture / SubpassInput)
=================================================
*/
	void  VLocalResManager::DescriptorSetBarriers::operator () (const UniformName &, EDescriptorType, const DescriptorSet::Image &img)
	{
		for (uint i = 0; i < img.elementCount; ++i)
		{
			auto&				elem	= img.elements[i];
			LocalImage const*	image	= _resMngr.Get( elem.imageId );

			if ( image )
				_resMngr._AddImage( image, img.state, EResourceState_ToImageLayout( img.state, image->AspectMask() ));
		}
	}
	
	void  VLocalResManager::DescriptorSetBarriers::operator () (const UniformName &, EDescriptorType, const DescriptorSet::Texture &tex)
	{
		for (uint i = 0; i < tex.elementCount; ++i)
		{
			auto&				elem	= tex.elements[i];
			LocalImage const*	image	= _resMngr.Get( elem.imageId );

			if ( image )
				_resMngr._AddImage( image, tex.state, EResourceState_ToImageLayout( tex.state, image->AspectMask() ));
		}
	}

/*
=================================================
	DescriptorsBarrier
=================================================
*/
	void  VLocalResManager::DescriptorsBarrier (DescriptorSetID ds, ArrayView<uint> dynamicOffsets)
	{
		auto*	desc_set = Get( ds );
		CHECK_ERRV( desc_set );
		
		DescriptorSetBarriers	visitor{ *this, dynamicOffsets };
		desc_set->ForEachUniform( visitor );
	}


}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
