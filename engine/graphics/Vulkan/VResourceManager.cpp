// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#ifdef AE_ENABLE_VULKAN

# include "graphics/Vulkan/VResourceManager.h"
# include "graphics/Vulkan/VEnumCast.h"

# include "graphics/Vulkan/Allocators/VUniMemAllocator.h"

# include "stl/Memory/LinearAllocator.h"

namespace AE::Graphics
{
namespace {
/*
=================================================
	Replace
----
	destroy previous resource instance and construct new instance
=================================================
*/
	template <typename ResType, typename ...Args>
	inline void Replace (INOUT VResourceBase<ResType> &target, Args&& ...args)
	{
		target.Data().~ResType();
		new (&target.Data()) ResType{ std::forward<Args &&>(args)... };
	}
}
//-----------------------------------------------------------------------------

	

/*
=================================================
	constructor
=================================================
*/
	VResourceManager::VResourceManager (const VDevice &dev) :
		_device{ dev }
	{
		STATIC_ASSERT( GfxResourceID::MaxIndex() <= DepsPool_t::capacity() );
		STATIC_ASSERT( GfxResourceID::MaxIndex() <= BufferPool_t::capacity() );
		STATIC_ASSERT( GfxResourceID::MaxIndex() <= ImagePool_t::capacity() );
		//STATIC_ASSERT( GfxResourceID::MaxIndex() <= VirtBufferPool_t::capacity() );
		//STATIC_ASSERT( GfxResourceID::MaxIndex() <= VirtImagePool_t::capacity() );
	}
	
/*
=================================================
	destructor
=================================================
*/
	VResourceManager::~VResourceManager ()
	{
		CHECK( not _defaultAllocator );
	}
	
/*
=================================================
	Initialize
=================================================
*/
	bool  VResourceManager::Initialize ()
	{
		CHECK_ERR( _CreateEmptyDescriptorSetLayout() );
		CHECK_ERR( _CreateDefaultSampler() );

		_defaultAllocator = MakeShared< VUniMemAllocator >( _device );

		return true;
	}
	
/*
=================================================
	ResourceDestructor
=================================================
*/
	struct VResourceManager::ResourceDestructor
	{
		VResourceManager&	res;

		template <typename T, uint I>
		void operator () ()
		{
			auto&	pool = res._GetResourcePool( T{} );
			pool.Release();
		}
	};
	
/*
=================================================
	Deinitialize
=================================================
*/
	void  VResourceManager::Deinitialize ()
	{
		if ( _defaultAllocator )
		{
			CHECK( _defaultAllocator.use_count() == 1 );
			_defaultAllocator = null;
		}

		ReleaseResource( _defaultSampler );
		ReleaseResource( _emptyDSLayout );

		AllResourceIDs_t::Visit( ResourceDestructor{*this} );
		
		_semaphorePool.Clear( [this](VkSemaphore &sem) { _device.vkDestroySemaphore( _device.GetVkDevice(), sem, null ); });
		_fencePool.Clear( [this](VkFence &fence) { _device.vkDestroyFence( _device.GetVkDevice(), fence, null ); });
	}
	
/*
=================================================
	IsResourceAlive
=================================================
*/
	bool  VResourceManager::IsResourceAlive (GfxResourceID id) const
	{
		using EType = GfxResourceID::EType;
		
		BEGIN_ENUM_CHECKS();
		switch ( id.ResourceType() )
		{
			case EType::Unknown :				return IsAlive( VDependencyID{ id.Index(), id.Generation() });
			case EType::Buffer :				return IsAlive( VBufferID{ id.Index(), id.Generation() });
			case EType::Image :					return IsAlive( VImageID{ id.Index(), id.Generation() });
			case EType::RayTracingGeometry :	break; //return IsAlive( VRayTracingGeometryID{ id.Index(), id.Generation() });
			case EType::RayTracingScene :		break; //return IsAlive( VRayTracingSceneID{ id.Index(), id.Generation() });
			case EType::VirtualBuffer :			return IsAlive( VVirtualBufferID{ id.Index(), id.Generation() });
			case EType::VirtualImage :			return IsAlive( VVirtualImageID{ id.Index(), id.Generation() });
			case EType::_Count :				break;
		}
		END_ENUM_CHECKS();
		RETURN_ERR( "unknown Gfx resource type" );
	}

/*
=================================================
	ReleaseResource
=================================================
*/
	bool  VResourceManager::ReleaseResource (UniqueID<GfxResourceID> &res)
	{
		using EType = GfxResourceID::EType;

		GfxResourceID	id = res.Release();

		BEGIN_ENUM_CHECKS();
		switch ( id.ResourceType() )
		{
			case EType::Unknown :				return _ReleaseResource( VDependencyID{ id.Index(), id.Generation() }) == 0;
			case EType::Buffer :				return _ReleaseResource( VBufferID{ id.Index(), id.Generation() }) == 0;
			case EType::Image :					return _ReleaseResource( VImageID{ id.Index(), id.Generation() }) == 0;
			case EType::RayTracingGeometry :	break; //return _ReleaseResource( VRayTracingGeometryID{ id.Index(), id.Generation() }) == 0;
			case EType::RayTracingScene :		break; //return _ReleaseResource( VRayTracingSceneID{ id.Index(), id.Generation() }) == 0;
			case EType::VirtualBuffer :			return _ReleaseResource( VVirtualBufferID{ id.Index(), id.Generation() }) == 0;
			case EType::VirtualImage :			return _ReleaseResource( VVirtualImageID{ id.Index(), id.Generation() }) == 0;
			case EType::_Count :				break;
		}
		END_ENUM_CHECKS();
		RETURN_ERR( "unknown Gfx resource type" );
	}
	
/*
=================================================
	GetBufferDescription
=================================================
*/
	BufferDesc const&  VResourceManager::GetBufferDescription (GfxResourceID id) const
	{
		CHECK_ERR( id.ResourceType() == GfxResourceID::EType::Buffer, _dummyBufferDesc );

		return GetDescription( VBufferID{ id.Index(), id.Generation() });
	}
	
/*
=================================================
	GetImageDescription
=================================================
*/
	ImageDesc const&  VResourceManager::GetImageDescription (GfxResourceID id) const
	{
		CHECK_ERR( id.ResourceType() == GfxResourceID::EType::Image, _dummyImageDesc );

		return GetDescription( VImageID{ id.Index(), id.Generation() });
	}

/*
=================================================
	GetBufferHandle
=================================================
*/
	VResourceManager::NativeBufferHandle_t  VResourceManager::GetBufferHandle (GfxResourceID id) const
	{
		CHECK_ERR( id.ResourceType() == GfxResourceID::EType::Buffer );

		auto*	buf = GetResource( VBufferID{ id.Index(), id.Generation() });
		CHECK_ERR( buf );

		return BitCast<BufferVk_t>(buf->Handle());
	}

/*
=================================================
	GetImageHandle
=================================================
*/
	VResourceManager::NativeImageHandle_t  VResourceManager::GetImageHandle (GfxResourceID id) const
	{
		CHECK_ERR( id.ResourceType() == GfxResourceID::EType::Image );

		auto*	img = GetResource( VImageID{ id.Index(), id.Generation() });
		CHECK_ERR( img );

		return BitCast<ImageVk_t>(img->Handle());
	}
		
/*
=================================================
	IsSupported
=================================================
*/
	bool  VResourceManager::IsSupported (const BufferDesc &desc) const
	{
		return VBuffer::IsSupported( _device, desc );
	}

	bool  VResourceManager::IsSupported (const ImageDesc &desc) const
	{
		return VImage::IsSupported( _device, desc );
	}
	
	bool  VResourceManager::IsSupported (GfxResourceID buffer, const BufferViewDesc &desc) const
	{
		CHECK_ERR( buffer.ResourceType() == GfxResourceID::EType::Buffer );

		auto*	buf = GetResource( VBufferID{ buffer.Index(), buffer.Generation() });
		CHECK_ERR( buf );

		return buf->IsSupported( _device, desc );
	}
	
	bool  VResourceManager::IsSupported (GfxResourceID image, const ImageViewDesc &desc) const
	{
		CHECK_ERR( image.ResourceType() == GfxResourceID::EType::Image );

		auto*	img = GetResource( VImageID{ image.Index(), image.Generation() });
		CHECK_ERR( img );

		return img->IsSupported( _device, desc );
	}
	
/*
=================================================
	_ChooseAllocator
=================================================
*/
	GfxMemAllocatorPtr  VResourceManager::_ChooseAllocator (const GfxMemAllocatorPtr &userDefined)
	{
		if ( userDefined )
			return userDefined;

		// TODO: more allocators
		return _defaultAllocator;
	}
	
/*
=================================================
	CreateFence
=================================================
*/
	VkFence  VResourceManager::CreateFence ()
	{
		VkFence	fence = VK_NULL_HANDLE;

		if ( _fencePool.Extract( OUT fence ))
			return fence;

		VkFenceCreateInfo	info = {};
		info.sType	= VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

		VK_CHECK( _device.vkCreateFence( _device.GetVkDevice(), &info, null, OUT &fence ));
		return fence;
	}

/*
=================================================
	ReleaseFences
=================================================
*/
	void  VResourceManager::ReleaseFences (ArrayView<VkFence> fences)
	{
		VK_CALL( _device.vkResetFences( _device.GetVkDevice(), uint(fences.size()), fences.data() ));

		for (auto& fence : fences)
		{
			if ( not _fencePool.Put( fence ))
			{
				_device.vkDestroyFence( _device.GetVkDevice(), fence, null );
			}
		}
	}
	
/*
=================================================
	ReleaseSemaphores
=================================================
*/
	void  VResourceManager::ReleaseSemaphores (ArrayView<VkSemaphore> semaphores)
	{
		for (auto& sem : semaphores)
		{
			if ( not _semaphorePool.Put( sem ))
			{
				_device.vkDestroySemaphore( _device.GetVkDevice(), sem, null );
			}
		}
	}
//-----------------------------------------------------------------------------


/*
=================================================
	CreateDependency
=================================================
*/
	UniqueID<GfxResourceID>  VResourceManager::CreateDependency (StringView dbgName)
	{
		VDependencyID	id;
		CHECK_ERR( _Assign( OUT id ));

		auto&	data = _GetResourcePool( id )[ id.Index() ];
		Replace( data );

		if ( not data.Create( dbgName ))
		{
			_Unassign( id );
			RETURN_ERR( "failed when creating dependency" );
		}
		
		data.AddRef();
		return UniqueID<GfxResourceID>{ GfxResourceID{ id.Index(), id.Generation(), GfxResourceID::EType::Unknown }};
	}
	
/*
=================================================
	CreateImage
=================================================
*/
	UniqueID<GfxResourceID>  VResourceManager::CreateImage (const VirtualImageDesc &, StringView)
	{
		return Default;
	}
	
/*
=================================================
	CreateBuffer
=================================================
*/
	UniqueID<GfxResourceID>  VResourceManager::CreateBuffer (const VirtualBufferDesc &, StringView)
	{
		return Default;
	}

/*
=================================================
	CreateImage
=================================================
*/
	UniqueID<GfxResourceID>  VResourceManager::CreateImage (const ImageDesc &desc, EResourceState defaultState, StringView dbgName, const GfxMemAllocatorPtr &allocator)
	{
		VImageID	id;
		CHECK_ERR( _Assign( OUT id ));

		auto&	data = _GetResourcePool( id )[ id.Index() ];
		Replace( data );

		if ( not data.Create( *this, desc, _ChooseAllocator( allocator ), defaultState, dbgName ))
		{
			_Unassign( id );
			RETURN_ERR( "failed when creating image" );
		}
		
		data.AddRef();

		return UniqueID<GfxResourceID>{ GfxResourceID{ id.Index(), id.Generation(), GfxResourceID::EType::Image }};
	}
	
/*
=================================================
	CreateBuffer
=================================================
*/
	UniqueID<GfxResourceID>  VResourceManager::CreateBuffer (const BufferDesc &desc, StringView dbgName, const GfxMemAllocatorPtr &allocator)
	{
		VBufferID	id;
		CHECK_ERR( _Assign( OUT id ));

		auto&	data = _GetResourcePool( id )[ id.Index() ];
		Replace( data );
		
		if ( not data.Create( *this, desc, _ChooseAllocator( allocator ), dbgName ))
		{
			_Unassign( id );
			RETURN_ERR( "failed when creating buffer" );
		}
		
		data.AddRef();

		return UniqueID<GfxResourceID>{ GfxResourceID{ id.Index(), id.Generation(), GfxResourceID::EType::Buffer }};
	}
	
/*
=================================================
	_CreateCachedResource
=================================================
*/
	template <typename ID, typename FnInitialize, typename FnCreate>
	inline UniqueID<ID>  VResourceManager::_CreateCachedResource (StringView errorStr, FnInitialize&& fnInit, FnCreate&& fnCreate)
	{
		ID	id;
		CHECK_ERR( _Assign( OUT id ));

		auto&	pool	= _GetResourcePool( id );
		auto&	data	= pool[ id.Index() ];
		fnInit( INOUT data );
		
		// search in cache
		Index_t	temp_id		= pool.Find( &data );
		bool	is_created	= false;

		if ( temp_id == UMax )
		{
			// create new
			if ( not fnCreate( data ))
			{
				_Unassign( id );
				RETURN_ERR( errorStr );
			}

			data.AddRef();
			is_created = true;
			
			// try to add to cache
			temp_id = pool.AddToCache( id.Index() ).first;
		}

		if ( temp_id == id.Index() )
			return UniqueID<ID>{ id };

		// use already cached resource
		auto&	temp = pool[ temp_id ];
		temp.AddRef();

		if ( is_created )
			data.Destroy( *this );
		
		_Unassign( id );

		return UniqueID<ID>{ ID{ temp_id, temp.GetGeneration() }};
	}
	
/*
=================================================
	CreateRenderPass
=================================================
*/
	UniqueID<RenderPassID>  VResourceManager::CreateRenderPass (ArrayView<VLogicalRenderPass*> logicalPasses, StringView dbgName)
	{
		return _CreateCachedResource<RenderPassID>( "failed when creating render pass",
										[&] (auto& data) { return Replace( data, logicalPasses ); },
										[&] (auto& data) { return data.Create( _device, dbgName ); });
	}
	
	UniqueID<VFramebufferID>  VResourceManager::CreateFramebuffer (ArrayView<Pair<VImageID, ImageViewDesc>> attachments,
																	RenderPassID rp, uint2 dim, uint layers, StringView dbgName)
	{
		return _CreateCachedResource<VFramebufferID>( "failed when creating framebuffer",
										[&] (auto& data) {
											return Replace( data, attachments, AcquireResource(rp), dim, layers );
										},
										[&] (auto& data) {
											if ( data.Create( *this, dbgName )) {
												//_validation.createdFramebuffers.fetch_add( 1, memory_order_relaxed );	// TODO
												return true;
											}
											return false;
										});
	}
//-----------------------------------------------------------------------------

	
/*
=================================================
	LoadSamplerPack
=================================================
*/
	bool  VResourceManager::LoadSamplerPack (const SharedPtr<RStream> &stream)
	{
		VSamplerPackID	id;
		CHECK_ERR( _Assign( OUT id ));

		auto&	data = _GetResourcePool( id )[ id.Index() ];
		Replace( data );
		
		if ( not data.Create( *this, stream, INOUT _samplerRefs ))
		{
			_Unassign( id );
			RETURN_ERR( "failed when loading sampler pack" );
		}
		
		data.AddRef();
		return true;
	}
	
/*
=================================================
	CreateSampler
=================================================
*/
	UniqueID<VSamplerID>  VResourceManager::CreateSampler (const VkSamplerCreateInfo &info, StringView dbgName)
	{
		VSamplerID	id;
		CHECK_ERR( _Assign( OUT id ));

		auto&	data = _GetResourcePool( id )[ id.Index() ];
		Replace( data );
		
		if ( not data.Create( GetDevice(), info, dbgName ))
		{
			_Unassign( id );
			RETURN_ERR( "failed when creating sampler" );
		}
		
		data.AddRef();
		return UniqueID<VSamplerID>{ id };
	}
	
/*
=================================================
	GetSampler
=================================================
*/
	VSamplerID  VResourceManager::GetSampler (const SamplerName &name) const
	{
		SHAREDLOCK( _samplerRefs.guard );

		auto	iter = _samplerRefs.map.find( name );
		CHECK_ERR( iter != _samplerRefs.map.end() );

		return iter->second;
	}
	
/*
=================================================
	GetVkSampler
=================================================
*/
	VkSampler  VResourceManager::GetVkSampler (const SamplerName &name) const
	{
		auto*	res = GetResource( GetSampler( name ));

		if ( not res )
			res = GetResource( _defaultSampler );

		return res->Handle();
	}

/*
=================================================
	_CreateDefaultSampler
=================================================
*/
	bool  VResourceManager::_CreateDefaultSampler ()
	{
		CHECK_ERR( not _defaultSampler );

		VkSamplerCreateInfo		info = {};
		info.sType					= VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		info.flags					= 0;
		info.magFilter				= VK_FILTER_NEAREST;
		info.minFilter				= VK_FILTER_NEAREST;
		info.mipmapMode				= VK_SAMPLER_MIPMAP_MODE_NEAREST;
		info.addressModeU			= VK_SAMPLER_ADDRESS_MODE_REPEAT;
		info.addressModeV			= VK_SAMPLER_ADDRESS_MODE_REPEAT;
		info.addressModeW			= VK_SAMPLER_ADDRESS_MODE_REPEAT;
		info.mipLodBias				= 0.0f;
		info.anisotropyEnable		= VK_FALSE;
		info.maxAnisotropy			= 0.0f;
		info.compareEnable			= VK_FALSE;
		info.compareOp				= VK_COMPARE_OP_NEVER;
		info.minLod					= 0.0f;
		info.maxLod					= 0.0f;
		info.borderColor			= VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
		info.unnormalizedCoordinates= VK_FALSE;
		
		_defaultSampler = CreateSampler( info );
		CHECK_ERR( _defaultSampler );

		return true;
	}
//-----------------------------------------------------------------------------



namespace {
/*
=================================================
	SetShaderStages
=================================================
*/
	void SetShaderStages (OUT VkPipelineShaderStageCreateInfo const*			&outStages,
						  OUT uint												&outCount,
						  ArrayView<VGraphicsPipelineTemplate::ShaderModule>	shaders,
						  LinearAllocator<>										&allocator)
	{
		auto*	stages	= allocator.Alloc<VkPipelineShaderStageCreateInfo>( 5 );
		outStages		= stages;
		outCount		= 0;

		for (auto& sh : shaders)
		{
			ASSERT( sh.module );

			VkPipelineShaderStageCreateInfo&	info = stages[outCount++];
			info.sType		= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			info.pNext		= null;
			info.flags		= 0;
			info.module		= sh.module;
			info.pName		= "main";		// TODO
			info.stage		= sh.stage;
			info.pSpecializationInfo = null;
		}
	}

/*
=================================================
	SetDynamicState
=================================================
*/
	void SetDynamicState (OUT VkPipelineDynamicStateCreateInfo	&outState,
						  EPipelineDynamicState					inState,
						  LinearAllocator<>						&allocator)
	{
		constexpr uint	max_size = CT_IntLog2<uint(EPipelineDynamicState::_Last)> + 1;

		auto*	states = allocator.Alloc<VkDynamicState>( max_size );

		outState.sType				= VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		outState.pNext				= null;
		outState.flags				= 0;
		outState.dynamicStateCount	= 0;
		outState.pDynamicStates		= states;

		for (EPipelineDynamicState t = EPipelineDynamicState(1 << 0);
			 t < EPipelineDynamicState::_Last;
			 t = EPipelineDynamicState(uint(t) << 1))
		{
			if ( not EnumEq( inState, t ))
				continue;

			*(states++) = VEnumCast( t );
			++outState.dynamicStateCount;
		}

		ASSERT( outState.dynamicStateCount <= max_size );
	}
	
/*
=================================================
	SetMultisampleState
=================================================
*/
	void SetMultisampleState (OUT VkPipelineMultisampleStateCreateInfo	&outState,
							  const RenderState::MultisampleState		&inState)
	{
		outState.sType					= VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		outState.pNext					= null;
		outState.flags					= 0;
		outState.rasterizationSamples	= VEnumCast( inState.samples );
		outState.sampleShadingEnable	= inState.sampleShading;
		outState.minSampleShading		= inState.minSampleShading;
		outState.pSampleMask			= inState.samples.IsEnabled() ? inState.sampleMask.data() : null;
		outState.alphaToCoverageEnable	= inState.alphaToCoverage;
		outState.alphaToOneEnable		= inState.alphaToOne;
	}
	
/*
=================================================
	SetTessellationState
=================================================
*/
	void SetTessellationState (OUT VkPipelineTessellationStateCreateInfo &outState,
							   uint patchSize)
	{
		outState.sType				= VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
		outState.pNext				= null;
		outState.flags				= 0;
		outState.patchControlPoints	= patchSize;
	}
	
/*
=================================================
	SetStencilOpState
=================================================
*/
	void SetStencilOpState (OUT VkStencilOpState				&outState,
							const RenderState::StencilFaceState	&inState)
	{
		outState.failOp			= VEnumCast( inState.failOp );
		outState.passOp			= VEnumCast( inState.passOp );
		outState.depthFailOp	= VEnumCast( inState.depthFailOp );
		outState.compareOp		= VEnumCast( inState.compareOp );
		outState.compareMask	= inState.compareMask;
		outState.writeMask		= inState.writeMask;
		outState.reference		= inState.reference;
	}

/*
=================================================
	SetDepthStencilState
=================================================
*/
	void SetDepthStencilState (OUT VkPipelineDepthStencilStateCreateInfo	&outState,
							   const RenderState::DepthBufferState			&depth,
							   const RenderState::StencilBufferState		&stencil)
	{
		outState.sType					= VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		outState.pNext					= null;
		outState.flags					= 0;

		// depth
		outState.depthTestEnable		= depth.test;
		outState.depthWriteEnable		= depth.write;
		outState.depthCompareOp			= VEnumCast( depth.compareOp );
		outState.depthBoundsTestEnable	= depth.boundsEnabled;
		outState.minDepthBounds			= depth.bounds.x;
		outState.maxDepthBounds			= depth.bounds.y;
		
		// stencil
		outState.stencilTestEnable		= stencil.enabled;
		SetStencilOpState( OUT outState.front, stencil.front );
		SetStencilOpState( OUT outState.back,  stencil.back  );
	}
	
/*
=================================================
	SetRasterizationState
=================================================
*/
	void SetRasterizationState (OUT VkPipelineRasterizationStateCreateInfo	&outState,
								const RenderState::RasterizationState		&inState)
	{
		outState.sType						= VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		outState.pNext						= null;
		outState.flags						= 0;
		outState.polygonMode				= VEnumCast( inState.polygonMode );
		outState.lineWidth					= inState.lineWidth;
		outState.depthBiasConstantFactor	= inState.depthBiasConstFactor;
		outState.depthBiasClamp				= inState.depthBiasClamp;
		outState.depthBiasSlopeFactor		= inState.depthBiasSlopeFactor;
		outState.depthBiasEnable			= inState.depthBias;
		outState.depthClampEnable			= inState.depthClamp;
		outState.rasterizerDiscardEnable	= inState.rasterizerDiscard;
		outState.frontFace					= inState.frontFaceCCW ? VK_FRONT_FACE_COUNTER_CLOCKWISE : VK_FRONT_FACE_CLOCKWISE;
		outState.cullMode					= VEnumCast( inState.cullMode );
	}

/*
=================================================
	SetupPipelineInputAssemblyState
=================================================
*/
	void SetupPipelineInputAssemblyState (OUT VkPipelineInputAssemblyStateCreateInfo	&outState,
										  const RenderState::InputAssemblyState			&inState)
	{
		outState.sType					= VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		outState.pNext					= null;
		outState.flags					= 0;
		outState.topology				= VEnumCast( inState.topology );
		outState.primitiveRestartEnable	= inState.primitiveRestart;
	}

/*
=================================================
	SetVertexInputState
=================================================
*/
	void SetVertexInputState (OUT VkPipelineVertexInputStateCreateInfo	&outState,
							  const VertexInputState					&inState,
							  LinearAllocator<>							&allocator)
	{
		auto*	input_attrib	= allocator.Alloc<VkVertexInputAttributeDescription>( inState.Vertices().size() );
		auto*	buffer_binding	= allocator.Alloc<VkVertexInputBindingDescription>( inState.BufferBindings().size() );

		outState.sType	= VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		outState.pNext	= null;
		outState.flags	= 0;

		outState.pVertexAttributeDescriptions		= input_attrib;
		outState.vertexAttributeDescriptionCount	= uint(inState.Vertices().size());
		outState.pVertexBindingDescriptions			= buffer_binding;
		outState.vertexBindingDescriptionCount		= uint(inState.BufferBindings().size());

		for (auto& src : inState.Vertices())
		{
			ASSERT( src.second.index != UMax );

			VkVertexInputAttributeDescription&	dst = *(input_attrib++);
			dst.binding		= src.second.bufferBinding;
			dst.format		= VEnumCast( src.second.type );
			dst.location	= src.second.index;
			dst.offset		= uint(src.second.offset);
		}

		for (auto& src : inState.BufferBindings())
		{
			VkVertexInputBindingDescription&	dst = *(buffer_binding++);
			dst.binding		= src.second.index;
			dst.inputRate	= VEnumCast( src.second.rate );
			dst.stride		= uint(src.second.stride);
		}

		// if pipeline has attributes then buffer binding must be defined too
		CHECK( inState.Vertices().empty() == inState.BufferBindings().empty() );
	}

/*
=================================================
	SetViewportState
=================================================
*/
	void SetViewportState (OUT VkPipelineViewportStateCreateInfo	&outState,
						   const uint2								&viewportSize,
						   const uint								viewportCount,
						   EPipelineDynamicState					dynamicStates,
						   LinearAllocator<>						&allocator)
	{
		outState.sType			= VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		outState.pNext			= null;
		outState.flags			= 0;
		outState.viewportCount	= viewportCount;
		outState.scissorCount	= viewportCount;

		if ( EnumEq( dynamicStates, EPipelineDynamicState::Viewport ) and EnumEq( dynamicStates, EPipelineDynamicState::Scissor ) )
			return;
		
		auto*	viewports	= allocator.Alloc<VkViewport>( viewportCount );
		auto*	scissors	= allocator.Alloc<VkRect2D>( viewportCount );

		for (uint i = 0; i < viewportCount; ++i)
		{
			viewports[i] = VkViewport{ 0, 0, float(viewportSize.x), float(viewportSize.y), 0.0f, 1.0f };
			scissors[i]	 = VkRect2D{ VkOffset2D{ 0, 0 }, VkExtent2D{ viewportSize.x, viewportSize.y }};
		}

		outState.pViewports	= EnumEq( dynamicStates, EPipelineDynamicState::Viewport ) ? null : viewports;
		outState.pScissors	= EnumEq( dynamicStates, EPipelineDynamicState::Scissor ) ? null : scissors;
	}

/*
=================================================
	SetColorBlendAttachmentState
=================================================
*/
	void SetColorBlendAttachmentState (OUT VkPipelineColorBlendAttachmentState &outState,
									   const RenderState::ColorBuffer &inState,
									   const bool logicOpEnabled)
	{
		outState.blendEnable			= logicOpEnabled ? false : inState.blend;
		outState.srcColorBlendFactor	= VEnumCast( inState.srcBlendFactor.color );
		outState.srcAlphaBlendFactor	= VEnumCast( inState.srcBlendFactor.alpha );
		outState.dstColorBlendFactor	= VEnumCast( inState.dstBlendFactor.color );
		outState.dstAlphaBlendFactor	= VEnumCast( inState.dstBlendFactor.alpha );
		outState.colorBlendOp			= VEnumCast( inState.blendOp.color );
		outState.alphaBlendOp			= VEnumCast( inState.blendOp.alpha );
		outState.colorWriteMask			= (inState.colorMask.x ? VK_COLOR_COMPONENT_R_BIT : 0) |
										  (inState.colorMask.y ? VK_COLOR_COMPONENT_G_BIT : 0) |
										  (inState.colorMask.z ? VK_COLOR_COMPONENT_B_BIT : 0) |
										  (inState.colorMask.w ? VK_COLOR_COMPONENT_A_BIT : 0);
	}

/*
=================================================
	SetColorBlendState
=================================================
*/
	void SetColorBlendState (OUT VkPipelineColorBlendStateCreateInfo	&outState,
							 const RenderState::ColorBuffersState		&inState,
							 const VRenderPass							&renderPass,
							 const uint									subpassIndex,
							 LinearAllocator<>							&allocator)
	{
		ASSERT( subpassIndex < renderPass.GetCreateInfo().subpassCount );

		const bool		logic_op_enabled	= ( inState.logicOp != ELogicOp::None );
		const auto&		subpass				= renderPass.GetCreateInfo().pSubpasses[ subpassIndex ];
		auto*			attachments			= allocator.Alloc<VkPipelineColorBlendAttachmentState>( subpass.colorAttachmentCount );

		for (size_t i = 0; i < subpass.colorAttachmentCount; ++i)
		{
			VkPipelineColorBlendAttachmentState&	color_state = attachments[i];
			color_state.colorWriteMask	= VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
										  VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		}

		for (size_t i = 0, cnt = Min(inState.buffers.size(), subpass.colorAttachmentCount); i < cnt; ++i)
		{
			SetColorBlendAttachmentState( OUT attachments[i], inState.buffers[i], logic_op_enabled );
		}

		outState.sType				= VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		outState.pNext				= null;
		outState.flags				= 0;
		outState.attachmentCount	= subpass.colorAttachmentCount;
		outState.pAttachments		= subpass.colorAttachmentCount ? attachments : null;
		outState.logicOpEnable		= logic_op_enabled;
		outState.logicOp			= logic_op_enabled ? VEnumCast( inState.logicOp ) : VK_LOGIC_OP_CLEAR;

		outState.blendConstants[0] = inState.blendColor.r;
		outState.blendConstants[1] = inState.blendColor.g;
		outState.blendConstants[2] = inState.blendColor.b;
		outState.blendConstants[3] = inState.blendColor.a;
	}

/*
=================================================
	ValidateRenderState
=================================================
*/
	void ValidateRenderState (const VDevice &dev, INOUT RenderState &renderState, INOUT EPipelineDynamicState &dynamicStates)
	{
		if ( renderState.rasterization.rasterizerDiscard )
		{
			renderState.color	= Default;
			renderState.depth	= Default;
			renderState.stencil	= Default;
			dynamicStates		= dynamicStates & ~(EPipelineDynamicState::RasterizerMask);
		}

		// reset to default state if dynamic state enabled.
		// it is needed for correct hash calculation.
		for (EPipelineDynamicState t = EPipelineDynamicState(1 << 0);
			 t < EPipelineDynamicState::_Last;
			 t = EPipelineDynamicState(uint(t) << 1)) 
		{
			if ( not EnumEq( dynamicStates, t ) )
				continue;

			BEGIN_ENUM_CHECKS();
			switch ( t )
			{
				case EPipelineDynamicState::Viewport :
					break;
				case EPipelineDynamicState::Scissor :
					break;

				//case EPipelineDynamicState::LineWidth :
				//	renderState.rasterization.lineWidth = 1.0f;
				//	break;

				//case EPipelineDynamicState::DepthBias :
				//	ASSERT( renderState.rasterization.depthBias );
				//	renderState.rasterization.depthBiasConstFactor	= 0.0f;
				//	renderState.rasterization.depthBiasClamp		= 0.0f;
				//	renderState.rasterization.depthBiasSlopeFactor	= 0.0f;
				//	break;

				//case EPipelineDynamicState::BlendConstants :
				//	renderState.color.blendColor = RGBA32f{ 1.0f };
				//	break;

				//case EPipelineDynamicState::DepthBounds :
				//	ASSERT( renderState.depth.boundsEnabled );
				//	renderState.depth.bounds = { 0.0f, 1.0f };
				//	break;

				case EPipelineDynamicState::StencilCompareMask :
					ASSERT( renderState.stencil.enabled ); 
					renderState.stencil.front.compareMask = UMax;
					renderState.stencil.back.compareMask  = UMax;
					break;

				case EPipelineDynamicState::StencilWriteMask :
					ASSERT( renderState.stencil.enabled ); 
					renderState.stencil.front.writeMask = UMax;
					renderState.stencil.back.writeMask  = UMax;
					break;

				case EPipelineDynamicState::StencilReference :
					ASSERT( renderState.stencil.enabled ); 
					renderState.stencil.front.reference = 0;
					renderState.stencil.back.reference  = 0;
					break;

				case EPipelineDynamicState::ShadingRatePalette :
					// do nothing
					break;

				case EPipelineDynamicState::Unknown :
				case EPipelineDynamicState::Default :
				case EPipelineDynamicState::_Last :
				case EPipelineDynamicState::All :
					break;	// to shutup warnings

				default :
					ASSERT(false);	// not supported
					break;
			}
			END_ENUM_CHECKS();
		}

		// validate color buffer states
		{
			const bool	dual_src_blend	= dev.GetProperties().features.dualSrcBlend;

			const auto	IsDualSrcBlendFactor = [] (EBlendFactor value) {
				switch ( value ) {
					case EBlendFactor::Src1Color :
					case EBlendFactor::OneMinusSrc1Color :
					case EBlendFactor::Src1Alpha :
					case EBlendFactor::OneMinusSrc1Alpha :
						return true;
				}
				return false;
			};

			for (auto& cb : renderState.color.buffers)
			{
				if ( not cb.blend )
				{	
					cb.srcBlendFactor =	 { EBlendFactor::One,	EBlendFactor::One };
					cb.dstBlendFactor	= { EBlendFactor::Zero,	EBlendFactor::Zero };
					cb.blendOp			= { EBlendOp::Add,		EBlendOp::Add };
				}
				else
				if ( not dual_src_blend )
				{
					ASSERT( not IsDualSrcBlendFactor( cb.srcBlendFactor.color ) );
					ASSERT( not IsDualSrcBlendFactor( cb.srcBlendFactor.alpha ) );
					ASSERT( not IsDualSrcBlendFactor( cb.dstBlendFactor.color ) );
					ASSERT( not IsDualSrcBlendFactor( cb.dstBlendFactor.alpha ) );
				}
			}
		}

		// validate depth states
		{
			if ( not renderState.depth.test )
				renderState.depth.compareOp = ECompareOp::LEqual;

			//if ( not renderState.depth.write )

			if ( not renderState.depth.boundsEnabled )
				renderState.depth.bounds = { 0.0f, 1.0f };
		}

		// validate stencil states
		{
			if ( not renderState.stencil.enabled )
			{
				renderState.stencil = Default;
			}
		}
	}
}

/*
=================================================
	_FindPipelineInCache
=================================================
*/
	template <typename PplnTemplType, typename DescType>
	auto  VResourceManager::_FindPipelineInCache (PplnTemplType* tmpl, const DescType &desc, HashVal descHash) const
	{
		SHAREDLOCK( tmpl->_pipelineMapGuard );

		for (auto iter = tmpl->_pipelineMap.find( descHash );
			 iter != tmpl->_pipelineMap.end() and iter->first == descHash;
			 ++iter)
		{
			auto*	rhs = GetResource( iter->second );

			if ( rhs and desc == rhs->Description() )
				return iter->second;
		}

		return (typename PplnTemplType::PipelineID_t)(Default);
	}
	
/*
=================================================
	_FindPipelineInCache
=================================================
*/
	template <typename PplnTemplType, typename DescType, typename ID>
	void  VResourceManager::_AddPipelineToCache (PplnTemplType* tmpl, const DescType &, HashVal descHash, ID id) const
	{
		EXLOCK( tmpl->_pipelineMapGuard );

		tmpl->_pipelineMap.insert({ descHash, id });
	}

/*
=================================================
	_FindPipelineTemplate
=================================================
*/
	template <typename NameType, typename ValueType, typename ID, typename PplnTemplType>
	bool  VResourceManager::_FindPipelineTemplate (const PipelineName &name, VPipelinePack::PipelineRefs::ItemsTmpl<NameType, ValueType> &items,
												   OUT UniqueID<ID> &pplnTmplId, OUT PplnTemplType* &pplnTmpl)
	{
		EXLOCK( items.guard );

		auto	iter = items.map.find( name );
		CHECK_ERR( iter != items.map.end() );

		pplnTmplId = AcquireResource( iter->second );

		pplnTmpl = GetResource( pplnTmplId );
		CHECK_ERR( pplnTmpl );

		return true;
	}

/*
=================================================
	_GetGraphicsPipeline
=================================================
*/
	GraphicsPipelineID  VResourceManager::_GetGraphicsPipeline (const PipelineName &name, const GraphicsPipelineDesc &inDesc,
																OUT UniqueID<VGraphicsPipelineTemplateID> &pplnTemplId,
																OUT UniqueID<RenderPassID> &renderPassId)
	{
		GraphicsPipelineDesc				desc		= inDesc;
		HashVal								desc_hash;
		VGraphicsPipelineTemplate const*	ppln_templ	= null;

		CHECK_ERR( _FindPipelineTemplate( name, _pipelineRefs.graphics, OUT pplnTemplId, OUT ppln_templ ));

		// validate
		{
			if ( ppln_templ->_patchControlPoints )
				desc.renderState.inputAssembly.topology = EPrimitive::Patch;

			desc.vertexInput.ApplyAttribs( ppln_templ->GetVertexAttribs() );
			ValidateRenderState( _device, INOUT desc.renderState, INOUT desc.dynamicState );
			
			// check topology
			CHECK_ERR(	uint(desc.renderState.inputAssembly.topology) < ppln_templ->_supportedTopology.size() and
						ppln_templ->_supportedTopology[uint(desc.renderState.inputAssembly.topology)] );

			desc_hash = desc.CalcHash();
		}

		if ( auto id = _FindPipelineInCache( ppln_templ, desc, desc_hash ); id )
			return id;

		// create new pipeline
		VkPipeline			ppln	= VK_NULL_HANDLE;
		VkPipelineLayout	layout	= VK_NULL_HANDLE;
		{
			renderPassId = AcquireResource( desc.renderPassId );

			auto*	render_pass = GetResource( desc.renderPassId );
			CHECK_ERR( render_pass );

			auto*	layout_res = GetResource( ppln_templ->GetLayoutID() );
			CHECK_ERR( layout_res );
			layout = layout_res->Handle();

			VkGraphicsPipelineCreateInfo			pipeline_info		= {};
			VkPipelineInputAssemblyStateCreateInfo	input_assembly_info	= {};
			VkPipelineColorBlendStateCreateInfo		blend_info			= {};
			VkPipelineDepthStencilStateCreateInfo	depth_stencil_info	= {};
			VkPipelineMultisampleStateCreateInfo	multisample_info	= {};
			VkPipelineRasterizationStateCreateInfo	rasterization_info	= {};
			VkPipelineTessellationStateCreateInfo	tessellation_info	= {};
			VkPipelineDynamicStateCreateInfo		dynamic_state_info	= {};
			VkPipelineVertexInputStateCreateInfo	vertex_input_info	= {};
			VkPipelineViewportStateCreateInfo		viewport_info		= {};
			LinearAllocator<>						allocator;			allocator.SetBlockSize( 16_Kb );

			SetShaderStages( OUT pipeline_info.pStages, OUT pipeline_info.stageCount, ppln_templ->_shaders, allocator );
			SetDynamicState( OUT dynamic_state_info, desc.dynamicState, allocator );
			SetMultisampleState( OUT multisample_info, desc.renderState.multisample );
			SetTessellationState( OUT tessellation_info, ppln_templ->_patchControlPoints );
			SetDepthStencilState( OUT depth_stencil_info, desc.renderState.depth, desc.renderState.stencil );
			SetRasterizationState( OUT rasterization_info, desc.renderState.rasterization );
			SetupPipelineInputAssemblyState( OUT input_assembly_info, desc.renderState.inputAssembly );
			SetVertexInputState( OUT vertex_input_info, desc.vertexInput, allocator );
			SetViewportState( OUT viewport_info, uint2(1024, 1024), desc.viewportCount, desc.dynamicState, allocator );
			SetColorBlendState( OUT blend_info, desc.renderState.color, *render_pass, desc.subpassIndex, allocator );

			pipeline_info.sType					= VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			pipeline_info.basePipelineIndex		= -1;
			pipeline_info.basePipelineHandle	= VK_NULL_HANDLE;
			pipeline_info.pColorBlendState		= &blend_info;
			pipeline_info.pDepthStencilState	= &depth_stencil_info;
			pipeline_info.pDynamicState			= &dynamic_state_info;
			pipeline_info.pInputAssemblyState	= &input_assembly_info;
			pipeline_info.pMultisampleState		= &multisample_info;
			pipeline_info.pRasterizationState	= &rasterization_info;
			pipeline_info.pTessellationState	= (ppln_templ->_patchControlPoints > 0 ? &tessellation_info : null);
			pipeline_info.pVertexInputState		= &vertex_input_info;
			pipeline_info.layout				= layout;
			pipeline_info.renderPass			= render_pass->Handle();
			pipeline_info.subpass				= desc.subpassIndex;

			if ( not rasterization_info.rasterizerDiscardEnable )
			{
				pipeline_info.pViewportState		= &viewport_info;
			}else{
				pipeline_info.pViewportState		= null;
				pipeline_info.pMultisampleState		= null;
				pipeline_info.pDepthStencilState	= null;
				pipeline_info.pColorBlendState		= null;
			}

			VK_CHECK( _device.vkCreateGraphicsPipelines( _device.GetVkDevice(), VK_NULL_HANDLE, 1, &pipeline_info, null, OUT &ppln ));
		}

		// create pipeline wrapper
		GraphicsPipelineID	id;
		CHECK_ERR( _Assign( OUT id ));

		auto&	data = _GetResourcePool( id )[ id.Index() ];
		Replace( data );
		
		if ( not data.Create( *pplnTemplId, desc, ppln, layout ))
		{
			_Unassign( id );
			RETURN_ERR( "failed to create graphics pipeline" );
		}

		// render pass id moved to pipeline class
		Unused( renderPassId.Release() );
		
		data.AddRef();
		
		_AddPipelineToCache( ppln_templ, desc, desc_hash, id );
		return id;
	}
	
/*
=================================================
	GetGraphicsPipeline
=================================================
*/
	GraphicsPipelineID  VResourceManager::GetGraphicsPipeline (const PipelineName &name, const GraphicsPipelineDesc &desc)
	{
		// keep strong reference to some resources to prevent deleting from another thread
		UniqueID<VGraphicsPipelineTemplateID>	ppln_templ_id;
		UniqueID<RenderPassID>					render_pass_id;

		GraphicsPipelineID	id = _GetGraphicsPipeline( name, desc, OUT ppln_templ_id, OUT render_pass_id );

		ReleaseResource( ppln_templ_id );
		ReleaseResource( render_pass_id );
		return id;
	}

/*
=================================================
	_GetMeshPipeline
=================================================
*/
	MeshPipelineID  VResourceManager::_GetMeshPipeline (const PipelineName &name, const MeshPipelineDesc &inDesc,
														UniqueID<VMeshPipelineTemplateID> &pplnTemplId,
														UniqueID<RenderPassID> &renderPassId)
	{
		MeshPipelineDesc				desc		= inDesc;
		HashVal							desc_hash;
		VMeshPipelineTemplate const*	ppln_templ	= null;
		
		CHECK_ERR( _FindPipelineTemplate( name, _pipelineRefs.mesh, OUT pplnTemplId, OUT ppln_templ ));

		// validate
		{
			ValidateRenderState( _device, INOUT desc.renderState, INOUT desc.dynamicState );

			desc.renderState.inputAssembly.topology = ppln_templ->_topology;

			desc_hash = desc.CalcHash();
		}
		
		if ( auto id = _FindPipelineInCache( ppln_templ, desc, desc_hash ); id )
			return id;

		// create new pipeline
		VkPipeline			ppln	= VK_NULL_HANDLE;
		VkPipelineLayout	layout	= VK_NULL_HANDLE;
		{
			renderPassId = AcquireResource( desc.renderPassId );

			auto*	render_pass = GetResource( desc.renderPassId );
			CHECK_ERR( render_pass );

			auto*	layout_res = GetResource( ppln_templ->GetLayoutID() );
			CHECK_ERR( layout_res );
			layout = layout_res->Handle();
			
			VkGraphicsPipelineCreateInfo			pipeline_info		= {};
			VkPipelineInputAssemblyStateCreateInfo	input_assembly_info	= {};
			VkPipelineColorBlendStateCreateInfo		blend_info			= {};
			VkPipelineDepthStencilStateCreateInfo	depth_stencil_info	= {};
			VkPipelineMultisampleStateCreateInfo	multisample_info	= {};
			VkPipelineRasterizationStateCreateInfo	rasterization_info	= {};
			VkPipelineDynamicStateCreateInfo		dynamic_state_info	= {};
			VkPipelineVertexInputStateCreateInfo	vertex_input_info	= {};
			VkPipelineViewportStateCreateInfo		viewport_info		= {};
			LinearAllocator<>						allocator;			allocator.SetBlockSize( 16_Kb );
			
			SetShaderStages( OUT pipeline_info.pStages, OUT pipeline_info.stageCount, ppln_templ->_shaders, allocator );
			SetDynamicState( OUT dynamic_state_info, desc.dynamicState, allocator );
			SetColorBlendState( OUT blend_info, desc.renderState.color, *render_pass, desc.subpassIndex, allocator );
			SetMultisampleState( OUT multisample_info, desc.renderState.multisample );
			SetDepthStencilState( OUT depth_stencil_info, desc.renderState.depth, desc.renderState.stencil );
			SetRasterizationState( OUT rasterization_info, desc.renderState.rasterization );
			SetupPipelineInputAssemblyState( OUT input_assembly_info, desc.renderState.inputAssembly );
			SetViewportState( OUT viewport_info, uint2(1024, 1024), desc.viewportCount, desc.dynamicState, allocator );

			vertex_input_info.sType	= VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

			pipeline_info.sType					= VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			pipeline_info.pNext					= null;
			pipeline_info.flags					= 0; //inst.flags;
			pipeline_info.pInputAssemblyState	= &input_assembly_info;
			pipeline_info.pRasterizationState	= &rasterization_info;
			pipeline_info.pColorBlendState		= &blend_info;
			pipeline_info.pDepthStencilState	= &depth_stencil_info;
			pipeline_info.pMultisampleState		= &multisample_info;
			pipeline_info.pVertexInputState		= &vertex_input_info;
			pipeline_info.pDynamicState			= &dynamic_state_info;
			pipeline_info.basePipelineIndex		= -1;
			pipeline_info.basePipelineHandle	= VK_NULL_HANDLE;
			pipeline_info.layout				= layout;
			pipeline_info.renderPass			= render_pass->Handle();
			pipeline_info.subpass				= desc.subpassIndex;
		
			if ( not rasterization_info.rasterizerDiscardEnable )
			{
				pipeline_info.pViewportState		= &viewport_info;
			}else{
				pipeline_info.pViewportState		= null;
				pipeline_info.pMultisampleState		= null;
				pipeline_info.pDepthStencilState	= null;
				pipeline_info.pColorBlendState		= null;
			}

			VK_CHECK( _device.vkCreateGraphicsPipelines( _device.GetVkDevice(), VK_NULL_HANDLE, 1, &pipeline_info, null, OUT &ppln ));
		}

		// create pipeline wrapper
		MeshPipelineID	id;
		CHECK_ERR( _Assign( OUT id ));

		auto&	data = _GetResourcePool( id )[ id.Index() ];
		Replace( data );
		
		if ( not data.Create( *pplnTemplId, desc, ppln, layout ))
		{
			_Unassign( id );
			RETURN_ERR( "failed to create mesh pipeline" );
		}
		
		// render pass id moved to pipeline class
		Unused( renderPassId.Release() );

		data.AddRef();
		
		_AddPipelineToCache( ppln_templ, desc, desc_hash, id );
		return id;
	}
	
/*
=================================================
	GetMeshPipeline
=================================================
*/
	MeshPipelineID  VResourceManager::GetMeshPipeline (const PipelineName &name, const MeshPipelineDesc &desc)
	{
		// keep strong reference to some resources to prevent deleting from another thread
		UniqueID<VMeshPipelineTemplateID>	ppln_templ_id;
		UniqueID<RenderPassID>				render_pass_id;

		MeshPipelineID	id = _GetMeshPipeline( name, desc, OUT ppln_templ_id, OUT render_pass_id );

		ReleaseResource( ppln_templ_id );
		ReleaseResource( render_pass_id );
		return id;
	}

/*
=================================================
	_GetComputePipeline
=================================================
*/
	ComputePipelineID  VResourceManager::_GetComputePipeline (const PipelineName &name, const ComputePipelineDesc &inDesc,
															  UniqueID<VComputePipelineTemplateID> &pplnTemplId)
	{
		ComputePipelineDesc				desc		= inDesc;
		HashVal							desc_hash;
		VComputePipelineTemplate const*	ppln_templ	= null;
		
		CHECK_ERR( _FindPipelineTemplate( name, _pipelineRefs.compute, OUT pplnTemplId, OUT ppln_templ ));

		// validate
		{
			desc.localGroupSize	= desc.localGroupSize.value_or( ppln_templ->_defaultLocalGroupSize );
			desc.localGroupSize = { ppln_templ->_localSizeSpec.x != VComputePipelineTemplate::UNDEFINED_SPECIALIZATION ? desc.localGroupSize->x : ppln_templ->_defaultLocalGroupSize.x,
									ppln_templ->_localSizeSpec.y != VComputePipelineTemplate::UNDEFINED_SPECIALIZATION ? desc.localGroupSize->y : ppln_templ->_defaultLocalGroupSize.y,
									ppln_templ->_localSizeSpec.z != VComputePipelineTemplate::UNDEFINED_SPECIALIZATION ? desc.localGroupSize->z : ppln_templ->_defaultLocalGroupSize.z };

			desc_hash = desc.CalcHash();
		}
		
		if ( auto id = _FindPipelineInCache( ppln_templ, desc, desc_hash ); id )
			return id;

		// create new pipeline
		VkPipeline			ppln	= VK_NULL_HANDLE;
		VkPipelineLayout	layout	= VK_NULL_HANDLE;
		{
			auto*	layout_res = GetResource( ppln_templ->GetLayoutID() );
			CHECK_ERR( layout_res );
			layout = layout_res->Handle();

			VkComputePipelineCreateInfo		pipeline_info = {};

			pipeline_info.sType			= VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
			pipeline_info.layout		= layout;
			pipeline_info.flags			= desc.dispatchBase ? VK_PIPELINE_CREATE_DISPATCH_BASE_KHR : 0;
			pipeline_info.stage.sType	= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			pipeline_info.stage.flags	= 0;
			pipeline_info.stage.stage	= VK_SHADER_STAGE_COMPUTE_BIT;
			pipeline_info.stage.module	= ppln_templ->_shader;
			pipeline_info.stage.pName	= "main";		// TODO

			VK_CHECK( _device.vkCreateComputePipelines( _device.GetVkDevice(), VK_NULL_HANDLE, 1, &pipeline_info, null, OUT &ppln ));
		}

		// create pipeline wrapper
		ComputePipelineID	id;
		CHECK_ERR( _Assign( OUT id ));

		auto&	data = _GetResourcePool( id )[ id.Index() ];
		Replace( data );
		
		if ( not data.Create( *pplnTemplId, desc, ppln, layout ))
		{
			_Unassign( id );
			RETURN_ERR( "failed to create compute pipeline" );
		}
		
		data.AddRef();
		
		_AddPipelineToCache( ppln_templ, desc, desc_hash, id );
		return id;
	}
	
/*
=================================================
	GetComputePipeline
=================================================
*/
	ComputePipelineID  VResourceManager::GetComputePipeline (const PipelineName &name, const ComputePipelineDesc &desc)
	{
		// keep strong reference to some resources to prevent deleting from another thread
		UniqueID<VComputePipelineTemplateID>	ppln_templ_id;

		ComputePipelineID	id = _GetComputePipeline( name, desc, OUT ppln_templ_id );

		ReleaseResource( ppln_templ_id );
		return id;
	}

/*
=================================================
	GetRayTracingPipeline
=================================================
*/
	RayTracingPipelineID  VResourceManager::GetRayTracingPipeline (const PipelineName &, const RayTracingPipelineDesc &)
	{
		return Default;
	}

/*
=================================================
	LoadPipelinePack
=================================================
*/
	UniqueID<PipelinePackID>  VResourceManager::LoadPipelinePack (const SharedPtr<RStream> &stream)
	{
		PipelinePackID	id;
		CHECK_ERR( _Assign( OUT id ));

		auto&	data = _GetResourcePool( id )[ id.Index() ];
		Replace( data );
		
		if ( not data.Create( *this, stream, INOUT _pipelineRefs ))
		{
			_Unassign( id );
			RETURN_ERR( "failed when loading pipeline pack" );
		}
		
		data.AddRef();
		return UniqueID<PipelinePackID>{ id };
	}
	
/*
=================================================
	ReloadPipelinePack
=================================================
*/
	bool  VResourceManager::ReloadPipelinePack (const SharedPtr<RStream> &, PipelinePackID)
	{
		// TODO
		return false;
	}
	
/*
=================================================
	CreateDescriptorSetLayout
=================================================
*/
	UniqueID<VDescriptorSetLayoutID>  VResourceManager::CreateDescriptorSetLayout (const VDescriptorSetLayout::Uniforms_t &uniforms,
																				   ArrayView<VkSampler> samplerStorage,
																				   StringView dbgName)
	{
		VDescriptorSetLayoutID	id;
		CHECK_ERR( _Assign( OUT id ));

		auto&	data = _GetResourcePool( id )[ id.Index() ];
		Replace( data );
		
		if ( not data.Create( GetDevice(), uniforms, samplerStorage, dbgName ))
		{
			_Unassign( id );
			RETURN_ERR( "failed when creating descriptor set layout" );
		}
		
		data.AddRef();
		return UniqueID<VDescriptorSetLayoutID>{ id };
	}
	
/*
=================================================
	_CreateEmptyDescriptorSetLayout
=================================================
*/
	bool  VResourceManager::_CreateEmptyDescriptorSetLayout ()
	{
		VDescriptorSetLayoutID	id;
		CHECK_ERR( _Assign( OUT id ));

		auto&	data = _GetResourcePool( id )[ id.Index() ];
		Replace( data );
		
		if ( not data.Create( GetDevice(), "EmptyDSLayout" ))
		{
			_Unassign( id );
			RETURN_ERR( "failed when creating empty descriptor set layout" );
		}
		
		data.AddRef();

		_emptyDSLayout.Set( id );
		return true;
	}

/*
=================================================
	CreatePipelineLayout
=================================================
*/
	UniqueID<VPipelineLayoutID>  VResourceManager::CreatePipelineLayout (const VPipelineLayout::DescriptorSets_t &descSetLayouts,
																		 const VPipelineLayout::PushConstants_t &pusConstants,
																		 StringView dbgName)
	{
		VPipelineLayoutID	id;
		CHECK_ERR( _Assign( OUT id ));

		auto&	data = _GetResourcePool( id )[ id.Index() ];
		Replace( data );
		
		auto*	empty_ds = GetResource( _emptyDSLayout );
		CHECK_ERR( empty_ds );

		if ( not data.Create( GetDevice(), descSetLayouts, pusConstants, empty_ds->Handle(), dbgName ))
		{
			_Unassign( id );
			RETURN_ERR( "failed when creating pipeline layout" );
		}
		
		data.AddRef();
		return UniqueID<VPipelineLayoutID>{ id };
	}
	
/*
=================================================
	CreateGPTemplate
=================================================
*/
	UniqueID<VGraphicsPipelineTemplateID>  VResourceManager::CreateGPTemplate (VPipelineLayoutID layoutId, VRenderPassOutputID rpOutputId,
																			   const PipelineCompiler::GraphicsPipelineDesc &desc,
																			   ArrayView<ShaderModule> modules, StringView dbgName)
	{
		VGraphicsPipelineTemplateID		id;
		CHECK_ERR( _Assign( OUT id ));

		auto&	data = _GetResourcePool( id )[ id.Index() ];
		Replace( data );
		
		auto*	empty_ds = GetResource( _emptyDSLayout );
		CHECK_ERR( empty_ds );

		if ( not data.Create( layoutId, rpOutputId, desc, modules, dbgName ))
		{
			_Unassign( id );
			RETURN_ERR( "failed when creating graphics pipeline template" );
		}
		
		data.AddRef();
		return UniqueID<VGraphicsPipelineTemplateID>{ id };
	}
	
/*
=================================================
	CreateGPTemplate
=================================================
*/
	UniqueID<VMeshPipelineTemplateID>  VResourceManager::CreateMPTemplate (VPipelineLayoutID layoutId, VRenderPassOutputID rpOutputId,
																		   const PipelineCompiler::MeshPipelineDesc &desc,
																		   ArrayView<ShaderModule> modules, StringView dbgName)
	{
		VMeshPipelineTemplateID		id;
		CHECK_ERR( _Assign( OUT id ));

		auto&	data = _GetResourcePool( id )[ id.Index() ];
		Replace( data );
		
		auto*	empty_ds = GetResource( _emptyDSLayout );
		CHECK_ERR( empty_ds );

		if ( not data.Create( layoutId, rpOutputId, desc, modules, dbgName ))
		{
			_Unassign( id );
			RETURN_ERR( "failed when creating mesh pipeline template" );
		}
		
		data.AddRef();
		return UniqueID<VMeshPipelineTemplateID>{ id };
	}
	
/*
=================================================
	CreateGPTemplate
=================================================
*/
	UniqueID<VComputePipelineTemplateID>  VResourceManager::CreateCPTemplate (VPipelineLayoutID layoutId, const PipelineCompiler::ComputePipelineDesc &desc,
																			  VkShaderModule modules, StringView dbgName)
	{
		VComputePipelineTemplateID		id;
		CHECK_ERR( _Assign( OUT id ));

		auto&	data = _GetResourcePool( id )[ id.Index() ];
		Replace( data );
		
		auto*	empty_ds = GetResource( _emptyDSLayout );
		CHECK_ERR( empty_ds );

		if ( not data.Create( layoutId, desc, modules, dbgName ))
		{
			_Unassign( id );
			RETURN_ERR( "failed when creating compute pipeline template" );
		}
		
		data.AddRef();
		return UniqueID<VComputePipelineTemplateID>{ id };
	}
	
/*
=================================================
	CreateRenderPassOutput
=================================================
*/
	UniqueID<VRenderPassOutputID>  VResourceManager::CreateRenderPassOutput (const VRenderPassOutput::Output_t &fragOutput)
	{
		VRenderPassOutputID		id;
		CHECK_ERR( _Assign( OUT id ));

		auto&	data = _GetResourcePool( id )[ id.Index() ];
		Replace( data );

		if ( not data.Create( fragOutput ))
		{
			_Unassign( id );
			RETURN_ERR( "failed when creating render pass output" );
		}
		
		data.AddRef();
		return UniqueID<VRenderPassOutputID>{ id };
	}
	
/*
=================================================
	CreateRenderPassOutput
=================================================
*/
	VRenderPassOutputID  VResourceManager::GetRenderPassOutput (const RenderPassName &name) const
	{
		SHAREDLOCK( _pipelineRefs.renderPassNames.guard );

		auto	iter = _pipelineRefs.renderPassNames.map.find( name );
		
		if ( iter == _pipelineRefs.renderPassNames.map.end() )
			return Default;

		return iter->second;
	}


}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
