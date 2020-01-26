// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

namespace AE::Graphics
{

	//
	// Graphics Context
	//

	class VRenderGraph::GraphicsContext final : public VulkanDeviceFn, public IGraphicsContext
	{
		friend class RenderContext;

	// types
	private:
		using Index_t		= VResourceManager::Index_t;
		using Generation_t	= VResourceManager::Generation_t;

		template <typename T, size_t CS, size_t MC>
		using PoolTmpl		= Threading::IndexedPool< VResourceBase<T>, Index_t, CS, MC >;
		
		template <typename Res, typename MainPool, size_t MC>
		struct LocalResPool
		{
			PoolTmpl< Res, MainPool::capacity()/MC, MC >	pool;
			HashMap< Index_t, Index_t >						toLocal;
		};

		using LocalImages_t		= LocalResPool< VLocalImage,	VResourceManager::ImagePool_t,		16 >;
		using LocalBuffers_t	= LocalResPool< VLocalBuffer,	VResourceManager::BufferPool_t,		16 >;
		
		using BufferBarriers_t	= HashSet< VLocalBuffer const *>;
		using ImageBarriers_t	= HashSet< VLocalImage const *>;


	// variables
	private:
		VkCommandBuffer				_cmdbuf					= VK_NULL_HANDLE;
		uint						_cmdCounter				= 0;
		bool						_enableBarriers			= true;

		VDevice const&				_device;
		VResourceManager &			_resMngr;
		LinearAllocator<>			_allocator;
		
		VBarrierManager				_barrierMngr;
		struct {
			BufferBarriers_t			buffers;
			ImageBarriers_t				images;
		}							_pendingBarriers;

		VQueuePtr					_queue;
		VCommandPool				_cmdPool;
		Ptr<VCommandBatch>			_cmdBatch;

		VkPipelineStageFlagBits		_allSrcStages			= VkPipelineStageFlagBits(0);
		VkPipelineStageFlagBits		_allDstStages			= VkPipelineStageFlagBits(0);

		struct {
			VkPipelineLayout			computeLayout		= VK_NULL_HANDLE;
			VkPipeline					computePipeline		= VK_NULL_HANDLE;
		}							_states;

		struct {
			LocalBuffers_t				buffers;
			LocalImages_t				images;
		}							_localRes;


	// methods
	public:
		explicit GraphicsContext (VResourceManager &resMngr, VQueuePtr queue);
		~GraphicsContext ();

		ND_ VkCommandBuffer			GetCommandBuffer ()		const	{ return _cmdbuf; }
		ND_ VQueuePtr				GetQueue ()				const	{ return _queue; }
		ND_ VCommandBatch*			GetBatch ()				const	{ return _cmdBatch.get(); }
		ND_ VDevice const&			GetDevice ()			const	{ return _device; }
		ND_ VResourceManager const&	GetResourceManager ()	const	{ return _resMngr; }

		template <typename ID>
		ND_ auto*				AcquireResource (ID id);
		
		ND_ VLocalBuffer const*	ToLocalBuffer (GfxResourceID id);
		ND_ VLocalImage  const*	ToLocalImage (GfxResourceID id);

		bool  Create ();
		bool  Begin (VCommandBatch *);
		bool  Submit ();


	// ITransferContext
		NativeContext_t  GetNativeContext () override;
		
		GfxResourceID   GetOutput (GfxResourceID id) override;
		void SetOutput (GfxResourceID id, GfxResourceID res) override;
		
		void ClearColorImage (GfxResourceID image, const ClearColor_t &color, ArrayView<ImageSubresourceRange> ranges) override;
		void ClearDepthStencilImage (GfxResourceID image, const DepthStencil &depthStencil, ArrayView<ImageSubresourceRange> ranges) override;
		void FillBuffer (GfxResourceID buffer, BytesU offset, BytesU size, uint data) override;

		void UpdateBuffer (GfxResourceID buffer, BytesU offset, ArrayView<uint> data) override;
		
		bool UpdateHostBuffer (GfxResourceID buffer, BytesU offset, ArrayView<uint> data) override;
		bool MapHostBuffer (GfxResourceID buffer, BytesU offset, INOUT BytesU &size, OUT void* &mapped) override;
		
		bool ReadBuffer (GfxResourceID buffer, BytesU offset, BytesU size, const Function<void (BufferView)> &fn) override;
		bool ReadImage (GfxResourceID image, const Function<void (ImageView)> &fn) override;

		void CopyBuffer (GfxResourceID srcBuffer, GfxResourceID dstBuffer, ArrayView<BufferCopy> ranges) override;
		void CopyImage (GfxResourceID srcImage, GfxResourceID dstImage, ArrayView<ImageCopy> ranges) override;
		void CopyBufferToImage (GfxResourceID srcBuffer, GfxResourceID dstImage, ArrayView<BufferImageCopy> ranges) override;
		void CopyImageToBuffer (GfxResourceID srcImage, GfxResourceID dstBuffer, ArrayView<BufferImageCopy> ranges) override;
		
		void Present (GfxResourceID image, MipmapLevel level, ImageLayer layer) override;


	// IComputeContext
		void BindPipeline (ComputePipelineID ppln) override;
		void BindDescriptorSet (uint index, DescriptorSetID ds, ArrayView<uint> dynamicOffsets) override;
		void PushConstant (BytesU offset, BytesU size, const void *values, EShaderStages stages) override;

		void Dispatch (const uint3 &groupCount) override;
		void DispatchIndirect (GfxResourceID buffer, BytesU offset) override;
		void DispatchBase (const uint3 &baseGroup, const uint3 &groupCount) override;
		

	// IGraphicsContext
		void BlitImage (GfxResourceID srcImage, GfxResourceID dstImage, EBlitFilter filter, ArrayView<ImageBlit> regions) override;
		void ResolveImage (GfxResourceID srcImage, GfxResourceID dstImage, ArrayView<ImageResolve> regions) override;


	private:
		template <typename ID, typename Res, typename MainPool, size_t MC>
		ND_ Res const*  _ToLocal (ID id, INOUT LocalResPool<Res,MainPool,MC> &, StringView msg);
		
		void _AddImage (const VLocalImage *img, EResourceState state, VkImageLayout layout, StructView<VkImageSubresourceLayers> subresLayers);
		void _AddImage (const VLocalImage *img, EResourceState state, VkImageLayout layout, StructView<VkImageSubresourceRange> subres);
		void _AddBuffer (const VLocalBuffer *buf, EResourceState state, VkDeviceSize offset, VkDeviceSize size);
		void _AddBuffer (const VLocalBuffer *buf, EResourceState state, ArrayView<VkBufferImageCopy> copy);
		void _CheckBufferAccess (const VLocalBuffer *buf, EResourceState state, VkDeviceSize offset, VkDeviceSize size);
		void _CheckBufferAccess (const VLocalBuffer *buf, EResourceState state, ArrayView<VkBufferImageCopy> copy);
		void _CommitBarriers ();

		void _FlushLocalResourceStates ();

		template <typename ID, typename ResPool>
		void _FlushLocalResourceStates (ResPool &);
	};


	
namespace {
/*
=================================================
	ConvertImageSubresourceRange
=================================================
*/
	inline void  ConvertImageSubresourceRange (OUT VkImageSubresourceRange* dstRanges, const IGraphicsContext::ImageSubresourceRange* srcRanges,
												const size_t count, const ImageDesc &desc)
	{
		for (size_t i = 0; i < count; ++i)
		{
			auto&	src = srcRanges[i];
			auto&	dst = dstRanges[i];

			ASSERT( src.baseMipLevel < desc.maxLevel );
			ASSERT( src.baseArrayLayer < desc.arrayLayers );

			dst.aspectMask		= VEnumCast( src.aspectMask );
			dst.baseMipLevel	= Min( src.baseMipLevel.Get(), desc.maxLevel.Get()-1 );
			dst.levelCount		= Min( src.levelCount, desc.maxLevel.Get() - src.baseMipLevel.Get() );
			dst.baseArrayLayer	= Min( src.baseArrayLayer.Get(), desc.arrayLayers.Get()-1 );
			dst.layerCount		= Min( src.layerCount, desc.arrayLayers.Get() - src.baseArrayLayer.Get() );
		}
	}

/*
=================================================
	ConvertImageSubresourceLayers
=================================================
*/
	inline void  ConvertImageSubresourceLayers (OUT VkImageSubresourceLayers &dst, const IGraphicsContext::ImageSubresourceLayers &src, const ImageDesc &desc)
	{
		ASSERT( src.mipLevel < desc.maxLevel );
		ASSERT( src.baseArrayLayer < desc.arrayLayers );

		dst.aspectMask		= VEnumCast( src.aspectMask );
		dst.mipLevel		= Min( src.mipLevel.Get(), desc.maxLevel.Get()-1 );
		dst.baseArrayLayer	= Min( src.baseArrayLayer.Get(), desc.arrayLayers.Get()-1 );
		dst.layerCount		= Min( src.layerCount, desc.arrayLayers.Get() - src.baseArrayLayer.Get() );
	}
	
/*
=================================================
	ConvertBufferImageCopy
=================================================
*/
	inline void  ConvertBufferImageCopy (OUT VkBufferImageCopy* dstRanges, const IGraphicsContext::BufferImageCopy* srcRanges,
										 const size_t count, const ImageDesc &desc)
	{
		for (size_t i = 0; i < count; ++i)
		{
			auto&	src = srcRanges[i];
			auto&	dst = dstRanges[i];

			ASSERT( All( src.imageOffset < desc.dimension ));
			ASSERT( All( src.imageOffset + src.imageExtent <= desc.dimension ));

			dst.bufferOffset		= VkDeviceSize(src.bufferOffset);
			dst.bufferRowLength		= src.bufferRowLength;
			dst.bufferImageHeight	= src.bufferImageHeight;
			ConvertImageSubresourceLayers( OUT dst.imageSubresource, src.imageSubresource, desc );
			dst.imageOffset			= { int(src.imageOffset.x), int(src.imageOffset.y), int(src.imageOffset.z) };
			dst.imageExtent			= { src.imageExtent.x, src.imageExtent.y, src.imageExtent.z };
		}
	}
}	// namespace
//-----------------------------------------------------------------------------



/*
=================================================
	constructor
=================================================
*/
	VRenderGraph::GraphicsContext::GraphicsContext (VResourceManager &resMngr, VQueuePtr queue) :
		_device{ resMngr.GetDevice() }, _resMngr{ resMngr }, _queue{ queue }
	{
		VulkanDeviceFn_Init( _device );
	}
	
/*
=================================================
	destructor
=================================================
*/
	VRenderGraph::GraphicsContext::~GraphicsContext ()
	{
		ASSERT( not _cmdbuf );
		ASSERT( _cmdCounter == 0 );

		_cmdPool.Destroy( _device );
	}

/*
=================================================
	Create
=================================================
*/
	bool  VRenderGraph::GraphicsContext::Create ()
	{
		_allocator.SetBlockSize( 4_Kb );

		CHECK_ERR( _cmdPool.Create( _device, _queue, 0, "" ));

		// set source pipeline stages
		{
			VkPipelineStageFlagBits	rt_flags =
				#ifdef VK_NV_ray_tracing
					_device.IsRayTracingEnabled() ? VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_NV | VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV : VkPipelineStageFlagBits(0);
				#else
					VkPipelineStageFlagBits(0);
				#endif

			BEGIN_ENUM_CHECKS();
			switch ( _queue->type )
			{
				case EQueueType::Graphics :
					_allSrcStages =	VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT		|
									VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT	|
									VK_PIPELINE_STAGE_TRANSFER_BIT			|
									rt_flags;
					_allDstStages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT	|
									rt_flags;
					break;

				case EQueueType::AsyncCompute :
					_allSrcStages =	VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT |
									rt_flags;
					_allDstStages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT	|
									rt_flags;
					break;

				case EQueueType::AsyncTransfer :
					_allSrcStages = VK_PIPELINE_STAGE_TRANSFER_BIT;
					_allDstStages = VK_PIPELINE_STAGE_TRANSFER_BIT;
					break;

				case EQueueType::_Count :
				case EQueueType::Unknown :	break;	// to shutup compiler warnings
			}
			END_ENUM_CHECKS();
		}
		return true;
	}

/*
=================================================
	Begin
=================================================
*/
	bool  VRenderGraph::GraphicsContext::Begin (VCommandBatch *batch)
	{
		CHECK_ERR( not _cmdbuf );
		CHECK_ERR( not _cmdBatch );

		_cmdBatch = batch;

		_cmdbuf = _cmdPool.AllocPrimary( _device );
		CHECK_ERR( _cmdbuf );
		
		VkCommandBufferBeginInfo	begin_info = {};
		begin_info.sType	= VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.flags	= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		VK_CHECK( vkBeginCommandBuffer( _cmdbuf, &begin_info ));

		// invalidate staging buffer cache
		// TODO: optimize ?
		{
			VkMemoryBarrier	barrier = {};
			barrier.sType			= VK_STRUCTURE_TYPE_MEMORY_BARRIER;
			barrier.srcAccessMask	= VK_ACCESS_HOST_WRITE_BIT;
			barrier.dstAccessMask	= VK_ACCESS_INDEX_READ_BIT | VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT;

			vkCmdPipelineBarrier( _cmdbuf, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 1, &barrier, 0, null, 0, null );
		}
		return true;
	}

/*
=================================================
	Submit
=================================================
*/
	bool  VRenderGraph::GraphicsContext::Submit ()
	{
		if ( not _cmdbuf )
			return true;

		if ( _cmdCounter == 0 )
		{
			VK_CHECK( vkEndCommandBuffer( _cmdbuf ));
			_cmdPool.RecyclePrimary( _device, _cmdbuf );
			_cmdbuf		= VK_NULL_HANDLE;
			_cmdBatch	= null;
			return true;
		}

		_FlushLocalResourceStates();

		VK_CHECK( vkEndCommandBuffer( _cmdbuf ));

		_barrierMngr.FlushMemoryRanges( _device );

		VkFence	fence = _resMngr.CreateFence();

		// submit commands
		{
			EXLOCK( _queue->guard );

			VkSubmitInfo			submit_info = {};
			submit_info.sType					= VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submit_info.commandBufferCount		= 1;
			submit_info.pCommandBuffers			= &_cmdbuf;
			submit_info.waitSemaphoreCount		= 0;
			submit_info.pWaitSemaphores			= null;
			submit_info.pWaitDstStageMask		= null;
			submit_info.signalSemaphoreCount	= 0;
			submit_info.pSignalSemaphores		= null;

			VK_CHECK( vkQueueSubmit( _queue->handle, 1, &submit_info, fence ));
		}

		// TODO: remove
		//VK_CHECK( vkQueueWaitIdle( _queue->handle ));
		//_cmdPool.RecyclePrimary( _device, _cmdbuf );

		_cmdBatch->AddFence( fence );
		_cmdBatch->AddCmdBuffer( &_cmdPool, _cmdbuf );

		_cmdBatch	= null;
		_cmdbuf		= VK_NULL_HANDLE;
		_cmdCounter	= 0;
		return true;
	}
	
/*
=================================================
	_FlushLocalResourceStates
=================================================
*/
	void  VRenderGraph::GraphicsContext::_FlushLocalResourceStates ()
	{
		_FlushLocalResourceStates<VBufferID>( _localRes.buffers );
		_FlushLocalResourceStates<VImageID>( _localRes.images );

		_barrierMngr.ForceCommit( _device, _cmdbuf, _allSrcStages, _allDstStages );
		
		// flush staging buffer cache
		// TODO: optimize
		{
			VkMemoryBarrier	barrier = {};
			barrier.sType			= VK_STRUCTURE_TYPE_MEMORY_BARRIER;
			barrier.srcAccessMask	= VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask	= VK_ACCESS_HOST_READ_BIT;

			vkCmdPipelineBarrier( _cmdbuf, _allSrcStages, VK_PIPELINE_STAGE_HOST_BIT, 0, 1, &barrier, 0, null, 0, null );
		}
	}
	
	template <typename ID, typename ResPool>
	void  VRenderGraph::GraphicsContext::_FlushLocalResourceStates (ResPool &localRes)
	{
		for (auto&[global_idx, local_idx] : localRes.toLocal)
		{
			auto&	res = localRes.pool[ local_idx ].Data();

			res.ResetState( _barrierMngr );
			localRes.pool.Unassign( local_idx );
		}

		localRes.toLocal.clear();
	}

/*
=================================================
	_ToLocal
=================================================
*/
	template <typename ID, typename Res, typename MainPool, size_t MC>
	inline Res const*  VRenderGraph::GraphicsContext::_ToLocal (ID id, INOUT LocalResPool<Res,MainPool,MC> &localRes, StringView msg)
	{
		auto[iter, inserted] = localRes.toLocal.insert({ id.Index(), Index_t(UMax) });
		Index_t&	local	 = iter->second;

		// if already exists
		if ( not inserted )
		{
			Res const*  result = &(localRes.pool[ local ].Data());
			ASSERT( result->ToGlobal() );
			return result;
		}

		auto*	res  = _resMngr.GetResource( id, true );
		if ( not res )
			return null;

		CHECK_ERR( localRes.pool.Assign( OUT local ));

		auto&	data = localRes.pool[ local ];
		
		data.Data().~Res();
		PlacementNew<Res>( &data.Data() );

		if ( not data.Create( res ))
		{
			localRes.pool.Unassign( local );
			RETURN_ERR( msg );
		}

		_cmdBatch->AddResource( id );

		return &(data.Data());
	}

/*
=================================================
	ToLocalBuffer
=================================================
*/
	VLocalBuffer const*  VRenderGraph::GraphicsContext::ToLocalBuffer (GfxResourceID id)
	{
		CHECK_ERR( id.ResourceType() == GfxResourceID::EType::Buffer );
		return _ToLocal( VBufferID{ id.Index(), id.Generation() }, _localRes.buffers, "failed when creating local buffer" );
	}
	
/*
=================================================
	ToLocalImage
=================================================
*/
	VLocalImage const*  VRenderGraph::GraphicsContext::ToLocalImage (GfxResourceID id)
	{
		CHECK_ERR( id.ResourceType() == GfxResourceID::EType::Image );
		return _ToLocal( VImageID{ id.Index(), id.Generation() }, _localRes.images, "failed when creating local image" );
	}
	
/*
=================================================
	_AddImage
=================================================
*/
	inline void  VRenderGraph::GraphicsContext::_AddImage (const VLocalImage *img, EResourceState state, VkImageLayout layout, StructView<VkImageSubresourceLayers> subresLayers)
	{
		if ( not img->IsMutable() )
			return;
	}
	
/*
=================================================
	_AddImage
=================================================
*/
	inline void  VRenderGraph::GraphicsContext::_AddImage (const VLocalImage *img, EResourceState state, VkImageLayout layout, StructView<VkImageSubresourceRange> subres)
	{
		if ( not img->IsMutable() )
			return;
	}
	
/*
=================================================
	_AddBuffer
=================================================
*/
	inline void  VRenderGraph::GraphicsContext::_AddBuffer (const VLocalBuffer *buf, EResourceState state, VkDeviceSize offset, VkDeviceSize size)
	{
		if ( not buf->IsMutable() )
			return;

		buf->AddPendingState( state, ExeOrderIndex(_cmdCounter + 1) );
		Unused( offset, size );

		_pendingBarriers.buffers.insert( buf );
	}
	
/*
=================================================
	_AddBuffer
=================================================
*/
	inline void  VRenderGraph::GraphicsContext::_AddBuffer (const VLocalBuffer *buf, EResourceState state, ArrayView<VkBufferImageCopy> ranges)
	{
		if ( not buf->IsMutable() )
			return;

		buf->AddPendingState( state, ExeOrderIndex(_cmdCounter + 1) );
		Unused( ranges );
		
		_pendingBarriers.buffers.insert( buf );
	}
	
/*
=================================================
	_CheckBufferAccess
=================================================
*/
	inline void  VRenderGraph::GraphicsContext::_CheckBufferAccess (const VLocalBuffer *buf, EResourceState state, VkDeviceSize offset, VkDeviceSize size)
	{
		// TODO
	}
	
/*
=================================================
	_CheckBufferAccess
=================================================
*/
	inline void  VRenderGraph::GraphicsContext::_CheckBufferAccess (const VLocalBuffer *buf, EResourceState state, ArrayView<VkBufferImageCopy> ranges)
	{
		// TODO
	}

/*
=================================================
	_CommitBarriers
=================================================
*/
	inline void  VRenderGraph::GraphicsContext::_CommitBarriers ()
	{
		for (auto& buf : _pendingBarriers.buffers) {
			buf->CommitBarrier( _barrierMngr );
		}
		_pendingBarriers.buffers.clear();
		
		for (auto& img : _pendingBarriers.images) {
			img->CommitBarrier( _barrierMngr );
		}
		_pendingBarriers.images.clear();

		_barrierMngr.Commit( _device, _cmdbuf );
	}
	
/*
=================================================
	AcquireResource
=================================================
*/
	template <typename ID>
	inline auto*  VRenderGraph::GraphicsContext::AcquireResource (ID id)
	{
		return _resMngr.GetResource( id, _cmdBatch->AddResource( id ));
	}

/*
=================================================
	GetNativeContext
=================================================
*/
	ITransferContext::NativeContext_t  VRenderGraph::GraphicsContext::GetNativeContext ()
	{
		VulkanContext	vctx;
		vctx.cmdBuffer	= BitCast<CommandBufferVk_t>(_cmdbuf);
		return vctx;
	}
	
/*
=================================================
	GetOutput
=================================================
*/
	GfxResourceID  VRenderGraph::GraphicsContext::GetOutput (GfxResourceID id)
	{
		// TODO
		return id;
	}
	
/*
=================================================
	SetOutput
=================================================
*/
	void  VRenderGraph::GraphicsContext::SetOutput (GfxResourceID id, GfxResourceID res)
	{
		// TODO
	}
	
/*
=================================================
	ClearColorImage
=================================================
*/
	void  VRenderGraph::GraphicsContext::ClearColorImage (GfxResourceID image, const ClearColor_t &color, ArrayView<ImageSubresourceRange> ranges)
	{
		auto*	img = ToLocalImage( image );
		CHECK_ERR( img, void());

		VkClearColorValue			clear_value;
		VkImageSubresourceRange*	vk_ranges	= _allocator.Alloc<VkImageSubresourceRange>( ranges.size() );

		Visit( color,
			   [&clear_value] (const RGBA32f &src) { MemCopy( clear_value.float32, src ); },
			   [&clear_value] (const RGBA32i &src) { MemCopy( clear_value.int32,   src ); },
			   [&clear_value] (const RGBA32u &src) { MemCopy( clear_value.uint32,  src ); });

		ConvertImageSubresourceRange( OUT vk_ranges, ranges.data(), ranges.size(), img->Description() );

		if ( _enableBarriers )
		{
			_AddImage( img, EResourceState::TransferDst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					   StructView<VkImageSubresourceRange>{ vk_ranges, ranges.size() });
		}
		_CommitBarriers();

		vkCmdClearColorImage( _cmdbuf, img->Handle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear_value, uint(ranges.size()), vk_ranges );

		_allocator.Discard();
		++_cmdCounter;
	}
	
/*
=================================================
	ClearDepthStencilImage
=================================================
*/
	void  VRenderGraph::GraphicsContext::ClearDepthStencilImage (GfxResourceID image, const DepthStencil &depthStencil, ArrayView<ImageSubresourceRange> ranges)
	{
		auto*	img = ToLocalImage( image );
		CHECK_ERR( img, void());

		VkClearDepthStencilValue	clear_value;
		VkImageSubresourceRange*	vk_ranges	= _allocator.Alloc<VkImageSubresourceRange>( ranges.size() );

		clear_value.depth	= depthStencil.depth;
		clear_value.stencil	= depthStencil.stencil;
		
		ConvertImageSubresourceRange( OUT vk_ranges, ranges.data(), ranges.size(), img->Description() );
		
		if ( _enableBarriers )
		{
			_AddImage( img, EResourceState::TransferDst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					   StructView<VkImageSubresourceRange>{ vk_ranges, ranges.size() });
		}
		_CommitBarriers();

		vkCmdClearDepthStencilImage( _cmdbuf, img->Handle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear_value, uint(ranges.size()), vk_ranges );

		_allocator.Discard();
		++_cmdCounter;
	}
	
/*
=================================================
	FillBuffer
=================================================
*/
	void  VRenderGraph::GraphicsContext::FillBuffer (GfxResourceID buffer, BytesU offset, BytesU size, uint data)
	{
		auto*	buf = ToLocalBuffer( buffer );
		CHECK_ERR( buf, void());

		ASSERT( offset < buf->Size() );
		ASSERT( size == ~0_b or (offset + size) <= buf->Size() );

		offset	= Min( offset, buf->Size()-1 );
		size	= Min( size, buf->Size() - offset );

		if ( _enableBarriers ) {
			_AddBuffer( buf, EResourceState::TransferDst, VkDeviceSize(offset), VkDeviceSize(size) );
		}
		_CommitBarriers();

		vkCmdFillBuffer( _cmdbuf, buf->Handle(), VkDeviceSize(offset), VkDeviceSize(size), data );
		++_cmdCounter;
	}
	
/*
=================================================
	UpdateBuffer
=================================================
*/
	void  VRenderGraph::GraphicsContext::UpdateBuffer (GfxResourceID buffer, BytesU offset, ArrayView<uint> data)
	{
		BytesU	size = ArraySizeOf(data);
		auto*	buf = ToLocalBuffer( buffer );
		CHECK_ERR( buf, void());

		ASSERT( offset < buf->Size() );
		ASSERT( (offset + size) <= buf->Size() );
		
		offset	= Min( offset, buf->Size()-1 );
		size	= Min( size, buf->Size() - offset );
		
		if ( _enableBarriers ) {
			_AddBuffer( buf, EResourceState::TransferDst, VkDeviceSize(offset), VkDeviceSize(size) );
		}
		_CommitBarriers();

		vkCmdUpdateBuffer( _cmdbuf, buf->Handle(), VkDeviceSize(offset), VkDeviceSize(size), data.data() );
		++_cmdCounter;
	}
	
/*
=================================================
	UpdateHostBuffer
=================================================
*/
	bool  VRenderGraph::GraphicsContext::UpdateHostBuffer (GfxResourceID buffer, BytesU offset, ArrayView<uint> data)
	{
		BytesU	size	= ArraySizeOf(data);
		void *	mapped	= null;

		CHECK_ERR( MapHostBuffer( buffer, offset, INOUT size, OUT mapped ));
		CHECK_ERR( ArraySizeOf(data) == size );

		std::memcpy( mapped, data.data(), size_t(size) );
		return true;
	}
	
/*
=================================================
	MapHostBuffer
=================================================
*/
	bool  VRenderGraph::GraphicsContext::MapHostBuffer (GfxResourceID buffer, BytesU offset, INOUT BytesU &size, OUT void* &mapped)
	{
		auto*	buf = ToLocalBuffer( buffer );
		CHECK_ERR( buf );
		CHECK_ERR( buf->IsHostVisible() );

		VResourceMemoryInfo		mem_info = {};
		CHECK_ERR( buf->ToGlobal()->GetMemoryInfo( OUT mem_info ));
		CHECK_ERR( mem_info.mappedPtr );

		offset	= Min( offset, buf->Size()-1 );
		size	= Min( size, buf->Size() - offset );
		mapped	= mem_info.mappedPtr + offset;

		if ( _enableBarriers )
		{
			if ( not EnumEq( mem_info.flags, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT ))
			{
				_barrierMngr.FlushMemory( mem_info.memory, VkDeviceSize(mem_info.offset), VkDeviceSize(mem_info.size) );
			}

			//_AddBuffer( buf, EResourceState::HostWrite, VkDeviceSize(offset), VkDeviceSize(size) );
		}
		return true;
	}
	
/*
=================================================
	ReadBuffer
=================================================
*/
	bool  VRenderGraph::GraphicsContext::ReadBuffer (GfxResourceID buffer, BytesU offset, BytesU size, const Function<void (BufferView)> &fn)
	{
		// TODO
		return false;
	}
	
/*
=================================================
	ReadImage
=================================================
*/
	bool  VRenderGraph::GraphicsContext::ReadImage (GfxResourceID image, const Function<void (ImageView)> &fn)
	{
		// TODO
		return false;
	}

/*
=================================================
	CopyBuffer
=================================================
*/
	void  VRenderGraph::GraphicsContext::CopyBuffer (GfxResourceID srcBuffer, GfxResourceID dstBuffer, ArrayView<BufferCopy> ranges)
	{
		auto*	src_buf = ToLocalBuffer( srcBuffer );
		auto*	dst_buf = ToLocalBuffer( dstBuffer );
		CHECK_ERR( src_buf and dst_buf, void());

		VkBufferCopy*	vk_ranges = _allocator.Alloc<VkBufferCopy>( ranges.size() );

		for (size_t i = 0; i < ranges.size(); ++i)
		{
			auto&	src = ranges[i];
			auto&	dst = vk_ranges[i];

			ASSERT( All( src.srcOffset < src_buf->Size() ));
			ASSERT( All( src.dstOffset < dst_buf->Size() ));
			ASSERT( All( (src.srcOffset + src.size) <= src_buf->Size() ));
			ASSERT( All( (src.dstOffset + src.size) <= dst_buf->Size() ));

			dst.srcOffset	= VkDeviceSize(Min( src.srcOffset, src_buf->Size()-1 ));
			dst.dstOffset	= VkDeviceSize(Min( src.dstOffset, dst_buf->Size()-1 ));
			dst.size		= VkDeviceSize(Min( src.size, src_buf->Size() - src.srcOffset, dst_buf->Size() - src.dstOffset ));
		}
		
		if ( _enableBarriers )
		{
			for (size_t i = 0; i < ranges.size(); ++i)
			{
				_AddBuffer( src_buf, EResourceState::TransferSrc, vk_ranges[i].srcOffset, vk_ranges[i].size );
				_AddBuffer( dst_buf, EResourceState::TransferDst, vk_ranges[i].dstOffset, vk_ranges[i].size );
			}
		}
		_CommitBarriers();

		vkCmdCopyBuffer( _cmdbuf, src_buf->Handle(), dst_buf->Handle(), uint(ranges.size()), vk_ranges );

		_allocator.Discard();
		++_cmdCounter;
	}
	
/*
=================================================
	CopyImage
=================================================
*/
	void  VRenderGraph::GraphicsContext::CopyImage (GfxResourceID srcImage, GfxResourceID dstImage, ArrayView<ImageCopy> ranges)
	{
		auto*	src_img = ToLocalImage( srcImage );
		auto*	dst_img = ToLocalImage( dstImage );
		CHECK_ERR( src_img and dst_img, void());

		VkImageCopy*	vk_ranges = _allocator.Alloc<VkImageCopy>( ranges.size() );

		for (size_t i = 0; i < ranges.size(); ++i)
		{
			auto&	src = ranges[i];
			auto&	dst = vk_ranges[i];

			ASSERT( All( src.srcOffset < src_img->Dimension() ));
			ASSERT( All( (src.srcOffset + src.extent) <= src_img->Dimension() ));
			ASSERT( All( src.dstOffset < dst_img->Dimension() ));
			ASSERT( All( (src.dstOffset + src.extent) <= dst_img->Dimension() ));

			dst.srcOffset	= { int(src.srcOffset.x), int(src.srcOffset.y), int(src.srcOffset.z) };
			dst.dstOffset	= { int(src.dstOffset.x), int(src.dstOffset.y), int(src.dstOffset.z) };
			dst.extent		= { src.extent.x,         src.extent.y,         src.extent.z };

			ConvertImageSubresourceLayers( OUT dst.srcSubresource, src.srcSubresource, src_img->Description() );
			ConvertImageSubresourceLayers( OUT dst.dstSubresource, src.dstSubresource, dst_img->Description() );
		}
		
		if ( _enableBarriers )
		{
			_AddImage( src_img, EResourceState::TransferSrc, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
						StructView<VkImageSubresourceLayers>{ vk_ranges, ranges.size(), offsetof( VkImageCopy, srcSubresource ) });

			_AddImage( dst_img, EResourceState::TransferDst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
						StructView<VkImageSubresourceLayers>{ vk_ranges, ranges.size(), offsetof( VkImageCopy, dstSubresource ) });
		}
		_CommitBarriers();

		vkCmdCopyImage( _cmdbuf, src_img->Handle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					    dst_img->Handle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					    uint(ranges.size()), vk_ranges );
		_allocator.Discard();
		++_cmdCounter;
	}
	
/*
=================================================
	CopyBufferToImage
=================================================
*/
	void  VRenderGraph::GraphicsContext::CopyBufferToImage (GfxResourceID srcBuffer, GfxResourceID dstImage, ArrayView<BufferImageCopy> ranges)
	{
		auto*	src_buf = ToLocalBuffer( srcBuffer );
		auto*	dst_img = ToLocalImage( dstImage );
		CHECK_ERR( src_buf and dst_img, void());

		VkBufferImageCopy*	vk_ranges = _allocator.Alloc<VkBufferImageCopy>( ranges.size() );
		ConvertBufferImageCopy( OUT vk_ranges, ranges.data(), ranges.size(), dst_img->Description() );
		
		if ( _enableBarriers )
		{
			_AddBuffer( src_buf, EResourceState::TransferSrc, ArrayView<VkBufferImageCopy>{ vk_ranges, ranges.size() });
			
			_AddImage( dst_img, EResourceState::TransferDst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					   StructView<VkImageSubresourceLayers>{ vk_ranges, ranges.size(), offsetof( VkBufferImageCopy, imageSubresource ) });
		}
		_CommitBarriers();

		vkCmdCopyBufferToImage( _cmdbuf, src_buf->Handle(), dst_img->Handle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, uint(ranges.size()), vk_ranges );
		_allocator.Discard();
		++_cmdCounter;
	}
	
/*
=================================================
	CopyImageToBuffer
=================================================
*/
	void  VRenderGraph::GraphicsContext::CopyImageToBuffer (GfxResourceID srcImage, GfxResourceID dstBuffer, ArrayView<BufferImageCopy> ranges)
	{
		auto*	src_img = ToLocalImage( srcImage );
		auto*	dst_buf = ToLocalBuffer( dstBuffer );
		CHECK_ERR( src_img and dst_buf, void());
		
		VkBufferImageCopy*	vk_ranges = _allocator.Alloc<VkBufferImageCopy>( ranges.size() );
		ConvertBufferImageCopy( OUT vk_ranges, ranges.data(), ranges.size(), src_img->Description() );
		
		if ( _enableBarriers )
		{
			_AddImage( src_img, EResourceState::TransferSrc, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
						StructView<VkImageSubresourceLayers>{ vk_ranges, ranges.size(), offsetof( VkBufferImageCopy, imageSubresource ) });
			
			_AddBuffer( dst_buf, EResourceState::TransferDst, ArrayView<VkBufferImageCopy>{ vk_ranges, ranges.size() });
		}
		_CommitBarriers();

		vkCmdCopyImageToBuffer( _cmdbuf, src_img->Handle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst_buf->Handle(), uint(ranges.size()), vk_ranges );
		_allocator.Discard();
		++_cmdCounter;
	}
	
/*
=================================================
	Present
=================================================
*/
	void  VRenderGraph::GraphicsContext::Present (GfxResourceID image, MipmapLevel level, ImageLayer layer)
	{
		// TODO
	}
	
/*
=================================================
	BindPipeline
=================================================
*/
	void  VRenderGraph::GraphicsContext::BindPipeline (ComputePipelineID ppln)
	{
		auto*	cppln = AcquireResource( ppln );
		CHECK_ERR( cppln, void());

		if ( _states.computePipeline == cppln->Handle() )
			return;

		_states.computePipeline	= cppln->Handle();
		_states.computeLayout	= cppln->Layout();
		vkCmdBindPipeline( _cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, _states.computePipeline );
		++_cmdCounter;
	}
	
/*
=================================================
	BindDescriptorSet
=================================================
*/
	void  VRenderGraph::GraphicsContext::BindDescriptorSet (uint index, DescriptorSetID ds, ArrayView<uint> dynamicOffsets)
	{
		// TODO
	}
	
/*
=================================================
	PushConstant
=================================================
*/
	void  VRenderGraph::GraphicsContext::PushConstant (BytesU offset, BytesU size, const void *values, EShaderStages stages)
	{
		vkCmdPushConstants( _cmdbuf, _states.computeLayout, VEnumCast(stages), uint(offset), uint(size), values );
		++_cmdCounter;
	}
	
/*
=================================================
	Dispatch
=================================================
*/
	void  VRenderGraph::GraphicsContext::Dispatch (const uint3 &groupCount)
	{
		ASSERT( _states.computePipeline );
		
		_CommitBarriers();

		vkCmdDispatch( _cmdbuf, groupCount.x, groupCount.y, groupCount.z );
		++_cmdCounter;
	}
	
/*
=================================================
	DispatchIndirect
=================================================
*/
	void  VRenderGraph::GraphicsContext::DispatchIndirect (GfxResourceID buffer, BytesU offset)
	{
		ASSERT( _states.computePipeline );

		auto*	buf = ToLocalBuffer( buffer );
		CHECK_ERR( buf, void());

		if ( _enableBarriers ) {
			_AddBuffer( buf, EResourceState::IndirectBuffer, VkDeviceSize(offset), sizeof(VDispatchIndirectCommand) );
		}
		_CommitBarriers();

		vkCmdDispatchIndirect( _cmdbuf, buf->Handle(), VkDeviceSize(offset) );
		++_cmdCounter;
	}
	
/*
=================================================
	DispatchBase
=================================================
*/
	void  VRenderGraph::GraphicsContext::DispatchBase (const uint3 &baseGroup, const uint3 &groupCount)
	{
		ASSERT( _states.computePipeline );
		
		_CommitBarriers();

		vkCmdDispatchBaseKHR( _cmdbuf, baseGroup.x, baseGroup.y, baseGroup.z, groupCount.x, groupCount.y, groupCount.z );
		++_cmdCounter;
	}
	
/*
=================================================
	BlitImage
=================================================
*/
	void  VRenderGraph::GraphicsContext::BlitImage (GfxResourceID srcImage, GfxResourceID dstImage, EBlitFilter filter, ArrayView<ImageBlit> regions)
	{
		auto*	src_img = ToLocalImage( srcImage );
		auto*	dst_img = ToLocalImage( dstImage );
		CHECK_ERR( src_img and dst_img, void());

		VkImageBlit*	vk_regions = _allocator.Alloc<VkImageBlit>( regions.size() );

		for (size_t i = 0; i < regions.size(); ++i)
		{
			auto&	src = regions[i];
			auto&	dst = vk_regions[i];

			ASSERT( All( src.srcOffset0 <= src_img->Dimension() ));
			ASSERT( All( src.srcOffset1 <= src_img->Dimension() ));
			ASSERT( All( src.dstOffset0 <= dst_img->Dimension() ));
			ASSERT( All( src.dstOffset1 <= dst_img->Dimension() ));

			dst.srcOffsets[0] = { int(src.srcOffset0.x), int(src.srcOffset0.y), int(src.srcOffset0.z) };
			dst.srcOffsets[1] = { int(src.srcOffset1.x), int(src.srcOffset1.y), int(src.srcOffset1.z) };
			dst.dstOffsets[0] = { int(src.dstOffset0.x), int(src.dstOffset0.y), int(src.dstOffset0.z) };
			dst.dstOffsets[1] = { int(src.dstOffset1.x), int(src.dstOffset1.y), int(src.dstOffset1.z) };
			ConvertImageSubresourceLayers( OUT dst.srcSubresource, src.srcSubresource, src_img->Description() );
			ConvertImageSubresourceLayers( OUT dst.dstSubresource, src.dstSubresource, dst_img->Description() );
		}

		if ( _enableBarriers )
		{
			_AddImage( src_img, EResourceState::TransferSrc, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
						StructView<VkImageSubresourceLayers>{ vk_regions, regions.size(), offsetof( VkImageBlit, srcSubresource ) });
			
			_AddImage( dst_img, EResourceState::TransferDst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
						StructView<VkImageSubresourceLayers>{ vk_regions, regions.size(), offsetof( VkImageBlit, dstSubresource ) });
		}
		_CommitBarriers();

		vkCmdBlitImage( _cmdbuf, src_img->Handle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					    dst_img->Handle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					    uint(regions.size()), vk_regions, VEnumCast(filter) );
		_allocator.Discard();
		++_cmdCounter;
	}
	
/*
=================================================
	ResolveImage
=================================================
*/
	void  VRenderGraph::GraphicsContext::ResolveImage (GfxResourceID srcImage, GfxResourceID dstImage, ArrayView<ImageResolve> regions)
	{
		auto*	src_img = ToLocalImage( srcImage );
		auto*	dst_img = ToLocalImage( dstImage );
		CHECK_ERR( src_img and dst_img, void());

		VkImageResolve*	vk_regions = _allocator.Alloc<VkImageResolve>( regions.size() );

		for (size_t i = 0; i < regions.size(); ++i)
		{
			auto&	src = regions[i];
			auto&	dst = vk_regions[i];
			
			ASSERT( All( src.srcOffset < src_img->Dimension() ));
			ASSERT( All( (src.srcOffset + src.extent) <= src_img->Dimension() ));
			ASSERT( All( src.dstOffset < dst_img->Dimension() ));
			ASSERT( All( (src.dstOffset + src.extent) <= dst_img->Dimension() ));

			dst.srcOffset	= { int(src.srcOffset.x), int(src.srcOffset.y), int(src.srcOffset.z) };
			dst.dstOffset	= { int(src.dstOffset.x), int(src.dstOffset.y), int(src.dstOffset.z) };
			dst.extent		= { src.extent.x,         src.extent.y,         src.extent.z };

			ConvertImageSubresourceLayers( OUT dst.srcSubresource, src.srcSubresource, src_img->Description() );
			ConvertImageSubresourceLayers( OUT dst.dstSubresource, src.dstSubresource, dst_img->Description() );
		}
		
		if ( _enableBarriers )
		{
			_AddImage( src_img, EResourceState::TransferSrc, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
						StructView<VkImageSubresourceLayers>{ vk_regions, regions.size(), offsetof( VkImageBlit, srcSubresource ) });

			_AddImage( dst_img, EResourceState::TransferDst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
						StructView<VkImageSubresourceLayers>{ vk_regions, regions.size(), offsetof( VkImageBlit, dstSubresource ) });
		}
		_CommitBarriers();

		vkCmdResolveImage( _cmdbuf, src_img->Handle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
						   dst_img->Handle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
						   uint(regions.size()), vk_regions );
		_allocator.Discard();
		++_cmdCounter;
	}


}	// AE::Graphics
