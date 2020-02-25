// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

namespace AE::Graphics
{

	//
	// Vulkan Command Batch
	//

	class VCommandBatch
	{
	// types
	public:
		struct OnBufferDataLoadedEvent
		{
			using Callback_t = Function< void (const BufferView &) >;
			
			Callback_t		callback;
			BufferView		view;

			OnBufferDataLoadedEvent () {}
			OnBufferDataLoadedEvent (Callback_t&& cb, BufferView&& view) : callback{std::move(cb)}, view{std::move(view)} {}
		};

		
		struct OnImageDataLoadedEvent
		{
			using Callback_t	= Function< void (const ImageView &) >;

			Callback_t		callback;
			ImageView		view;

			OnImageDataLoadedEvent () {}
			OnImageDataLoadedEvent (Callback_t&& cb, ImageView&& view) : callback{std::move(cb)}, view{std::move(view)} {}
		};
		//---------------------------------------------------------------------


	private:
		struct StagingBuffer
		{
			GfxResourceID		bufferId;
			BytesU				capacity;
			BytesU				size;
			StagingBufferIdx	index;
			
			void *				mappedPtr	= null;
			BytesU				memOffset;					// can be used to flush memory ranges
			VkDeviceMemory		mem			= VK_NULL_HANDLE;
			bool				isCoherent	= false;

			ND_ bool	IsFull ()	const	{ return size >= capacity; }
			ND_ bool	Empty ()	const	{ return size == 0_b; }
		};
		using StagingBuffers_t	= FixedArray< StagingBuffer, 8 >;


		using Semaphores_t		= FixedArray< VkSemaphore, 16 >;
		using Fences_t			= FixedArray< VkFence, 8 >;
		using CmdBuffers_t		= FixedArray< UniqueID<BakedCommandBufferID>, 16 >;


	// variables
	private:
		VkFence				_fence		= VK_NULL_HANDLE;

		Atomic<uint16_t>	_generation	{0};

		Semaphores_t		_semaphores;
		Fences_t			_fences;
		CmdBuffers_t		_cmdBuffers;
		
		// staging buffers
		struct {
			StagingBuffers_t					hostToDevice;	// CPU write, GPU read
			StagingBuffers_t					deviceToHost;	// GPU write, CPU read
			Array< OnBufferDataLoadedEvent >	onBufferLoadedEvents;
			Array< OnImageDataLoadedEvent >		onImageLoadedEvents;
		}					_staging;

		VResourceManager&	_resMngr;

		bool				_isComplete	= false;
		
		RWDataRaceCheck		_drCheck;


	// methods
	public:
		VCommandBatch (VResourceManager &rm) : _resMngr{rm} {}
		~VCommandBatch () {}

		void  Initialize ();
		bool  OnComplete ();

		void  AddSemaphore (VkSemaphore sem);
		void  AddFence (VkFence fence);
		void  AddCmdBuffer (UniqueID<BakedCommandBufferID> &&id);

		// staging buffer
		bool  GetWritable (const BytesU srcRequiredSize, const BytesU blockAlign, const BytesU offsetAlign, const BytesU dstMinSize,
						   OUT GfxResourceID &dstBuffer, OUT BytesU &dstOffset, OUT BufferView::Data &outDataRange);
		bool  GetReadable (const BytesU srcRequiredSize, const BytesU blockAlign, const BytesU offsetAlign, const BytesU dstMinSize,
						   OUT GfxResourceID &dstBuffer, OUT BytesU &dstOffset, OUT BufferView::Data &outDataRange);
		bool  AddDataLoadedEvent (OnImageDataLoadedEvent &&);
		bool  AddDataLoadedEvent (OnBufferDataLoadedEvent &&);

		ND_ bool	IsComplete ()	const	{ SHAREDLOCK( _drCheck );  return _isComplete; }
		ND_ uint	Generation ()	const	{ SHAREDLOCK( _drCheck );  return _generation.load( EMemoryOrder::Relaxed ); }

	private:
		bool  _ReleaseFencesAndSemaphores ();
		void  _ProcessReadOps ();
		
		bool  _GetStaging (StagingBuffers_t &stagingBuffers, EBufferUsage usage,
						   const BytesU srcRequiredSize, const BytesU blockAlign, const BytesU offsetAlign, const BytesU dstMinSize,
						   OUT GfxResourceID &dstBuffer, OUT BytesU &dstOffset, OUT BufferView::Data &outDataRange);
	};

	

/*
=================================================
	Initialize
=================================================
*/
	void  VCommandBatch::Initialize ()
	{
		_isComplete = false;
	}

/*
=================================================
	OnComplete
=================================================
*/
	bool  VCommandBatch::OnComplete ()
	{
		EXLOCK( _drCheck );

		if ( _isComplete )
			return true;

		CHECK_ERR( _ReleaseFencesAndSemaphores() );

		_ProcessReadOps();

		for (auto& cb : _cmdBuffers) {
			_resMngr.ReleaseResource( cb );
		}
		_cmdBuffers.clear();

		_isComplete = true;
		
		_generation.fetch_add( 1, EMemoryOrder::Relaxed );
		return true;
	}
	
/*
=================================================
	_ReleaseFencesAndSemaphores
=================================================
*/
	bool  VCommandBatch::_ReleaseFencesAndSemaphores ()
	{
		auto&	dev = _resMngr.GetDevice();

		CHECK( _fences.size() );
		VK_CHECK( dev.vkWaitForFences( dev.GetVkDevice(), uint(_fences.size()), _fences.data(), true, UMax ));

		_resMngr.ReleaseFences( _fences );
		_resMngr.ReleaseSemaphores( _semaphores );

		_fences.clear();
		_semaphores.clear();

		return true;
	}
	
/*
=================================================
	_ProcessReadOps
=================================================
*/
	void  VCommandBatch::_ProcessReadOps ()
	{
		// invalidate non-cocherent memory before reading
		FixedArray<VkMappedMemoryRange, 32>		regions;
		VDevice const&							dev = _resMngr.GetDevice();

		for (auto& buf : _staging.deviceToHost)
		{
			if ( buf.isCoherent )
				continue;

			if ( regions.size() == regions.capacity() )
			{
				VK_CALL( dev.vkInvalidateMappedMemoryRanges( dev.GetVkDevice(), uint(regions.size()), regions.data() ));
				regions.clear();
			}

			auto&	reg = regions.emplace_back();
			reg.sType	= VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			reg.pNext	= null;
			reg.memory	= buf.mem;
			reg.offset	= VkDeviceSize(buf.memOffset);
			reg.size	= VkDeviceSize(buf.size);
		}

		if ( regions.size() )
			VK_CALL( dev.vkInvalidateMappedMemoryRanges( dev.GetVkDevice(), uint(regions.size()), regions.data() ));

		
		// trigger buffer events
		for (auto& ev : _staging.onBufferLoadedEvents)
		{
			ev.callback( ev.view );
		}
		_staging.onBufferLoadedEvents.clear();
		
		
		// trigger image events
		for (auto& ev : _staging.onImageLoadedEvents)
		{
			ev.callback( ev.view );
		}
		_staging.onImageLoadedEvents.clear();
		

		// release resources
		{
			for (auto& sb : _staging.hostToDevice) {
				_resMngr.ReleaseStagingBuffer( sb.index );
			}
			_staging.hostToDevice.clear();

			for (auto& sb : _staging.deviceToHost) {
				_resMngr.ReleaseStagingBuffer( sb.index );
			}
			_staging.deviceToHost.clear();
		}
	}

/*
=================================================
	AddSemaphore
=================================================
*/
	void  VCommandBatch::AddSemaphore (VkSemaphore sem)
	{
		EXLOCK( _drCheck );
		_semaphores.try_push_back( sem );	// TODO: check for overflow
	}
	
/*
=================================================
	AddFence
=================================================
*/
	void  VCommandBatch::AddFence (VkFence fence)
	{
		EXLOCK( _drCheck );
		_fences.try_push_back( fence );		// TODO: check for overflow
	}
	
/*
=================================================
	AddCmdBuffer
=================================================
*/
	void  VCommandBatch::AddCmdBuffer (UniqueID<BakedCommandBufferID>&& id)
	{
		EXLOCK( _drCheck );
		_cmdBuffers.try_push_back( std::move(id) );
	}
	
/*
=================================================
	AddDataLoadedEvent
=================================================
*/
	bool  VCommandBatch::AddDataLoadedEvent (OnImageDataLoadedEvent &&ev)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( ev.callback and not ev.view.Parts().empty() );

		_staging.onImageLoadedEvents.push_back( std::move(ev) );
		return true;
	}
	
	bool  VCommandBatch::AddDataLoadedEvent (OnBufferDataLoadedEvent &&ev)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( ev.callback and not ev.view.empty() );

		_staging.onBufferLoadedEvents.push_back( std::move(ev) );
		return true;
	}

/*
=================================================
	GetWritable
=================================================
*/
	bool  VCommandBatch::GetWritable (const BytesU srcRequiredSize, const BytesU blockAlign, const BytesU offsetAlign, const BytesU dstMinSize,
									  OUT GfxResourceID &dstBuffer, OUT BytesU &dstOffset, OUT BufferView::Data &outDataRange)
	{
		return _GetStaging( _staging.hostToDevice, EBufferUsage::TransferSrc,
						    srcRequiredSize, blockAlign, offsetAlign, dstMinSize,
						    OUT dstBuffer, OUT dstOffset, OUT outDataRange );
	}
	
/*
=================================================
	GetReadable
=================================================
*/
	bool  VCommandBatch::GetReadable (const BytesU srcRequiredSize, const BytesU blockAlign, const BytesU offsetAlign, const BytesU dstMinSize,
									  OUT GfxResourceID &dstBuffer, OUT BytesU &dstOffset, OUT BufferView::Data &outDataRange)
	{
		return _GetStaging( _staging.deviceToHost, EBufferUsage::TransferDst,
						    srcRequiredSize, blockAlign, offsetAlign, dstMinSize,
						    OUT dstBuffer, OUT dstOffset, OUT outDataRange );
	}

/*
=================================================
	_GetStaging
=================================================
*/
	bool  VCommandBatch::_GetStaging (StagingBuffers_t &stagingBuffers, EBufferUsage usage,
									  const BytesU srcRequiredSize, const BytesU blockAlign, const BytesU offsetAlign, const BytesU dstMinSize,
									  OUT GfxResourceID &dstBuffer, OUT BytesU &dstOffset, OUT BufferView::Data &outDataRange)
	{
		EXLOCK( _drCheck );
		ASSERT( blockAlign > 0_b and offsetAlign > 0_b );
		ASSERT( dstMinSize == AlignToSmaller( dstMinSize, blockAlign ));

		// search in existing
		StagingBuffer*	suitable		= null;
		StagingBuffer*	max_available	= null;
		BytesU			max_size;

		for (auto& buf : stagingBuffers)
		{
			const BytesU	off	= AlignToLarger( buf.size, offsetAlign );
			const BytesU	av	= AlignToSmaller( buf.capacity - off, blockAlign );

			if ( av >= srcRequiredSize )
			{
				suitable = &buf;
				break;
			}

			if ( not max_available or av > max_size )
			{
				max_available	= &buf;
				max_size		= av;
			}
		}

		// no suitable space, try to use max available block
		if ( not suitable and max_available and max_size >= dstMinSize )
		{
			suitable = max_available;
		}

		// allocate new buffer
		if ( not suitable )
		{
			CHECK_ERR( stagingBuffers.size() < stagingBuffers.capacity() );

			GfxResourceID		buf_id;
			StagingBufferIdx	buf_idx;
			CHECK_ERR( _resMngr.CreateStagingBuffer( usage, OUT buf_id, OUT buf_idx ));

			auto*	buffer = _resMngr.GetResource( VBufferID{ buf_id.Index(), buf_id.Generation() });
			CHECK_ERR( buffer );

			VResourceMemoryInfo		mem_info;
			CHECK_ERR( buffer->GetMemoryInfo( OUT mem_info ));

			suitable = &stagingBuffers.emplace_back();
			suitable->bufferId	= buf_id;
			suitable->index		= buf_idx;
			suitable->mappedPtr	= mem_info.mappedPtr;
			suitable->mem		= mem_info.memory;
			suitable->capacity	= mem_info.size;
			suitable->memOffset	= mem_info.offset;
			suitable->isCoherent= EnumEq( mem_info.flags, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );
		}

		// write data to buffer
		dstOffset			= AlignToLarger( suitable->size, offsetAlign );
		outDataRange.size	= Min( AlignToSmaller( suitable->capacity - dstOffset, blockAlign ), srcRequiredSize );
		dstBuffer			= suitable->bufferId;
		outDataRange.ptr	= suitable->mappedPtr + dstOffset;

		suitable->size = dstOffset + outDataRange.size;
		return true;
	}


}	// AE::Graphics
