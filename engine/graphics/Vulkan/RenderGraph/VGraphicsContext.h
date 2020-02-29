// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

namespace AE::Graphics
{

	//
	// Direct Graphics Context
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
		
		using BufferBarriers_t	= HashSet< VLocalBuffer const *>;	// TODO: optimize
		using ImageBarriers_t	= HashSet< VLocalImage const *>;

		class DescriptorSetBarriers;


	// variables
	private:
		VkCommandBuffer				_cmdbuf					= VK_NULL_HANDLE;
		uint						_cmdCounter				= 0;
		bool						_enableBarriers			= true;

		VDevice const&				_device;
		VResourceManager &			_resMngr;
		LinearAllocator<>			_allocator;
		
		VResourceMap				_resources;

		VBarrierManager				_barrierMngr;
		struct {
			BufferBarriers_t			buffers;
			ImageBarriers_t				images;
		}							_pendingBarriers;

		VQueuePtr					_queue;
		VCommandPoolPtr				_cmdPool;
		Ptr<VCommandBatch>			_cmdBatch;
		UniqueID<BakedCommandBufferID>	_cachedCommands;

		VkPipelineStageFlagBits		_allSrcStages			= Zero;
		VkPipelineStageFlagBits		_allDstStages			= Zero;

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
		bool  Begin (VCommandBatch *, VCommandPoolManager &);
		bool  Submit ();
		bool  LastSubmit ();

		bool  Execute (const VLogicalRenderPass &logicalRP, UniqueID<BakedCommandBufferID> renderCommands);

		bool  BeginPass (const VLogicalRenderPass &logicalRP, VkSubpassContents contents);
		void  EndPass (const VLogicalRenderPass &logicalRP);

		void  MergeResources (VResourceMap &);


	// ITransferContext
		NativeContext_t  GetNativeContext () override;
		
		void  ImageBarrier (GfxResourceID image, EResourceState srcState, EResourceState dstState) override;
		void  ImageBarrier (GfxResourceID image, EResourceState srcState, EResourceState dstState, const ImageSubresourceRange &subRes) override;

		void  BufferBarrier (GfxResourceID buffer, EResourceState srcState, EResourceState dstState) override;
		void  BufferBarrier (GfxResourceID buffer, EResourceState srcState, EResourceState dstState, BytesU offset, BytesU size) override;
		
		void  DescriptorsBarrier (DescriptorSetID ds) override;

		void  SetInitialState (GfxResourceID id, EResourceState state) override;
		void  SetInitialState (GfxResourceID id, EResourceState state, EImageLayout layout) override;

		void  GlobalBarrier () override;

		GfxResourceID   GetOutput (GfxResourceID id) override;
		void  SetOutput (GfxResourceID id, GfxResourceID res) override;
		
		void  ClearColorImage (GfxResourceID image, const ClearColor_t &color, ArrayView<ImageSubresourceRange> ranges) override;
		void  ClearDepthStencilImage (GfxResourceID image, const DepthStencil &depthStencil, ArrayView<ImageSubresourceRange> ranges) override;
		void  FillBuffer (GfxResourceID buffer, BytesU offset, BytesU size, uint data) override;

		void  UpdateBuffer (GfxResourceID buffer, BytesU offset, ArrayView<uint8_t> data) override;
		
		bool  UpdateHostBuffer (GfxResourceID buffer, BytesU offset, ArrayView<uint8_t> data) override;
		bool  MapHostBuffer (GfxResourceID buffer, BytesU offset, INOUT BytesU &size, OUT void* &mapped) override;
		
		bool  ReadBuffer (GfxResourceID buffer, BytesU offset, BytesU size, Function<void (const BufferView &)>&& fn) override;
		bool  ReadImage (GfxResourceID image, const uint3 &offset, const uint3 &size, MipmapLevel mipLevel,
						 ImageLayer arrayLayer, EImageAspect aspectMask, Function<void (const ImageView &)>&& fn) override;
		
		bool  UploadBuffer (GfxResourceID buffer, BytesU offset, BytesU size, OUT BufferView &ranges) override;
		bool  UploadImage (GfxResourceID image, const uint3 &offset, const uint3 &size, MipmapLevel mipLevel,
						   ImageLayer arrayLayer, EImageAspect aspectMask, OUT ImageView &ranges) override; 

		void  CopyBuffer (GfxResourceID srcBuffer, GfxResourceID dstBuffer, ArrayView<BufferCopy> ranges) override;
		void  CopyImage (GfxResourceID srcImage, GfxResourceID dstImage, ArrayView<ImageCopy> ranges) override;
		void  CopyBufferToImage (GfxResourceID srcBuffer, GfxResourceID dstImage, ArrayView<BufferImageCopy> ranges) override;
		void  CopyImageToBuffer (GfxResourceID srcImage, GfxResourceID dstBuffer, ArrayView<BufferImageCopy> ranges) override;
		
		void  Present (GfxResourceID image, MipmapLevel level, ImageLayer layer) override;


	// IComputeContext
		void  BindPipeline (ComputePipelineID ppln) override;
		void  BindDescriptorSet (uint index, DescriptorSetID ds, ArrayView<uint> dynamicOffsets) override;
		void  PushConstant (BytesU offset, BytesU size, const void *values, EShaderStages stages) override;

		void  Dispatch (const uint3 &groupCount) override;
		void  DispatchIndirect (GfxResourceID buffer, BytesU offset) override;
		void  DispatchBase (const uint3 &baseGroup, const uint3 &groupCount) override;
		

	// IGraphicsContext
		void  BlitImage (GfxResourceID srcImage, GfxResourceID dstImage, EBlitFilter filter, ArrayView<ImageBlit> regions) override;
		void  ResolveImage (GfxResourceID srcImage, GfxResourceID dstImage, ArrayView<ImageResolve> regions) override;


	private:
		template <typename ID, typename Res, typename MainPool, size_t MC>
		ND_ Res const*  _ToLocal (ID id, INOUT LocalResPool<Res,MainPool,MC> &, StringView msg);
		
		void  _AddImage (const VLocalImage *img, EResourceState state, VkImageLayout layout);
		void  _AddImage (const VLocalImage *img, EResourceState state, VkImageLayout layout, StructView<VkImageSubresourceLayers> subresLayers);
		void  _AddImage (const VLocalImage *img, EResourceState state, VkImageLayout layout, StructView<VkImageSubresourceRange> subres);
		void  _AddBuffer (const VLocalBuffer *buf, EResourceState state);
		void  _AddBuffer (const VLocalBuffer *buf, EResourceState state, VkDeviceSize offset, VkDeviceSize size);
		void  _AddBuffer (const VLocalBuffer *buf, EResourceState state, ArrayView<VkBufferImageCopy> copy);
		void  _CommitBarriers ();

		void  _FlushLocalResourceStates ();

		template <typename ID, typename ResPool>
		void  _FlushLocalResourceStates (ResPool &);
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



	//
	// Descriptor Set Barriers
	//
	class VRenderGraph::GraphicsContext::DescriptorSetBarriers
	{
	// types
	private:
		using EDescriptorType = DescriptorSet::EDescriptorType;

	// variables
	private:
		GraphicsContext &	_gctx;
		ArrayView<uint>		_dynamicOffsets;

	// methods
	public:
		DescriptorSetBarriers (GraphicsContext &gctx, ArrayView<uint> offsets) : _gctx{gctx}, _dynamicOffsets{offsets} {}

		// ResourceGraph //
		void operator () (const UniformName &, EDescriptorType, const DescriptorSet::Buffer &buf);
		void operator () (const UniformName &, EDescriptorType, const DescriptorSet::TexelBuffer &buf);
		void operator () (const UniformName &, EDescriptorType, const DescriptorSet::Image &img);
		void operator () (const UniformName &, EDescriptorType, const DescriptorSet::Texture &tex);
		void operator () (const UniformName &, EDescriptorType, const DescriptorSet::Sampler &) {}
		//void operator () (const UniformName &, const DescriptorSet::RayTracingScene &);
	};

/*
=================================================
	operator (Buffer)
=================================================
*/
	void  VRenderGraph::GraphicsContext::DescriptorSetBarriers::operator () (const UniformName &, EDescriptorType, const DescriptorSet::Buffer &buf)
	{
		for (uint i = 0; i < buf.elementCount; ++i)
		{
			auto&				elem	= buf.elements[i];
			VLocalBuffer const*	buffer	= _gctx.ToLocalBuffer( elem.bufferId );
			if ( not buffer )
				continue;

			// validation
			{
				const VkDeviceSize	offset	= VkDeviceSize(elem.offset) + (buf.dynamicOffsetIndex < _dynamicOffsets.size() ? _dynamicOffsets[buf.dynamicOffsetIndex] : 0);		
				const VkDeviceSize	size	= VkDeviceSize(elem.size == ~0_b ? buffer->Size() - offset : elem.size);

				ASSERT( (size >= buf.staticSize) and (buf.arrayStride == 0 or (size - buf.staticSize) % buf.arrayStride == 0) );
				ASSERT( offset < buffer->Size() );
				ASSERT( offset + size <= buffer->Size() );

				auto&	limits	= _gctx._device.GetProperties().properties.limits;
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

			_gctx._AddBuffer( buffer, buf.state );
		}
	}

/*
=================================================
	operator (TexelBuffer)
=================================================
*/
	void  VRenderGraph::GraphicsContext::DescriptorSetBarriers::operator () (const UniformName &, EDescriptorType, const DescriptorSet::TexelBuffer &texbuf)
	{
		for (uint i = 0; i < texbuf.elementCount; ++i)
		{
			auto&				elem	= texbuf.elements[i];
			VLocalBuffer const*	buffer	= _gctx.ToLocalBuffer( elem.bufferId );
			if ( not buffer )
				continue;

			_gctx._AddBuffer( buffer, texbuf.state );
		}
	}

/*
=================================================
	operator (Image / Texture / SubpassInput)
=================================================
*/
	void  VRenderGraph::GraphicsContext::DescriptorSetBarriers::operator () (const UniformName &, EDescriptorType, const DescriptorSet::Image &img)
	{
		for (uint i = 0; i < img.elementCount; ++i)
		{
			auto&				elem	= img.elements[i];
			VLocalImage const*  image	= _gctx.ToLocalImage( elem.imageId );

			if ( image )
				_gctx._AddImage( image, img.state, EResourceState_ToImageLayout( img.state, image->AspectMask() ));
		}
	}
	
	void  VRenderGraph::GraphicsContext::DescriptorSetBarriers::operator () (const UniformName &, EDescriptorType, const DescriptorSet::Texture &tex)
	{
		for (uint i = 0; i < tex.elementCount; ++i)
		{
			auto&				elem	= tex.elements[i];
			VLocalImage const*  image	= _gctx.ToLocalImage( elem.imageId );

			if ( image )
				_gctx._AddImage( image, tex.state, EResourceState_ToImageLayout( tex.state, image->AspectMask() ));
		}
	}

/*
=================================================
	operator (RayTracingScene)
=================================================
*
	void  VRenderGraph::GraphicsContext::DescriptorSetBarriers::operator () (const UniformName &, EDescriptorType, const DescriptorSet::RayTracingScene &rts)
	{
		for (uint i = 0; i < rts.elementCount; ++i)
		{
			VLocalRTScene const*  scene = _gctx._ToLocal( rts.elements[i].sceneId );
			if ( not scene )
				return;

			_gctx._AddRTScene( scene, EResourceState::RayTracingShaderRead );

			auto&	data = scene->ToGlobal()->CurrentData();
			SHAREDLOCK( data.guard );
			ASSERT( data.geometryInstances.size() );

			for (auto& inst : data.geometryInstances)
			{
				if ( auto* geom = _gctx._ToLocal( inst.geometry.Get() ))
				{
					_gctx._AddRTGeometry( geom, EResourceState::RayTracingShaderRead );
				}
			}
		}
	}*/
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
	}

/*
=================================================
	Create
=================================================
*/
	bool  VRenderGraph::GraphicsContext::Create ()
	{
		_allocator.SetBlockSize( 4_Kb );

		// set source pipeline stages
		{
			VkPipelineStageFlagBits	rt_flags =
				#ifdef VK_NV_ray_tracing
					_device.GetFeatures().rayTracingNV ? VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_NV | VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV : Zero;
				#else
					Zero;
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
	bool  VRenderGraph::GraphicsContext::Begin (VCommandBatch *batch, VCommandPoolManager &cmdPoolMngr)
	{
		CHECK_ERR( not _cmdbuf );
		CHECK_ERR( not _cmdBatch );

		_cmdBatch = batch;

		if ( not _cmdPool )
		{
			_cmdPool = cmdPoolMngr.Acquire( _queue, ECommandPoolType::Primary_OneTimeSubmit );
			CHECK_ERR( _cmdPool );
		}

		_cachedCommands = _resMngr.CreateCommandBuffer( _cmdPool, Default );
		CHECK_ERR( _cachedCommands );

		_cmdbuf = _resMngr.GetResource( _cachedCommands )->GetCommands();
		
		VkCommandBufferBeginInfo	begin_info = {};
		begin_info.sType	= VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.flags	= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		VK_CHECK( vkBeginCommandBuffer( _cmdbuf, &begin_info ));

		// invalidate cache for staging buffers
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

			_resMngr.ReleaseResource( _cachedCommands );
			_resources.Release( _resMngr );

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
		
		_resMngr.GetResource( _cachedCommands )->SetResources( std::move(_resources) );
		_resources = Default;

		_cmdBatch->AddFence( fence );
		_cmdBatch->AddCmdBuffer( std::move(_cachedCommands) );

		_cmdBatch	= null;
		_cmdbuf		= VK_NULL_HANDLE;
		_cmdCounter	= 0;
		return true;
	}
	
/*
=================================================
	LastSubmit
=================================================
*/
	bool  VRenderGraph::GraphicsContext::LastSubmit ()
	{
		bool	res = Submit();

		_cmdPool->Unlock();
		_cmdPool = null;

		return res;
	}

/*
=================================================
	AcquireResource
=================================================
*/
	template <typename ID>
	inline auto*  VRenderGraph::GraphicsContext::AcquireResource (ID id)
	{
		return _resMngr.GetResource( id, _resources.Add( id ));
	}
	
/*
=================================================
	Execute
=================================================
*/
	bool  VRenderGraph::GraphicsContext::Execute (const VLogicalRenderPass &logicalRP, UniqueID<BakedCommandBufferID> renderCommands)
	{
		CHECK( _resources.Add( renderCommands ));

		auto*	cmds = _resMngr.GetResource( renderCommands.Release() );
		CHECK_ERR( cmds );

		// add barriers
		for (auto&[id, state] : cmds->Barriers())
		{
			if ( id.ResourceType() == GfxResourceID::EType::Buffer )
			{
				auto*	buf = ToLocalBuffer( id );
				_AddBuffer( buf, state );
			}
			else
			if ( id.ResourceType() == GfxResourceID::EType::Image )
			{
				auto*	img = ToLocalImage( id );
				_AddImage( img, state, EResourceState_ToImageLayout( state, img->AspectMask() ));
			}
			else
				AE_LOGE( "unsupported resource type" );
		}

		CHECK_ERR( BeginPass( logicalRP, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS ));

		VkCommandBuffer		cmdbuffers[] = { cmds->GetCommands() };
		vkCmdExecuteCommands( _cmdbuf, uint(CountOf( cmdbuffers )), cmdbuffers );

		EndPass( logicalRP );

		++_cmdCounter;
		return true;
	}

/*
=================================================
	BeginPass
=================================================
*/
	bool  VRenderGraph::GraphicsContext::BeginPass (const VLogicalRenderPass &logicalRP, VkSubpassContents contents)
	{
		if ( logicalRP.GetSubpassIndex() == 0 )
		{
			auto*	framebuffer = AcquireResource( logicalRP.GetFramebuffer() );
			auto*	render_pass = AcquireResource( logicalRP.GetRenderPass() );
			CHECK_ERR( framebuffer );
			CHECK_ERR( render_pass );

			for (auto& att : logicalRP.GetColorTargets())
			{
				auto*	img = ToLocalImage( att.imageId );
				CHECK_ERR( img );

				_AddImage( img, att.state, att.layout );
			}
			_CommitBarriers();

			VkRenderPassBeginInfo	pass_info = {};
			pass_info.sType						= VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			pass_info.renderPass				= render_pass->Handle();
			pass_info.renderArea.offset.x		= logicalRP.GetArea().left;
			pass_info.renderArea.offset.y		= logicalRP.GetArea().top;
			pass_info.renderArea.extent.width	= uint(logicalRP.GetArea().Width());
			pass_info.renderArea.extent.height	= uint(logicalRP.GetArea().Height());
			pass_info.clearValueCount			= render_pass->GetCreateInfo().attachmentCount;
			pass_info.pClearValues				= logicalRP.GetClearValues().data();
			pass_info.framebuffer				= framebuffer->Handle();
			
			vkCmdBeginRenderPass( _cmdbuf, &pass_info, contents );
		}
		else
		{
			vkCmdNextSubpass( _cmdbuf, contents );

			// TODO: clear attachment
		}
		
		if ( contents == VK_SUBPASS_CONTENTS_INLINE )
		{
			if ( auto viewports = logicalRP.GetViewports(); viewports.size() )
				vkCmdSetViewport( _cmdbuf, 0, uint(viewports.size()), viewports.data() );
		
			if ( auto scissors = logicalRP.GetScissors(); scissors.size() )
				vkCmdSetScissor( _cmdbuf, 0, uint(scissors.size()), scissors.data() );
		}

		++_cmdCounter;
		return true;
	}
	
/*
=================================================
	EndPass
=================================================
*/
	void  VRenderGraph::GraphicsContext::EndPass (const VLogicalRenderPass &logicalRP)
	{
		if ( logicalRP.IsLastSubpass() )
		{
			vkCmdEndRenderPass( _cmdbuf );
		}
	}
	
/*
=================================================
	MergeResources
=================================================
*/
	void  VRenderGraph::GraphicsContext::MergeResources (VResourceMap &other)
	{
		_resources.Merge( INOUT other );
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
			auto&	res = localRes.pool[ local_idx ];

			res.Destroy( _barrierMngr );
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

		_resources.Add( id );

		CHECK_ERR( localRes.pool.Assign( OUT local ));

		auto&	data = localRes.pool[ local ];
		
		data.Data().~Res();
		PlacementNew<Res>( &data.Data() );

		if ( not data.Create( res ))
		{
			localRes.pool.Unassign( local );
			RETURN_ERR( msg );
		}

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
	inline void  VRenderGraph::GraphicsContext::_AddImage (const VLocalImage *img, EResourceState state, VkImageLayout layout)
	{
		if ( not img->IsMutable() )
			return;
		
		img->AddPendingState( state, layout, ExeOrderIndex(_cmdCounter + 1) );
		
		_pendingBarriers.images.insert( img );
	}

	inline void  VRenderGraph::GraphicsContext::_AddImage (const VLocalImage *img, EResourceState state, VkImageLayout layout, StructView<VkImageSubresourceLayers> subresLayers)
	{
		_AddImage( img, state, layout );
		Unused( subresLayers );
	}
	
	inline void  VRenderGraph::GraphicsContext::_AddImage (const VLocalImage *img, EResourceState state, VkImageLayout layout, StructView<VkImageSubresourceRange> subres)
	{
		_AddImage( img, state, layout );
		Unused( subres );
	}
	
/*
=================================================
	_AddBuffer
=================================================
*/
	inline void  VRenderGraph::GraphicsContext::_AddBuffer (const VLocalBuffer *buf, EResourceState state)
	{
		if ( not buf->IsMutable() )
			return;

		buf->AddPendingState( state, ExeOrderIndex(_cmdCounter + 1) );

		_pendingBarriers.buffers.insert( buf );
	}

	inline void  VRenderGraph::GraphicsContext::_AddBuffer (const VLocalBuffer *buf, EResourceState state, VkDeviceSize offset, VkDeviceSize size)
	{
		_AddBuffer( buf, state );
		Unused( offset, size );
	}
	
	inline void  VRenderGraph::GraphicsContext::_AddBuffer (const VLocalBuffer *buf, EResourceState state, ArrayView<VkBufferImageCopy> ranges)
	{
		_AddBuffer( buf, state );
		Unused( ranges );
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
	ImageBarrier
=================================================
*/
	void  VRenderGraph::GraphicsContext::ImageBarrier (GfxResourceID image, EResourceState srcState, EResourceState dstState)
	{
		CHECK_ERR( not _enableBarriers, void());

		auto*	img = ToLocalImage( image );
		CHECK_ERR( img, void());
		
		VkImageMemoryBarrier	barrier = {};
		barrier.subresourceRange	= { img->AspectMask(), 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS };
		barrier.sType				= VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		barrier.image				= img->Handle();
		barrier.oldLayout			= EResourceState_ToImageLayout( srcState, barrier.subresourceRange.aspectMask );
		barrier.newLayout			= EResourceState_ToImageLayout( dstState, barrier.subresourceRange.aspectMask );
		barrier.srcQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
		barrier.srcAccessMask		= EResourceState_ToAccess( srcState );
		barrier.dstAccessMask		= EResourceState_ToAccess( dstState );
		auto	src_stage			= EResourceState_ToPipelineStages( srcState );
		auto	dst_stage			= EResourceState_ToPipelineStages( dstState );

		_barrierMngr.AddImageBarrier( src_stage, dst_stage, Zero, barrier );
	}
	
/*
=================================================
	ImageBarrier
=================================================
*/
	void  VRenderGraph::GraphicsContext::ImageBarrier (GfxResourceID image, EResourceState srcState, EResourceState dstState, const ImageSubresourceRange &subRes)
	{
		CHECK_ERR( not _enableBarriers, void());

		auto*	img = ToLocalImage( image );
		CHECK_ERR( img, void());
		
		VkImageMemoryBarrier	barrier = {};
		barrier.subresourceRange.aspectMask		= VEnumCast( subRes.aspectMask );
		barrier.subresourceRange.baseMipLevel	= subRes.baseMipLevel.Get();	// TODO: validate?
		barrier.subresourceRange.levelCount		= subRes.levelCount;
		barrier.subresourceRange.baseArrayLayer	= subRes.baseArrayLayer.Get();
		barrier.subresourceRange.layerCount		= subRes.layerCount;
		barrier.sType				= VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		barrier.image				= img->Handle();
		barrier.oldLayout			= EResourceState_ToImageLayout( srcState, barrier.subresourceRange.aspectMask );
		barrier.newLayout			= EResourceState_ToImageLayout( dstState, barrier.subresourceRange.aspectMask );
		barrier.srcQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
		barrier.srcAccessMask		= EResourceState_ToAccess( srcState );
		barrier.dstAccessMask		= EResourceState_ToAccess( dstState );
		auto	src_stage			= EResourceState_ToPipelineStages( srcState );
		auto	dst_stage			= EResourceState_ToPipelineStages( dstState );

		_barrierMngr.AddImageBarrier( src_stage, dst_stage, Zero, barrier );
	}
	
/*
=================================================
	BufferBarrier
=================================================
*/
	void  VRenderGraph::GraphicsContext::BufferBarrier (GfxResourceID buffer, EResourceState srcState, EResourceState dstState)
	{
		CHECK_ERR( not _enableBarriers, void());

		auto*	buf = ToLocalBuffer( buffer );
		CHECK_ERR( buf, void());
		
		VkBufferMemoryBarrier	barrier = {};
		barrier.sType				= VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		barrier.buffer				= buf->Handle();
		barrier.offset				= 0;
		barrier.size				= VK_WHOLE_SIZE;
		barrier.srcQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
		barrier.srcAccessMask		= EResourceState_ToAccess( srcState );
		barrier.dstAccessMask		= EResourceState_ToAccess( dstState );
		auto	src_stage			= EResourceState_ToPipelineStages( srcState );
		auto	dst_stage			= EResourceState_ToPipelineStages( dstState );
		
		_barrierMngr.AddBufferBarrier( src_stage, dst_stage, barrier );
	}
	
/*
=================================================
	BufferBarrier
=================================================
*/
	void  VRenderGraph::GraphicsContext::BufferBarrier (GfxResourceID buffer, EResourceState srcState, EResourceState dstState, BytesU offset, BytesU size)
	{
		CHECK_ERR( not _enableBarriers, void());

		auto*	buf = ToLocalBuffer( buffer );
		CHECK_ERR( buf, void());
		
		VkBufferMemoryBarrier	barrier = {};
		barrier.sType				= VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		barrier.buffer				= buf->Handle();
		barrier.offset				= VkDeviceSize(offset);	// TODO: validate?
		barrier.size				= VkDeviceSize(size);
		barrier.srcQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
		barrier.srcAccessMask		= EResourceState_ToAccess( srcState );
		barrier.dstAccessMask		= EResourceState_ToAccess( dstState );
		auto	src_stage			= EResourceState_ToPipelineStages( srcState );
		auto	dst_stage			= EResourceState_ToPipelineStages( dstState );
		
		_barrierMngr.AddBufferBarrier( src_stage, dst_stage, barrier );
	}
	
/*
=================================================
	DescriptorsBarrier
=================================================
*/
	void  VRenderGraph::GraphicsContext::DescriptorsBarrier (DescriptorSetID ds)
	{
		auto*	desc_set = AcquireResource( ds );
		CHECK_ERR( desc_set, void());

		if ( _enableBarriers )
		{
			DescriptorSetBarriers	visitor{ *this, Default };
			desc_set->ForEachUniform( visitor );
		}
	}

/*
=================================================
	SetInitialState
=================================================
*/
	void  VRenderGraph::GraphicsContext::SetInitialState (GfxResourceID id, EResourceState state)
	{
		BEGIN_ENUM_CHECKS();
		switch ( id.ResourceType() )
		{
			case GfxResourceID::EType::Buffer :
			{
				if ( auto* buf = ToLocalBuffer( id ))
					buf->SetInitialState( state );
				break;
			}
			case GfxResourceID::EType::Image :
			{
				if ( auto* img = ToLocalImage( id ))
					img->SetInitialState( state );
				break;
			}
			case GfxResourceID::EType::Unknown :
			case GfxResourceID::EType::RayTracingGeometry :
			case GfxResourceID::EType::RayTracingScene :
			case GfxResourceID::EType::VirtualBuffer :
			case GfxResourceID::EType::VirtualImage :
			case GfxResourceID::EType::_Count :
			default :
				ASSERT(!"current resource type is not supported");
				break;
		}
		END_ENUM_CHECKS();
	}
	
/*
=================================================
	SetInitialState
=================================================
*/
	void  VRenderGraph::GraphicsContext::SetInitialState (GfxResourceID id, EResourceState state, EImageLayout layout)
	{
		BEGIN_ENUM_CHECKS();
		switch ( id.ResourceType() )
		{
			case GfxResourceID::EType::Image :
			{
				if ( auto* img = ToLocalImage( id ))
					img->SetInitialState( state, layout );
				break;
			}
			case GfxResourceID::EType::Unknown :
			case GfxResourceID::EType::Buffer :
			case GfxResourceID::EType::RayTracingGeometry :
			case GfxResourceID::EType::RayTracingScene :
			case GfxResourceID::EType::VirtualBuffer :
			case GfxResourceID::EType::VirtualImage :
			case GfxResourceID::EType::_Count :
			default :
				ASSERT(!"current resource type is not supported");
				break;
		}
		END_ENUM_CHECKS();
	}

/*
=================================================
	GlobalBarrier
=================================================
*/
	void  VRenderGraph::GraphicsContext::GlobalBarrier ()
	{
		CHECK_ERR( not _enableBarriers, void());

		VkMemoryBarrier		barrier = {};
		barrier.sType			= VK_STRUCTURE_TYPE_MEMORY_BARRIER;
		barrier.srcAccessMask	= _allSrcStages & VWriteAccessMask;
		barrier.dstAccessMask	= _allDstStages;

		vkCmdPipelineBarrier( _cmdbuf, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 1, &barrier, 0, null, 0, null );
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
		Unused( id, res );
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
	void  VRenderGraph::GraphicsContext::UpdateBuffer (GfxResourceID buffer, BytesU offset, ArrayView<uint8_t> data)
	{
		BytesU	size = ArraySizeOf(data);
		auto*	buf  = ToLocalBuffer( buffer );
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
	bool  VRenderGraph::GraphicsContext::UpdateHostBuffer (GfxResourceID buffer, BytesU offset, ArrayView<uint8_t> data)
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
	bool  VRenderGraph::GraphicsContext::ReadBuffer (GfxResourceID srcBuffer, const BytesU offset, BytesU size, Function<void (const BufferView &)>&& fn)
	{
		auto*	buf = ToLocalBuffer( srcBuffer );
		CHECK_ERR( buf and fn and  offset < buf->Size() );

		size = Min( size, buf->Size() - offset );
		
		const uint		max_parts	= BufferView::Count;
		const BytesU	min_part	= 4_Kb;
		const BytesU	min_size	= Min( (size + max_parts-1) / max_parts, Min( size, min_part ));
		GfxResourceID	staging_buf;
		BytesU			readn;
		BufferView		view;

		FixedArray<BufferCopy, max_parts>	copy;

		// copy to staging buffer
		for (; readn < size;)
		{
			BufferView::Data	range;
			GfxResourceID		dst_buf;
			BytesU				dst_offset;

			CHECK_ERR( _cmdBatch->GetReadable( size - readn, 1_b, 16_b, min_size, OUT dst_buf, OUT dst_offset, OUT range ));
			
			if ( copy.size() and (dst_buf != staging_buf or copy.size() == copy.capacity()) )
			{
				CopyBuffer( srcBuffer, staging_buf, copy );
				copy.clear();
			}

			copy.push_back({ offset + readn, dst_offset, range.size });
			CHECK_ERR( view.try_push_back( range.ptr, range.size ));

			staging_buf  = dst_buf;
			readn		+= range.size;
		}
		
		if ( copy.size() )
			CopyBuffer( srcBuffer, staging_buf, copy );
		
		_cmdBatch->AddDataLoadedEvent( VCommandBatch::OnBufferDataLoadedEvent{ std::move(fn), std::move(view) });
		return true;
	}
	
/*
=================================================
	ReadImage
=================================================
*/
	bool  VRenderGraph::GraphicsContext::ReadImage (GfxResourceID srcImage, const uint3 &imageOffset, const uint3 &imageSize, MipmapLevel mipLevel,
													ImageLayer arrayLayer, EImageAspect aspectMask, Function<void (const ImageView &)>&& fn)
	{
		auto*	img = ToLocalImage( srcImage );
		CHECK_ERR( img );
		
		ASSERT(All( imageOffset < img->Dimension() ));
		ASSERT(All( imageSize <= img->Dimension() ));
		ASSERT( mipLevel < img->Description().maxLevel );
		ASSERT( arrayLayer < img->Description().arrayLayers );

		const uint		max_parts		= BufferView::Count;
		const BytesU	stagingbuf_size	= _resMngr.GetHostReadBufferSize();
		const auto&		fmt_info		= EPixelFormat_GetInfo( img->Description().format );
		const auto&		block_dim		= fmt_info.blockSize;
		const uint		block_size		= aspectMask != EImageAspect::Stencil ? fmt_info.bitsPerBlock : fmt_info.bitsPerBlock2;
		const uint3		image_size		= Max( imageSize, uint3(1) );
		const BytesU	row_pitch		= BytesU(image_size.x * block_size + block_dim.x-1) / (block_dim.x * 8);
		const BytesU	slice_pitch		= (image_size.y * row_pitch + block_dim.y-1) / block_dim.y;
		const BytesU	total_size		= slice_pitch * image_size.z;
		const BytesU	min_size		= (total_size + max_parts-1) / max_parts;
		const uint		row_length		= image_size.x;
		const uint		img_height		= image_size.y;
		GfxResourceID	staging_buf;
		BytesU			readn;
		BufferView		view;

		FixedArray<BufferImageCopy, max_parts>	copy;
		
		// copy to staging buffer slice by slice
		if ( total_size < stagingbuf_size )
		{
			uint	z_offset = 0;
			for (BytesU written; written < total_size;)
			{
				BufferView::Data	range;
				GfxResourceID		dst_buf;
				BytesU				dst_offset;

				CHECK_ERR( _cmdBatch->GetReadable( total_size - readn, 1_b, slice_pitch, min_size, OUT dst_buf, OUT dst_offset, OUT range ));
				
				if ( copy.size() and (dst_buf != staging_buf or copy.size() == copy.capacity()) )
				{
					CopyImageToBuffer( srcImage, staging_buf, copy );
					copy.clear();
				}
				
				CHECK_ERR( view.try_push_back( range.ptr, range.size ));
				
				const uint	z_size = uint(range.size / slice_pitch);

				auto&	reg = copy.emplace_back();
				reg.bufferImageHeight	= img_height;
				reg.bufferOffset		= dst_offset;
				reg.bufferRowLength		= row_length;
				reg.imageExtent			= {image_size.x,  image_size.y,  z_size};
				reg.imageOffset			= {imageOffset.x, imageOffset.y, imageOffset.z + z_offset};
				reg.imageSubresource.aspectMask		= aspectMask;
				reg.imageSubresource.baseArrayLayer	= arrayLayer;
				reg.imageSubresource.layerCount		= 1;
				reg.imageSubresource.mipLevel		= mipLevel;

				staging_buf  = dst_buf;
				written		+= range.size;
				z_offset	+= z_size;
			}
			ASSERT( z_offset == image_size.z );
		}
		else
		
		// copy to staging buffer row by row
		for (uint slice = 0; slice < image_size.z; ++slice)
		{
			uint	y_offset = 0;

			for (BytesU written; written < slice_pitch;)
			{
				BufferView::Data	range;
				GfxResourceID		dst_buf;
				BytesU				dst_offset;

				CHECK_ERR( _cmdBatch->GetReadable( total_size - readn, 1_b, row_pitch, min_size, OUT dst_buf, OUT dst_offset, OUT range ));
				
				if ( copy.size() and (dst_buf != staging_buf or copy.size() == copy.capacity()) )
				{
					CopyImageToBuffer( srcImage, staging_buf, copy );
					copy.clear();
				}
				
				CHECK_ERR( view.try_push_back( range.ptr, range.size ));

				const uint	y_size = uint((range.size * block_dim.y) / row_pitch);
				
				auto&	reg = copy.emplace_back();
				reg.bufferImageHeight	= img_height;
				reg.bufferOffset		= dst_offset;
				reg.bufferRowLength		= row_length;
				reg.imageExtent			= {image_size.x,  y_size,                  1u};
				reg.imageOffset			= {imageOffset.x, imageOffset.y + y_offset, imageOffset.z};
				reg.imageSubresource.aspectMask		= aspectMask;
				reg.imageSubresource.baseArrayLayer	= arrayLayer;
				reg.imageSubresource.layerCount		= 1;
				reg.imageSubresource.mipLevel		= mipLevel;

				staging_buf  = dst_buf;
				written		+= range.size;
				y_offset	+= y_size;
			}
			ASSERT( y_offset == image_size.y );
		}
		
		if ( copy.size() )
			CopyImageToBuffer( srcImage, staging_buf, copy );

		_cmdBatch->AddDataLoadedEvent( VCommandBatch::OnImageDataLoadedEvent{
			std::move(fn),
			ImageView{ std::move(view), image_size, row_pitch, slice_pitch, img->Description().format, aspectMask }});

		return true;
	}
	
/*
=================================================
	UploadBuffer
=================================================
*/
	bool  VRenderGraph::GraphicsContext::UploadBuffer (GfxResourceID dstBuffer, const BytesU offset, BytesU size, OUT BufferView &outRanges)
	{
		outRanges.clear();
		
		auto*	buf = ToLocalBuffer( dstBuffer );
		CHECK_ERR( buf );
		CHECK_ERR( offset < buf->Size() );

		size = Min( size, buf->Size() - offset );
		
		const uint		max_parts	= BufferView::Count;
		const BytesU	min_part	= 4_Kb;
		const BytesU	min_size	= Min( (size + max_parts-1) / max_parts, Min( size, min_part ));
		GfxResourceID	staging_buf;
		BytesU			writen;

		FixedArray<BufferCopy, max_parts>	copy;

		// copy from staging buffer to dest buffer
		for (; writen < size;)
		{
			BufferView::Data	range;
			GfxResourceID		src_buf;
			BytesU				src_offset;

			CHECK_ERR( _cmdBatch->GetWritable( size - writen, 1_b, 16_b, min_size, OUT src_buf, OUT src_offset, OUT range ));

			if ( copy.size() and src_buf != staging_buf )
			{
				CopyBuffer( staging_buf, dstBuffer, copy );
				copy.clear();
			}

			copy.push_back({ src_offset, offset + writen, range.size });
			CHECK_ERR( outRanges.try_push_back( range.ptr, range.size ));

			staging_buf  = src_buf;
			writen		+= range.size;
		}
		
		if ( copy.size() )
			CopyBuffer( staging_buf, dstBuffer, copy );

		return true;
	}
	
/*
=================================================
	UploadImage
=================================================
*/
	bool  VRenderGraph::GraphicsContext::UploadImage (GfxResourceID dstImage, const uint3 &imageOffset, const uint3 &imageSize, MipmapLevel mipLevel,
													  ImageLayer arrayLayer, EImageAspect aspectMask, OUT ImageView &outRanges)
	{
		auto*	img = ToLocalImage( dstImage );
		CHECK_ERR( img );
		
		ASSERT(All( imageOffset < img->Dimension() ));
		ASSERT(All( imageSize <= img->Dimension() ));
		ASSERT( mipLevel < img->Description().maxLevel );
		ASSERT( arrayLayer < img->Description().arrayLayers );

		const uint		max_parts		= BufferView::Count;
		const BytesU	stagingbuf_size	= _resMngr.GetHostReadBufferSize();
		const auto&		fmt_info		= EPixelFormat_GetInfo( img->Description().format );
		const auto&		block_dim		= fmt_info.blockSize;
		const uint		block_size		= aspectMask != EImageAspect::Stencil ? fmt_info.bitsPerBlock : fmt_info.bitsPerBlock2;
		const uint3		image_size		= Max( imageSize, uint3(1) );
		const BytesU	row_pitch		= BytesU(image_size.x * block_size + block_dim.x-1) / (block_dim.x * 8);
		const BytesU	slice_pitch		= (image_size.y * row_pitch + block_dim.y-1) / block_dim.y;
		const BytesU	total_size		= slice_pitch * image_size.z;
		const BytesU	min_size		= (total_size + max_parts-1) / max_parts;
		const uint		row_length		= image_size.x;
		const uint		img_height		= image_size.y;
		GfxResourceID	staging_buf;
		BytesU			readn;
		BufferView		view;

		FixedArray<BufferImageCopy, max_parts>	copy;
		
		// copy to staging buffer slice by slice
		if ( total_size < stagingbuf_size )
		{
			uint	z_offset = 0;
			for (BytesU written; written < total_size;)
			{
				BufferView::Data	range;
				GfxResourceID		dst_buf;
				BytesU				dst_offset;

				CHECK_ERR( _cmdBatch->GetWritable( total_size - readn, 1_b, slice_pitch, min_size, OUT dst_buf, OUT dst_offset, OUT range ));
				
				if ( copy.size() and (dst_buf != staging_buf or copy.size() == copy.capacity()) )
				{
					CopyBufferToImage( staging_buf, dstImage, copy );
					copy.clear();
				}
				
				CHECK_ERR( view.try_push_back( range.ptr, range.size ));
				
				const uint	z_size = uint(range.size / slice_pitch);

				auto&	reg = copy.emplace_back();
				reg.bufferImageHeight	= img_height;
				reg.bufferOffset		= dst_offset;
				reg.bufferRowLength		= row_length;
				reg.imageExtent			= {image_size.x,  image_size.y,  z_size};
				reg.imageOffset			= {imageOffset.x, imageOffset.y, imageOffset.z + z_offset};
				reg.imageSubresource.aspectMask		= aspectMask;
				reg.imageSubresource.baseArrayLayer	= arrayLayer;
				reg.imageSubresource.layerCount		= 1;
				reg.imageSubresource.mipLevel		= mipLevel;

				staging_buf  = dst_buf;
				written		+= range.size;
				z_offset	+= z_size;
			}
			ASSERT( z_offset == image_size.z );
		}
		else
		
		// copy to staging buffer row by row
		for (uint slice = 0; slice < image_size.z; ++slice)
		{
			uint	y_offset = 0;

			for (BytesU written; written < slice_pitch;)
			{
				BufferView::Data	range;
				GfxResourceID		dst_buf;
				BytesU				dst_offset;

				CHECK_ERR( _cmdBatch->GetWritable( total_size - readn, 1_b, row_pitch, min_size, OUT dst_buf, OUT dst_offset, OUT range ));
				
				if ( copy.size() and (dst_buf != staging_buf or copy.size() == copy.capacity()) )
				{
					CopyBufferToImage( staging_buf, dstImage, copy );
					copy.clear();
				}
				
				CHECK_ERR( view.try_push_back( range.ptr, range.size ));

				const uint	y_size = uint((range.size * block_dim.y) / row_pitch);
				
				auto&	reg = copy.emplace_back();
				reg.bufferImageHeight	= img_height;
				reg.bufferOffset		= dst_offset;
				reg.bufferRowLength		= row_length;
				reg.imageExtent			= {image_size.x,  y_size,                   1u};
				reg.imageOffset			= {imageOffset.x, imageOffset.y + y_offset, imageOffset.z};
				reg.imageSubresource.aspectMask		= aspectMask;
				reg.imageSubresource.baseArrayLayer	= arrayLayer;
				reg.imageSubresource.layerCount		= 1;
				reg.imageSubresource.mipLevel		= mipLevel;

				staging_buf  = dst_buf;
				written		+= range.size;
				y_offset	+= y_size;
			}
			ASSERT( y_offset == image_size.y );
		}
		
		if ( copy.size() )
			CopyBufferToImage( staging_buf, dstImage, copy );

		outRanges = ImageView{ std::move(view), image_size, row_pitch, slice_pitch, img->Description().format, aspectMask };
		return true;
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
						StructView<VkImageSubresourceLayers>{ vk_ranges, ranges.size(), offsetof( VkImageCopy, srcSubresource )});

			_AddImage( dst_img, EResourceState::TransferDst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
						StructView<VkImageSubresourceLayers>{ vk_ranges, ranges.size(), offsetof( VkImageCopy, dstSubresource )});
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
		Unused( image, level, layer );
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
		ASSERT( _states.computeLayout );

		auto*	desc_set = AcquireResource( ds );
		CHECK_ERR( desc_set, void());

		if ( _enableBarriers )
		{
			DescriptorSetBarriers	visitor{ *this, dynamicOffsets };
			desc_set->ForEachUniform( visitor );
		}

		const VkDescriptorSet	vk_ds[] = { desc_set->Handle() };

		vkCmdBindDescriptorSets( _cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, _states.computeLayout, index, uint(CountOf(vk_ds)), vk_ds, uint(dynamicOffsets.size()), dynamicOffsets.data() );
		++_cmdCounter;
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
