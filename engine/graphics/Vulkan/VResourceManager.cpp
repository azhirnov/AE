// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#ifdef AE_ENABLE_VULKAN

# include "graphics/Vulkan/VResourceManager.h"
# include "graphics/Vulkan/VEnumCast.h"
# include "graphics/Vulkan/Allocators/VUniMemAllocator.h"

# include "graphics/Private/DescriptorSetHelper.h"

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
		Reconstruct<ResType>( target.Data(), FwdArg<Args>( args )... );
	}
}
//-----------------------------------------------------------------------------

	

/*
=================================================
	constructor
=================================================
*/
	VResourceManagerImpl::VResourceManagerImpl (const VDevice &dev) :
		_device{ dev },
		_descriptorMngr{ *this },
		_stagingMngr{ *this }
	{
		//STATIC_ASSERT( BufferID::MaxIndex() <= BufferPool_t::capacity() );
		//STATIC_ASSERT( ImageID::MaxIndex() <= ImagePool_t::capacity() );
	}
	
/*
=================================================
	destructor
=================================================
*/
	VResourceManagerImpl::~VResourceManagerImpl ()
	{
		CHECK( not _defaultAllocator );
	}
	
/*
=================================================
	Initialize
=================================================
*/
	bool  VResourceManagerImpl::Initialize (const GraphicsCreateInfo &info)
	{
		CHECK_ERR( _CreateEmptyDescriptorSetLayout() );
		CHECK_ERR( _CreateDefaultSampler() );
		CHECK_ERR( _stagingMngr.Initialize( info ));

		_defaultAllocator = MakeRC< VUniMemAllocator >( _device );

		return true;
	}

/*
=================================================
	ResourceDestructor
=================================================
*/
	struct VResourceManagerImpl::ResourceDestructor
	{
		VResourceManagerImpl&	res;

		template <typename T, uint I>
		void operator () ()
		{
			auto&	pool = res._GetResourcePool( T{} );
			pool.Release();
		}
	};
	
/*
=================================================
	_DestroyResources
=================================================
*/
	template <typename DataT, usize CS, usize MC>
	inline void  VResourceManagerImpl::_DestroyResources (INOUT PoolTmpl<DataT,CS,MC> &pool)
	{
		for (usize i = 0, count = pool.size(); i < count; ++i)
		{
			Index_t	id	 = Index_t(i);
			auto&	data = pool[id];
				
			if ( data.IsCreated() )
			{
				data.Destroy( *this );
				pool.Unassign( id );
			}
		}
	}

/*
=================================================
	_DestroyResourceCache
=================================================
*/
	template <typename DataT, usize CS, usize MC>
	inline void  VResourceManagerImpl::_DestroyResourceCache (INOUT CachedPoolTmpl<DataT,CS,MC> &pool)
	{
		for (usize i = 0, count = pool.size(); i < count; ++i)
		{
			Index_t	id	 = Index_t(i);
			auto&	data = pool[id];
				
			if ( data.IsCreated() )
			{
				pool.RemoveFromCache( id );
				data.Destroy( *this );
				pool.Unassign( id );
			}
		}
	}

/*
=================================================
	Deinitialize
=================================================
*/
	void  VResourceManagerImpl::Deinitialize ()
	{
		_stagingMngr.Deinitialize();

		if ( _defaultAllocator )
		{
			CHECK( _defaultAllocator.use_count() == 1 );
			_defaultAllocator = null;
		}

		ReleaseResource( _defaultSampler );
		ReleaseResource( _emptyDSLayout );
		
		_DestroyResources( INOUT _resPool.samplerPacks );
		_DestroyResources( INOUT _resPool.samplers );
		_DestroyResources( INOUT _resPool.graphicsPpln );
		_DestroyResources( INOUT _resPool.computePpln );
		_DestroyResources( INOUT _resPool.meshPpln );
		_DestroyResources( INOUT _resPool.dsLayouts );
		_DestroyResources( INOUT _resPool.pplnLayouts );
		_DestroyResourceCache( INOUT _resPool.renderPassCache );
		_DestroyResourceCache( INOUT _resPool.framebufferCache );
		_DestroyResourceCache( INOUT _resPool.descSetCache );

		_descriptorMngr.Deinitialize();

		const auto	ClearRefs = [] (auto& refs) {
			EXLOCK( refs.guard );
			refs.map.clear();
		};

		ClearRefs( _samplerRefs );
		ClearRefs( _pipelineRefs.compute );
		ClearRefs( _pipelineRefs.graphics );
		ClearRefs( _pipelineRefs.mesh );
		ClearRefs( _pipelineRefs.renderPassNames );

		AllResourceIDs_t::Visit( ResourceDestructor{*this} );
		
		_semaphorePool.Clear( [this](VkSemaphore &sem) { _device.vkDestroySemaphore( _device.GetVkDevice(), sem, null ); });
		_fencePool.Clear( [this](VkFence &fence) { _device.vkDestroyFence( _device.GetVkDevice(), fence, null ); });
	}
	
/*
=================================================
	Get***Description
=================================================
*/
	BufferDesc const&  VResourceManagerImpl::GetBufferDescription (BufferID id) const
	{
		return GetDescription( id );
	}

	ImageDesc const&  VResourceManagerImpl::GetImageDescription (ImageID id) const
	{
		return GetDescription( id );
	}

/*
=================================================
	Get***Handle
=================================================
*/
	VkBuffer  VResourceManagerImpl::GetBufferHandle (BufferID id) const
	{
		auto*	buf = GetResource( id );
		CHECK_ERR( buf, VK_NULL_HANDLE );

		return buf->Handle();
	}

	VkImage  VResourceManagerImpl::GetImageHandle (ImageID id) const
	{
		auto*	img = GetResource( id );
		CHECK_ERR( img, VK_NULL_HANDLE );

		return img->Handle();
	}
		
/*
=================================================
	GetPipelineNativeDesc
=================================================
*/
	template <typename ID>
	VulkanPipelineInfo  VResourceManagerImpl::_GetPipelineNativeDesc (ID id) const
	{
		auto*	ppln = GetResource( id );
		CHECK_ERR( ppln );

		auto*	ppln_templ = GetResource( ppln->TemplateID() );
		CHECK_ERR( ppln_templ );

		auto*	ppln_lay = GetResource( ppln_templ->GetLayoutID() );
		CHECK_ERR( ppln_lay );

		VulkanPipelineInfo	info;

		ppln_lay->GetNativeDesc( OUT info );
		info.pipeline = ppln->Handle();

		return info;
	}

	VulkanPipelineInfo  VResourceManagerImpl::GetPipelineNativeDesc (GraphicsPipelineID id) const
	{
		return _GetPipelineNativeDesc( id );
	}
	
	VulkanPipelineInfo  VResourceManagerImpl::GetPipelineNativeDesc (MeshPipelineID id) const
	{
		return _GetPipelineNativeDesc( id );
	}
	
	VulkanPipelineInfo  VResourceManagerImpl::GetPipelineNativeDesc (ComputePipelineID id) const
	{
		return _GetPipelineNativeDesc( id );
	}
	
	VulkanPipelineInfo  VResourceManagerImpl::GetPipelineNativeDesc (RayTracingPipelineID id) const
	{
		Unused( id );
		//return _GetPipelineNativeDesc( id );
		return Default;
	}
	
/*
=================================================
	GetMemoryInfo
=================================================
*/
	bool  VResourceManagerImpl::GetMemoryInfo (ImageID id, OUT VulkanMemoryObjInfo &info) const
	{
		auto*	image = GetResource( id );
		CHECK_ERR( image );

		return image->GetMemoryInfo( OUT info );
	}

	bool  VResourceManagerImpl::GetMemoryInfo (BufferID id, OUT VulkanMemoryObjInfo &info) const
	{
		auto*	buffer = GetResource( id );
		CHECK_ERR( buffer );

		return buffer->GetMemoryInfo( OUT info );
	}

/*
=================================================
	IsSupported
=================================================
*/
	bool  VResourceManagerImpl::IsSupported (const BufferDesc &desc) const
	{
		return VBuffer::IsSupported( _device, desc );
	}

	bool  VResourceManagerImpl::IsSupported (const ImageDesc &desc) const
	{
		return VImage::IsSupported( _device, desc );
	}
	
	bool  VResourceManagerImpl::IsSupported (BufferID buffer, const BufferViewDesc &desc) const
	{
		auto*	buf = GetResource( buffer );
		CHECK_ERR( buf );

		return buf->IsSupported( _device, desc );
	}
	
	bool  VResourceManagerImpl::IsSupported (ImageID image, const ImageViewDesc &desc) const
	{
		auto*	img = GetResource( image );
		CHECK_ERR( img );

		return img->IsSupported( _device, desc );
	}
	
/*
=================================================
	_ChooseAllocator
=================================================
*/
	VMemAllocatorPtr  VResourceManagerImpl::_ChooseAllocator (const VMemAllocatorPtr &userDefined)
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
	VkFence  VResourceManagerImpl::CreateFence ()
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
	CreateSemaphore
=================================================
*/
	VkSemaphore  VResourceManagerImpl::CreateSemaphore ()
	{
		VkSemaphore	sem = VK_NULL_HANDLE;

		if ( _semaphorePool.Extract( OUT sem ))
			return sem;

		VkSemaphoreCreateInfo	info = {};
		info.sType	= VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
			
		VK_CHECK( _device.vkCreateSemaphore( _device.GetVkDevice(), &info, null, &sem ));
		return sem;
	}

/*
=================================================
	ReleaseFences
=================================================
*/
	void  VResourceManagerImpl::ReleaseFences (ArrayView<VkFence> fences)
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
	void  VResourceManagerImpl::ReleaseSemaphores (ArrayView<VkSemaphore> semaphores)
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
	CreateImage
=================================================
*/
	UniqueID<ImageID>  VResourceManagerImpl::CreateImage (const ImageDesc &desc, EResourceState defaultState, StringView dbgName, const VMemAllocatorPtr &allocator)
	{
		ImageID	id;
		CHECK_ERR( _Assign( OUT id ));

		auto&	data = _GetResourcePool( id )[ id.Index() ];
		Replace( data );

		if ( not data.Create( *this, desc, _ChooseAllocator( allocator ), defaultState, dbgName ))
		{
			_Unassign( id );
			RETURN_ERR( "failed when creating image" );
		}
		
		data.AddRef();

		return UniqueID<ImageID>{ id };
	}
	
/*
=================================================
	CreateBuffer
=================================================
*/
	UniqueID<BufferID>  VResourceManagerImpl::CreateBuffer (const BufferDesc &desc, StringView dbgName, const VMemAllocatorPtr &allocator)
	{
		BufferID	id;
		CHECK_ERR( _Assign( OUT id ));

		auto&	data = _GetResourcePool( id )[ id.Index() ];
		Replace( data );
		
		if ( not data.Create( *this, desc, _ChooseAllocator( allocator ), dbgName ))
		{
			_Unassign( id );
			RETURN_ERR( "failed when creating buffer" );
		}
		
		data.AddRef();

		return UniqueID<BufferID>{ id };
	}
	
/*
=================================================
	CreateImage
=================================================
*/
	UniqueID<ImageID>  VResourceManagerImpl::CreateImage (const VulkanImageDesc &desc, StringView dbgName)
	{
		ImageID	id;
		CHECK_ERR( _Assign( OUT id ));

		auto&	data = _GetResourcePool( id )[ id.Index() ];
		Replace( data );

		if ( not data.Create( _device, desc, dbgName ))
		{
			_Unassign( id );
			RETURN_ERR( "failed when creating image" );
		}
		
		data.AddRef();

		return UniqueID<ImageID>{ id };
	}
	
/*
=================================================
	CreateBuffer
=================================================
*/
	UniqueID<BufferID>  VResourceManagerImpl::CreateBuffer (const VulkanBufferDesc &desc, StringView dbgName)
	{
		BufferID	id;
		CHECK_ERR( _Assign( OUT id ));

		auto&	data = _GetResourcePool( id )[ id.Index() ];
		Replace( data );
		
		if ( not data.Create( _device, desc, dbgName ))
		{
			_Unassign( id );
			RETURN_ERR( "failed when creating buffer" );
		}
		
		data.AddRef();

		return UniqueID<BufferID>{ id };
	}
	
/*
=================================================
	_InitPipelineResources
=================================================
*/
	template <typename PplnID>
	bool  VResourceManagerImpl::_InitPipelineResources (const PplnID &pplnId, const DescriptorSetName &name, OUT DescriptorSet &ds) const
	{
		auto*	ppln = GetResource( pplnId );
		CHECK_ERR( ppln );

		auto*	ppln_templ = GetResource( ppln->TemplateID() );
		CHECK_ERR( ppln_templ );

		auto*	ppln_layout = GetResource( ppln_templ->GetLayoutID() );
		CHECK_ERR( ppln_layout );

		DescriptorSetLayoutID	layout_id;
		uint					binding;

		if ( not ppln_layout->GetDescriptorSetLayout( name, OUT layout_id, OUT binding ))
			return false;

		VDescriptorSetLayout const*	ds_layout = GetResource( layout_id );
		CHECK_ERR( ds_layout );

		CHECK_ERR( DescriptorSetHelper::Initialize( OUT ds, layout_id, binding, ds_layout->GetDescriptorTemplate() ));
		return true;
	}

/*
=================================================
	InitializeDescriptorSet
=================================================
*/
	bool  VResourceManagerImpl::InitializeDescriptorSet (GraphicsPipelineID ppln, const DescriptorSetName &name, OUT DescriptorSet &ds) const
	{
		return _InitPipelineResources( ppln, name, OUT ds );
	}

	bool  VResourceManagerImpl::InitializeDescriptorSet (MeshPipelineID ppln, const DescriptorSetName &name, OUT DescriptorSet &ds) const
	{
		return _InitPipelineResources( ppln, name, OUT ds );
	}

	bool  VResourceManagerImpl::InitializeDescriptorSet (ComputePipelineID ppln, const DescriptorSetName &name, OUT DescriptorSet &ds) const
	{
		return _InitPipelineResources( ppln, name, OUT ds );
	}

	bool  VResourceManagerImpl::InitializeDescriptorSet (RayTracingPipelineID ppln, const DescriptorSetName &name, OUT DescriptorSet &ds) const
	{
		Unused( ppln, name, ds );
		return false;
		//return _InitPipelineResources( ppln, name, OUT ds );
	}
	
/*
=================================================
	InitializeDescriptorSet
=================================================
*/
	bool  VResourceManagerImpl::InitializeDescriptorSet (const PipelineName &pplnName, const DescriptorSetName &dsName, OUT DescriptorSet &ds) const
	{
		const auto	InitDS = [this] (const DescriptorSetName &dsName, VPipelineLayoutID pplnLayout, OUT DescriptorSet &ds) -> bool
		{
			auto*	ppln_layout = GetResource( pplnLayout );
			CHECK_ERR( ppln_layout );

			DescriptorSetLayoutID	layout_id;
			uint					binding;

			if ( not ppln_layout->GetDescriptorSetLayout( dsName, OUT layout_id, OUT binding ))
				return false;

			VDescriptorSetLayout const*	ds_layout = GetResource( layout_id );
			CHECK_ERR( ds_layout );

			CHECK_ERR( DescriptorSetHelper::Initialize( OUT ds, layout_id, binding, ds_layout->GetDescriptorTemplate() ));
			return true;
		};

		// search in graphics pipelines
		{
			EXLOCK( _pipelineRefs.graphics.guard );

			auto	iter = _pipelineRefs.graphics.map.find( pplnName );
			if ( iter != _pipelineRefs.graphics.map.end() )
			{
				auto*	ppln_templ = GetResource( iter->second );
				CHECK_ERR( ppln_templ );

				return InitDS( dsName, ppln_templ->GetLayoutID(), OUT ds );
			}
		}

		// search in mesh pipelines
		{
			EXLOCK( _pipelineRefs.mesh.guard );

			auto	iter = _pipelineRefs.mesh.map.find( pplnName );
			if ( iter != _pipelineRefs.mesh.map.end() )
			{
				auto*	ppln_templ = GetResource( iter->second );
				CHECK_ERR( ppln_templ );

				return InitDS( dsName, ppln_templ->GetLayoutID(), OUT ds );
			}
		}

		// search in compute pipelines
		{
			EXLOCK( _pipelineRefs.compute.guard );

			auto	iter = _pipelineRefs.compute.map.find( pplnName );
			if ( iter != _pipelineRefs.compute.map.end() )
			{
				auto*	ppln_templ = GetResource( iter->second );
				CHECK_ERR( ppln_templ );

				return InitDS( dsName, ppln_templ->GetLayoutID(), OUT ds );
			}
		}

		RETURN_ERR( "can't find pipeline" );
	}

/*
=================================================
	_CreateCachedResource
=================================================
*/
	template <typename ID, typename FnInitialize, typename FnCreate>
	inline UniqueID<ID>  VResourceManagerImpl::_CreateCachedResource (StringView errorStr, FnInitialize&& fnInit, FnCreate&& fnCreate)
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
	UniqueID<RenderPassID>  VResourceManagerImpl::CreateRenderPass (ArrayView<RenderPassDesc> passes, StringView dbgName)
	{
		return _CreateCachedResource<RenderPassID>( "failed when creating render pass",
										[&] (auto& data) { return Replace( data, *this, passes ); },
										[&] (auto& data) { return data.Create( _device, dbgName ); });
	}
	
/*
=================================================
	CreateFramebuffer
=================================================
*/
	UniqueID<VFramebufferID>  VResourceManagerImpl::CreateFramebuffer (ArrayView<Pair<ImageID, ImageViewDesc>> attachments, RenderPassID rp, uint2 dim, uint layers, StringView dbgName)
	{
		return _CreateCachedResource<VFramebufferID>( "failed when creating framebuffer",
										[&] (auto& data) {
											return Replace( data, attachments, rp, dim, layers );
										},
										[&] (auto& data) {
											if ( data.Create( *this, dbgName )) {
												_validation.createdFramebuffers.fetch_add( 1, EMemoryOrder::Relaxed );
												return true;
											}
											return false;
										});
	}
	
/*
=================================================
	CreateDescriptorSet
=================================================
*/
	DescriptorSetID  VResourceManagerImpl::CreateDescriptorSet (const DescriptorSet &desc)
	{
		CHECK_ERR( desc.IsInitialized() );
	
		auto*	layout = GetResource( desc.GetLayout() );
		CHECK_ERR( layout );

		return _CreateCachedResource<DescriptorSetID>( "failed when creating descriptor set",
								   [&] (auto& data) { return Replace( data, desc ); },
								   [&] (auto& data) {
										if (data.Create( *this, *layout, False{"not quiet"} )) {
											_validation.createdDescriptorSets.fetch_add( 1, EMemoryOrder::Relaxed );
											return true;
										}
										return false;
									}).Release();
	}
//-----------------------------------------------------------------------------

	
/*
=================================================
	LoadSamplerPack
=================================================
*/
	bool  VResourceManagerImpl::LoadSamplerPack (const RC<RStream> &stream)
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
	UniqueID<VSamplerID>  VResourceManagerImpl::CreateSampler (const VkSamplerCreateInfo &info, StringView dbgName)
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
	VSamplerID  VResourceManagerImpl::GetSampler (const SamplerName &name) const
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
	VkSampler  VResourceManagerImpl::GetVkSampler (const SamplerName &name) const
	{
		auto*	res = GetResource( GetSampler( name ));

		if ( not res )
			res = GetResource( _defaultSampler );

		return res->Handle();
	}
	
/*
=================================================
	IsAlive
=================================================
*/
	bool  VResourceManagerImpl::IsAlive (const SamplerName &name) const
	{
		SHAREDLOCK( _samplerRefs.guard );

		auto	iter = _samplerRefs.map.find( name );
		if ( iter == _samplerRefs.map.end() )
			return false;

		return IsAlive( iter->second );
	}
	
/*
=================================================
	IsAlive
=================================================
*/
	bool  VResourceManagerImpl::IsAlive (DescriptorSetID id, bool recursive) const
	{
		ASSERT( id );
		
		auto*	ds = GetResource( id, False{"dont inc ref"}, True{"quiet"} );
		if ( not ds )
			return false;

		if ( not recursive )
			return true;

		return ds->IsAllResourcesAlive( *this );
	}

/*
=================================================
	_CreateDefaultSampler
=================================================
*/
	bool  VResourceManagerImpl::_CreateDefaultSampler ()
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
	using SpecConstants_t = PipelineCompiler::SpirvShaderCode::SpecConstants_t;

/*
=================================================
	AddLocalGroupSizeSpecialization
=================================================
*/
	static void  AddSpecialization (OUT VkSpecializationInfo const*				&outSpecInfo,
									const GraphicsPipelineDesc::SpecValues_t	&specData1,
									const GraphicsPipelineDesc::SpecValues_t	&specData2,
									const SpecConstants_t						&specRef,
									LinearAllocator<>							&allocator)
	{
		using UsedBits_t = BitSet< 64 >;

		auto*		entry_arr		= allocator.Alloc<VkSpecializationMapEntry>( specRef.size() );
		auto*		data_arr		= allocator.Alloc<uint>( specRef.size() );
		uint		entriy_count	= 0;
		uint		data_count		= 0;
		UsedBits_t	used;

		const auto	Process = [&] (const GraphicsPipelineDesc::SpecValues_t &specData)
		{
			for (auto&[name, data] : specData)
			{
				auto	iter = specRef.find( name );
				if ( iter == specRef.end() )
					continue;

				if ( used[iter->second] )
					continue;

				used[iter->second] = true;
				
				VkSpecializationMapEntry&	entry = entry_arr[entriy_count++];
				entry.constantID	= iter->second;
				entry.offset		= data_count * sizeof(*data_arr);
				entry.size			= sizeof(uint);

				data_arr[data_count++] = data;
			}
		};

		outSpecInfo = null;

		Process( specData1 );
		Process( specData2 );

		if ( entriy_count and data_count )
		{
			auto&	spec = *allocator.Alloc<VkSpecializationInfo>();
			spec.mapEntryCount	= entriy_count;
			spec.pMapEntries	= entry_arr;
			spec.dataSize		= data_count * sizeof(*data_arr);
			spec.pData			= data_arr;

			outSpecInfo = &spec;
		}
	}

/*
=================================================
	SetShaderStages
=================================================
*/
	static void  SetShaderStages (OUT VkPipelineShaderStageCreateInfo const*			&outStages,
								  OUT uint												&outCount,
								  ArrayView<VGraphicsPipelineTemplate::ShaderModule>	shaders,
								  const GraphicsPipelineDesc::SpecValues_t				&specData1,
								  const GraphicsPipelineDesc::SpecValues_t				&specData2,
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

			if ( specData1.size() or specData2.size() )
			{
				CHECK( sh.spec != null );
				if ( sh.spec )
					AddSpecialization( OUT info.pSpecializationInfo, specData1, specData2, *sh.spec, allocator );
			}
		}
	}

/*
=================================================
	SetDynamicState
=================================================
*/
	static void  SetDynamicState (OUT VkPipelineDynamicStateCreateInfo	&outState,
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
			if ( not AllBits( inState, t ))
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
	static void  SetMultisampleState (OUT VkPipelineMultisampleStateCreateInfo	&outState,
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
	static void  SetTessellationState (OUT VkPipelineTessellationStateCreateInfo &outState,
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
	static void  SetStencilOpState (OUT VkStencilOpState				&outState,
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
	static void  SetDepthStencilState (OUT VkPipelineDepthStencilStateCreateInfo	&outState,
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
	static void  SetRasterizationState (OUT VkPipelineRasterizationStateCreateInfo	&outState,
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
	static void  SetupPipelineInputAssemblyState (OUT VkPipelineInputAssemblyStateCreateInfo	&outState,
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
	static void  SetVertexInputState (OUT VkPipelineVertexInputStateCreateInfo	&outState,
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
	static void  SetViewportState (OUT VkPipelineViewportStateCreateInfo	&outState,
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

		if ( AllBits( dynamicStates, EPipelineDynamicState::Viewport ) and AllBits( dynamicStates, EPipelineDynamicState::Scissor ))
			return;
		
		auto*	viewports	= allocator.Alloc<VkViewport>( viewportCount );
		auto*	scissors	= allocator.Alloc<VkRect2D>( viewportCount );

		for (uint i = 0; i < viewportCount; ++i)
		{
			viewports[i] = VkViewport{ 0, 0, float(viewportSize.x), float(viewportSize.y), 0.0f, 1.0f };
			scissors[i]	 = VkRect2D{ VkOffset2D{ 0, 0 }, VkExtent2D{ viewportSize.x, viewportSize.y }};
		}

		outState.pViewports	= AllBits( dynamicStates, EPipelineDynamicState::Viewport ) ? null : viewports;
		outState.pScissors	= AllBits( dynamicStates, EPipelineDynamicState::Scissor ) ? null : scissors;
	}

/*
=================================================
	SetColorBlendAttachmentState
=================================================
*/
	static void  SetColorBlendAttachmentState (OUT VkPipelineColorBlendAttachmentState &outState,
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
	static void  SetColorBlendState (OUT VkPipelineColorBlendStateCreateInfo	&outState,
									 const RenderState::ColorBuffersState		&inState,
									 const VRenderPass							&renderPass,
									 const uint									subpassIndex,
									 LinearAllocator<>							&allocator)
	{
		ASSERT( subpassIndex < renderPass.GetCreateInfo().subpassCount );

		const bool		logic_op_enabled	= ( inState.logicOp != ELogicOp::None );
		const auto&		subpass				= renderPass.GetCreateInfo().pSubpasses[ subpassIndex ];
		auto*			attachments			= allocator.Alloc<VkPipelineColorBlendAttachmentState>( subpass.colorAttachmentCount );

		for (usize i = 0; i < subpass.colorAttachmentCount; ++i)
		{
			VkPipelineColorBlendAttachmentState&	color_state = attachments[i];
			color_state.colorWriteMask	= VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
										  VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		}

		for (usize i = 0, cnt = Min(inState.buffers.size(), subpass.colorAttachmentCount); i < cnt; ++i)
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
	static void  ValidateRenderState (const VDevice &dev, INOUT RenderState &renderState, INOUT EPipelineDynamicState &dynamicStates)
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
			if ( not AllBits( dynamicStates, t ))
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
					cb.srcBlendFactor	= { EBlendFactor::One,	EBlendFactor::One };
					cb.dstBlendFactor	= { EBlendFactor::Zero,	EBlendFactor::Zero };
					cb.blendOp			= { EBlendOp::Add,		EBlendOp::Add };
				}
				else
				if ( not dual_src_blend )
				{
					ASSERT( not IsDualSrcBlendFactor( cb.srcBlendFactor.color ));
					ASSERT( not IsDualSrcBlendFactor( cb.srcBlendFactor.alpha ));
					ASSERT( not IsDualSrcBlendFactor( cb.dstBlendFactor.color ));
					ASSERT( not IsDualSrcBlendFactor( cb.dstBlendFactor.alpha ));
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
	
/*
=================================================
	AddLocalGroupSizeSpecialization
=================================================
*/
	template <typename SpecEntries, typename SpecData>
	static void  AddSpecialization (INOUT SpecEntries											&outEntries,
									INOUT SpecData												&outEntryData,
									const GraphicsPipelineDesc::SpecValues_t					&specData1,
									const GraphicsPipelineDesc::SpecValues_t					&specData2,
									const PipelineCompiler::SpirvShaderCode::SpecConstants_t	&specRef)
	{
		const auto	Process = [&outEntries, &outEntryData, &specRef] (const GraphicsPipelineDesc::SpecValues_t &specData)
		{
			for (auto&[name, data] : specData)
			{
				auto	iter = specRef.find( name );
				if ( iter == specRef.end() )
					continue;
				
				VkSpecializationMapEntry	entry;
				entry.constantID	= iter->second;
				entry.offset		= uint(ArraySizeOf( outEntryData ));
				entry.size			= sizeof(uint);
				outEntries.push_back( entry );
				outEntryData.push_back( data );
			}
		};

		Process( specData1 );
		Process( specData2 );
	}

/*
=================================================
	AddLocalGroupSizeSpecialization
=================================================
*/
	template <typename SpecEntries, typename SpecData>
	static void  AddLocalGroupSizeSpecialization (INOUT SpecEntries	&outEntries,
												  INOUT SpecData	&outEntryData,
												  const uint3		&localSizeSpec,
												  const uint3		&localGroupSize)
	{
		const bool3	has_spec = (localSizeSpec != uint3(~0u));

		if ( has_spec.x )
		{
			VkSpecializationMapEntry	entry;
			entry.constantID	= localSizeSpec.x;
			entry.offset		= uint(ArraySizeOf( outEntryData ));
			entry.size			= sizeof(uint);
			outEntries.push_back( entry );
			outEntryData.push_back( localGroupSize.x );
		}
		
		if ( has_spec.y )
		{
			VkSpecializationMapEntry	entry;
			entry.constantID	= localSizeSpec.y;
			entry.offset		= uint(ArraySizeOf( outEntryData ));
			entry.size			= sizeof(uint);
			outEntries.push_back( entry );
			outEntryData.push_back( localGroupSize.y );
		}
		
		if ( has_spec.z )
		{
			VkSpecializationMapEntry	entry;
			entry.constantID	= localSizeSpec.z;
			entry.offset		= uint(ArraySizeOf( outEntryData ));
			entry.size			= sizeof(uint);
			outEntries.push_back( entry );
			outEntryData.push_back( localGroupSize.z );
		}
	}

}	// namesoace
//-----------------------------------------------------------------------------


/*
=================================================
	_FindPipelineInCache
=================================================
*/
	template <typename PplnTemplType, typename DescType>
	auto  VResourceManagerImpl::_FindPipelineInCache (PplnTemplType* tmpl, const DescType &desc, HashVal descHash) const
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
	void  VResourceManagerImpl::_AddPipelineToCache (PplnTemplType* tmpl, const DescType &, HashVal descHash, ID id) const
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
	bool  VResourceManagerImpl::_FindPipelineTemplate (const PipelineName &name, VPipelinePack::PipelineRefs::ItemsTmpl<NameType, ValueType> &items,
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
	GraphicsPipelineID  VResourceManagerImpl::_GetGraphicsPipeline (const PipelineName &name, const GraphicsPipelineDesc &inDesc,
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
			
			SetShaderStages( OUT pipeline_info.pStages, OUT pipeline_info.stageCount, ppln_templ->_shaders, ppln_templ->_specialization, desc.specialization, allocator );
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
	GraphicsPipelineID  VResourceManagerImpl::GetGraphicsPipeline (const PipelineName &name, const GraphicsPipelineDesc &desc)
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
	MeshPipelineID  VResourceManagerImpl::_GetMeshPipeline (const PipelineName &name, const MeshPipelineDesc &inDesc,
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
			
			SetShaderStages( OUT pipeline_info.pStages, OUT pipeline_info.stageCount, ppln_templ->_shaders, ppln_templ->_specialization, desc.specialization, allocator );
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
	MeshPipelineID  VResourceManagerImpl::GetMeshPipeline (const PipelineName &name, const MeshPipelineDesc &desc)
	{
		// keep strong reference to some resources to prevent deleting from another thread
		UniqueID<VMeshPipelineTemplateID>	ppln_templ_id;
		UniqueID<RenderPassID>				render_pass_id;

		ASSERT( not desc.hasMeshGroupSize );	// TODO
		ASSERT( not desc.hasTaskGroupSize );

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
	ComputePipelineID  VResourceManagerImpl::_GetComputePipeline (const PipelineName &name, const ComputePipelineDesc &inDesc,
															  UniqueID<VComputePipelineTemplateID> &pplnTemplId)
	{
		using SpecEntries_t	= FixedArray< VkSpecializationMapEntry, ComputePipelineDesc::SpecValues_t::capacity()*2 + 3 >;
		using SpecData_t	= FixedArray< uint, ComputePipelineDesc::SpecValues_t::capacity()*2 + 3 >;

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

			VkSpecializationInfo			spec = {};
			VkComputePipelineCreateInfo		pipeline_info = {};

			pipeline_info.sType			= VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
			pipeline_info.layout		= layout;
			pipeline_info.flags			= desc.dispatchBase ? VK_PIPELINE_CREATE_DISPATCH_BASE_KHR : 0;
			pipeline_info.stage.sType	= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			pipeline_info.stage.flags	= 0;
			pipeline_info.stage.stage	= VK_SHADER_STAGE_COMPUTE_BIT;
			pipeline_info.stage.module	= ppln_templ->_shader.module;
			pipeline_info.stage.pName	= "main";		// TODO
			
			SpecEntries_t	spec_entries;
			SpecData_t		spec_data;
			AddLocalGroupSizeSpecialization( INOUT spec_entries, INOUT spec_data, ppln_templ->_localSizeSpec, *desc.localGroupSize );

			if ( ppln_templ->_specialization.size() or desc.specialization.size() )
			{
				CHECK_ERR( ppln_templ->_shader.spec != null );
				AddSpecialization( INOUT spec_entries, INOUT spec_data, ppln_templ->_specialization, desc.specialization, *ppln_templ->_shader.spec );
			}

			if ( spec_entries.size() and spec_data.size() )
			{
				spec.mapEntryCount	= uint(spec_entries.size());
				spec.pMapEntries	= spec_entries.data();
				spec.dataSize		= usize(ArraySizeOf( spec_data ));
				spec.pData			= spec_data.data();

				pipeline_info.stage.pSpecializationInfo	= &spec;
			}

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
	ComputePipelineID  VResourceManagerImpl::GetComputePipeline (const PipelineName &name, const ComputePipelineDesc &desc)
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
	RayTracingPipelineID  VResourceManagerImpl::GetRayTracingPipeline (const PipelineName &, const RayTracingPipelineDesc &)
	{
		// TODO
		return Default;
	}

/*
=================================================
	LoadPipelinePack
=================================================
*/
	UniqueID<PipelinePackID>  VResourceManagerImpl::LoadPipelinePack (const RC<RStream> &stream)
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
	bool  VResourceManagerImpl::ReloadPipelinePack (const RC<RStream> &, PipelinePackID)
	{
		// TODO
		return false;
	}
	
/*
=================================================
	CreateDescriptorSetLayout
=================================================
*/
	UniqueID<DescriptorSetLayoutID>  VResourceManagerImpl::CreateDescriptorSetLayout (const VDescriptorSetLayout::Uniforms_t &uniforms,
																				  ArrayView<VkSampler> samplerStorage,
																				  StringView dbgName)
	{
		DescriptorSetLayoutID	id;
		CHECK_ERR( _Assign( OUT id ));

		auto&	data = _GetResourcePool( id )[ id.Index() ];
		Replace( data );
		
		if ( not data.Create( GetDevice(), uniforms, samplerStorage, dbgName ))
		{
			_Unassign( id );
			RETURN_ERR( "failed when creating descriptor set layout" );
		}
		
		data.AddRef();
		return UniqueID<DescriptorSetLayoutID>{ id };
	}
	
/*
=================================================
	_CreateEmptyDescriptorSetLayout
=================================================
*/
	bool  VResourceManagerImpl::_CreateEmptyDescriptorSetLayout ()
	{
		DescriptorSetLayoutID	id;
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
	UniqueID<VPipelineLayoutID>  VResourceManagerImpl::CreatePipelineLayout (const VPipelineLayout::DescriptorSets_t &descSetLayouts,
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
	UniqueID<VGraphicsPipelineTemplateID>  VResourceManagerImpl::CreateGPTemplate (VPipelineLayoutID layoutId, VRenderPassOutputID rpOutputId,
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
	UniqueID<VMeshPipelineTemplateID>  VResourceManagerImpl::CreateMPTemplate (VPipelineLayoutID layoutId, VRenderPassOutputID rpOutputId,
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
	UniqueID<VComputePipelineTemplateID>  VResourceManagerImpl::CreateCPTemplate (VPipelineLayoutID layoutId, const PipelineCompiler::ComputePipelineDesc &desc,
																			  const ShaderModule &module, StringView dbgName)
	{
		VComputePipelineTemplateID		id;
		CHECK_ERR( _Assign( OUT id ));

		auto&	data = _GetResourcePool( id )[ id.Index() ];
		Replace( data );
		
		auto*	empty_ds = GetResource( _emptyDSLayout );
		CHECK_ERR( empty_ds );

		if ( not data.Create( layoutId, desc, module, dbgName ))
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
	UniqueID<VRenderPassOutputID>  VResourceManagerImpl::CreateRenderPassOutput (const VRenderPassOutput::Output_t &fragOutput)
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
	VRenderPassOutputID  VResourceManagerImpl::GetRenderPassOutput (const RenderPassName &name) const
	{
		SHAREDLOCK( _pipelineRefs.renderPassNames.guard );

		auto	iter = _pipelineRefs.renderPassNames.map.find( name );
		
		if ( iter == _pipelineRefs.renderPassNames.map.end() )
			return Default;

		return iter->second;
	}
//-----------------------------------------------------------------------------


	
/*
=================================================
	RunResourceValidation
=================================================
*/
	void  VResourceManagerImpl::RunResourceValidation (uint maxIterations)
	{
		static constexpr uint	scale = MaxCached / 16;

		const auto	UpdateCounter = [] (INOUT Atomic<uint> &counter, uint maxValue) -> uint
		{
			if ( not maxValue )
				return 0;

			uint	expected = 0;
			uint	count	 = 0;
			for (; not counter.compare_exchange_weak( INOUT expected, expected - count, EMemoryOrder::Relaxed );) {
				count = Min( maxValue, expected * scale );
			}
			return count;
		};

		const auto	UpdateLastIndex = [] (INOUT Atomic<uint> &lastIndex, uint count, uint size)
		{
			uint	new_value = count;
			uint	expected  = 0;
			for (; not lastIndex.compare_exchange_weak( INOUT expected, new_value, EMemoryOrder::Relaxed );) {
				new_value = expected + count;
				new_value = new_value >= size ? new_value - size : new_value;
				ASSERT( new_value < size );
			}
			return expected;
		};

		const auto	ValidateResources = [this, &UpdateCounter, &UpdateLastIndex, maxIterations] (INOUT Atomic<uint> &counter, INOUT Atomic<uint> &lastIndex, auto& pool)
		{
			const uint	max_iter = UpdateCounter( INOUT counter, maxIterations );
			if ( max_iter )
			{
				const uint	max_count = uint(pool.size());
				const uint	last_idx  = UpdateLastIndex( INOUT lastIndex, max_iter, max_count );

				for (uint i = 0; i < max_iter; ++i)
				{
					uint	j		= last_idx + i;	j = (j >= max_count ? j - max_count : j);
					Index_t	index	= Index_t(j);

					auto&	res = pool [index];
					if ( res.IsCreated() and not res.Data().IsAllResourcesAlive( *this ))
					{
						pool.RemoveFromCache( index );
						res.Destroy( *this );
						pool.Unassign( index );
					}
				}
			}
		};

		ValidateResources( _validation.createdDescriptorSets, _validation.lastCheckedDescriptorSet, _resPool.descSetCache );
		ValidateResources( _validation.createdFramebuffers, _validation.lastCheckedFramebuffer, _resPool.framebufferCache );
	}


}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
