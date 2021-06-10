// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef AE_ENABLE_VULKAN

# include "graphics/Public/ResourceManager.h"
# include "graphics/Vulkan/VDevice.h"
//# include "graphics/Vulkan/VDescriptorManager.h"

# include "graphics/Vulkan/Resources/VBuffer.h"
# include "graphics/Vulkan/Resources/VComputePipeline.h"
# include "graphics/Vulkan/Resources/VDescriptorSet.h"
# include "graphics/Vulkan/Resources/VDescriptorSetLayout.h"
# include "graphics/Vulkan/Resources/VFramebuffer.h"
# include "graphics/Vulkan/Resources/VGraphicsPipeline.h"
# include "graphics/Vulkan/Resources/VImage.h"
# include "graphics/Vulkan/Resources/VMeshPipeline.h"
# include "graphics/Vulkan/Resources/VPipelineLayout.h"
# include "graphics/Vulkan/Resources/VPipelinePack.h"
# include "graphics/Vulkan/Resources/VResourceBase.h"
# include "graphics/Vulkan/Resources/VRenderPassOutput.h"
# include "graphics/Vulkan/Resources/VRenderPass.h"
# include "graphics/Vulkan/Resources/VSampler.h"
# include "graphics/Vulkan/Resources/VSampler.h"
# include "graphics/Vulkan/Resources/VStagingBufferManager.h"

# include "threading/Containers/LfIndexedPool.h"
# include "threading/Containers/CachedIndexedPool.h"
# include "threading/Containers/LfStaticPool.h"

namespace AE::Graphics
{

	//
	// Vulkan Resource Manager
	//

	class VResourceManagerImpl final : public VResourceManager
	{
		friend class VRenderGraph;

	// types
	public:
		using Index_t		= BufferID::Index_t;
		using Generation_t	= BufferID::Generation_t;

		template <typename T, usize ChunkSize, usize MaxChunks>
		using PoolTmpl	= Threading::IndexedPool< T, Index_t, ChunkSize, MaxChunks, UntypedAlignedAllocator >;
		
		template <typename T, usize ChunkSize, usize MaxChunks>
		using CachedPoolTmpl = Threading::CachedIndexedPool< T, Index_t, ChunkSize, MaxChunks, UntypedAlignedAllocator >;

		// chunk size
		static constexpr uint	MaxDeps			= 1u << 10;
		static constexpr uint	MaxImages		= 1u << 10;
		static constexpr uint	MaxBuffers		= 1u << 10;
		static constexpr uint	MaxMemoryObjs	= 1u << 10;
		static constexpr uint	MaxCached		= 1u << 9;
		static constexpr uint	MaxRTObjects	= 1u << 9;
		static constexpr uint	MaxPipelines	= 1u << 10;
		
		using BufferPool_t			= PoolTmpl< VResourceBase<VBuffer>,						MaxBuffers,		32 >;
		using ImagePool_t			= PoolTmpl< VResourceBase<VImage>,						MaxImages,		32 >;
		using SamplerPackPool_t		= PoolTmpl< VResourceBase<VSamplerPack>,				128,			 1 >;
		using SamplerPool_t			= PoolTmpl< VResourceBase<VSampler>,					4096,			 1 >;		// TODO: static array
		using PipelinePackPool_t	= PoolTmpl< VResourceBase<VPipelinePack>,				128,			 1 >;		// TODO: static array
		using DSLayoutPool_t		= PoolTmpl<	VResourceBase<VDescriptorSetLayout>,		MaxCached,		 8 >;
		using PplnLayoutPool_t		= PoolTmpl<	VResourceBase<VPipelineLayout>,				MaxCached,		 8 >;
		using GPipelinePool_t		= PoolTmpl< VResourceBase<VGraphicsPipeline>,			MaxPipelines,	32 >;
		using GPTemplatePool_t		= PoolTmpl< VResourceBase<VGraphicsPipelineTemplate>,	MaxCached,		 8 >;
		using CPipelinePool_t		= PoolTmpl< VResourceBase<VComputePipeline>,			MaxPipelines,	32 >;
		using CPTemplatePool_t		= PoolTmpl< VResourceBase<VComputePipelineTemplate>,	MaxCached,		 8 >;
		using MPipelinePool_t		= PoolTmpl< VResourceBase<VMeshPipeline>,				MaxPipelines,	32 >;
		using MPTemplatePool_t		= PoolTmpl< VResourceBase<VMeshPipelineTemplate>,		MaxCached,		 8 >;
		using RPOutputPool_t		= PoolTmpl< VResourceBase<VRenderPassOutput>,			MaxCached,		 8 >;
		using RenderPassPool_t		= CachedPoolTmpl< VResourceBase<VRenderPass>,			MaxCached,		 8 >;
		using FramebufferPool_t		= CachedPoolTmpl< VResourceBase<VFramebuffer>,			MaxCached,		 8 >;
		using DescSetPool_t			= CachedPoolTmpl< VResourceBase<VDescriptorSet>,		MaxCached,		 8 >;

		using PipelineRefs			= VPipelinePack::PipelineRefs;
		using SamplerRefs			= VSamplerPack::SamplerRefs;
		using ShaderModule			= VGraphicsPipelineTemplate::ShaderModule;
		
		struct ResourceDestructor;
		using AllResourceIDs_t		= TypeList< BufferID, ImageID, PipelinePackID, DescriptorSetLayoutID,
												VPipelineLayoutID, GraphicsPipelineID, VGraphicsPipelineTemplateID, ComputePipelineID,
												VComputePipelineTemplateID, MeshPipelineID, VMeshPipelineTemplateID, VSamplerPackID,
												VSamplerID, VRenderPassOutputID, RenderPassID, VFramebufferID >;
		
		using SemaphorePool_t		= Threading::LfStaticPool< VkSemaphore, 64*4 >;
		using FencePool_t			= Threading::LfStaticPool< VkFence,     64*4 >;
		
		using StagingBufferfPool_t	= Threading::LfIndexedPool< UniqueID<BufferID>, uint, 64, 4 >;


	// variables
	private:
		VDevice const&			_device;

		struct {
			BufferPool_t			buffers;
			ImagePool_t				images;
			
			SamplerPackPool_t		samplerPacks;
			SamplerPool_t			samplers;

			PipelinePackPool_t		pipelinePacks;

			DSLayoutPool_t			dsLayouts;
			PplnLayoutPool_t		pplnLayouts;

			GPipelinePool_t			graphicsPpln;
			GPTemplatePool_t		graphicsTempl;

			CPipelinePool_t			computePpln;
			CPTemplatePool_t		computeTempl;

			MPipelinePool_t			meshPpln;
			MPTemplatePool_t		meshTempl;

			RPOutputPool_t			renderPassOutputs;

			RenderPassPool_t		renderPassCache;
			FramebufferPool_t		framebufferCache;

			DescSetPool_t			descSetCache;
		}						_resPool;
		
		SamplerRefs				_samplerRefs;
		PipelineRefs			_pipelineRefs;

		SemaphorePool_t			_semaphorePool;
		FencePool_t				_fencePool;
		
		VMemAllocatorPtr		_defaultAllocator;

		VDescriptorManager		_descriptorMngr;
		VStagingBufferManager	_stagingMngr;

		// dummy resource descriptions
		struct {
			const BufferDesc		buffer;
			const ImageDesc			image;
		}						_dummyDesc;

		UniqueID<VSamplerID>			_defaultSampler;
		UniqueID<DescriptorSetLayoutID>	_emptyDSLayout;

		// cached resources validation
		struct {
			Atomic<uint>			createdFramebuffers			{0};
			Atomic<uint>			lastCheckedFramebuffer		{0};

			Atomic<uint>			createdDescriptorSets		{0};
			Atomic<uint>			lastCheckedDescriptorSet	{0};
		}						_validation;


	// methods
	private:
		explicit VResourceManagerImpl (const VDevice &);

		bool  Initialize (const GraphicsCreateInfo &);
		void  Deinitialize ();
		

	public:
		~VResourceManagerImpl ();

		bool						IsSupported (const BufferDesc &desc) const override;
		bool						IsSupported (const ImageDesc &desc) const override;
		bool						IsSupported (BufferID buffer, const BufferViewDesc &desc) const override;
		bool						IsSupported (ImageID image, const ImageViewDesc &desc) const override;

		UniqueID<ImageID>			CreateImage (const ImageDesc &desc, EResourceState defaultState, StringView dbgName, const VMemAllocatorPtr &allocator = null) override;
		UniqueID<BufferID>			CreateBuffer (const BufferDesc &desc, StringView dbgName, const VMemAllocatorPtr &allocator = null) override;
		
		UniqueID<ImageID>			CreateImage (const VulkanImageDesc &desc, StringView dbgName) override;
		UniqueID<BufferID>			CreateBuffer (const VulkanBufferDesc &desc, StringView dbgName) override;
		
		bool						InitializeDescriptorSet (GraphicsPipelineID ppln, const DescriptorSetName &name, OUT DescriptorSet &ds) const override;
		bool						InitializeDescriptorSet (MeshPipelineID ppln, const DescriptorSetName &name, OUT DescriptorSet &ds) const override;
		bool						InitializeDescriptorSet (ComputePipelineID ppln, const DescriptorSetName &name, OUT DescriptorSet &ds) const override;
		bool						InitializeDescriptorSet (RayTracingPipelineID ppln, const DescriptorSetName &name, OUT DescriptorSet &ds) const override;
		bool						InitializeDescriptorSet (const PipelineName &pplnName, const DescriptorSetName &dsName, OUT DescriptorSet &ds) const override;
		DescriptorSetID				CreateDescriptorSet (const DescriptorSet &ds) override;

		UniqueID<PipelinePackID>	LoadPipelinePack (const RC<RStream> &stream) override;
		bool						ReloadPipelinePack (const RC<RStream> &stream, PipelinePackID id) override;
		
		GraphicsPipelineID			GetGraphicsPipeline (const PipelineName &name, const GraphicsPipelineDesc &desc) override;
		MeshPipelineID				GetMeshPipeline (const PipelineName &name, const MeshPipelineDesc &desc) override;
		ComputePipelineID			GetComputePipeline (const PipelineName &name, const ComputePipelineDesc &desc) override;
		RayTracingPipelineID		GetRayTracingPipeline (const PipelineName &name, const RayTracingPipelineDesc &desc) override;

		ND_ UniqueID<DescriptorSetLayoutID>			CreateDescriptorSetLayout (const VDescriptorSetLayout::Uniforms_t &uniforms, ArrayView<VkSampler> samplerStorage, StringView dbgName = Default);
		ND_ UniqueID<VPipelineLayoutID>				CreatePipelineLayout (const VPipelineLayout::DescriptorSets_t &descSetLayouts, const VPipelineLayout::PushConstants_t &pusConstants, StringView dbgName = Default);
		ND_ UniqueID<VGraphicsPipelineTemplateID>	CreateGPTemplate (VPipelineLayoutID layoutId, VRenderPassOutputID rpOutputId, const PipelineCompiler::GraphicsPipelineDesc &desc, ArrayView<ShaderModule> modules, StringView dbgName = Default);
		ND_ UniqueID<VMeshPipelineTemplateID>		CreateMPTemplate (VPipelineLayoutID layoutId, VRenderPassOutputID rpOutputId, const PipelineCompiler::MeshPipelineDesc &desc, ArrayView<ShaderModule> modules, StringView dbgName = Default);
		ND_ UniqueID<VComputePipelineTemplateID>	CreateCPTemplate (VPipelineLayoutID layoutId, const PipelineCompiler::ComputePipelineDesc &desc, const ShaderModule &modules, StringView dbgName = Default);
		
		ND_ UniqueID<VRenderPassOutputID>	CreateRenderPassOutput (const VRenderPassOutput::Output_t &fragOutput);
		ND_ VRenderPassOutputID				GetRenderPassOutput (const RenderPassName &name) const;

		ND_ UniqueID<RenderPassID>		CreateRenderPass (ArrayView<RenderPassDesc> passes, StringView dbgName = Default);
		ND_ UniqueID<VFramebufferID>	CreateFramebuffer (ArrayView<Pair<ImageID, ImageViewDesc>> attachments, RenderPassID rp, uint2 dim, uint layers, StringView dbgName = Default);

		bool						LoadSamplerPack (const RC<RStream> &stream) override;
		ND_ UniqueID<VSamplerID>	CreateSampler (const VkSamplerCreateInfo &info, StringView dbgName = Default);
		ND_ VSamplerID				GetSampler (const SamplerName &name) const;
		ND_ VkSampler				GetVkSampler (const SamplerName &name) const;

		ND_ VkFence					CreateFence ();
		ND_ VkSemaphore				CreateSemaphore ();
		
		void	ReleaseFences (ArrayView<VkFence>);
		void	ReleaseSemaphores (ArrayView<VkSemaphore>);

		bool	ReleaseResource (UniqueID<ImageID> &id)			override	{ return _ReleaseResource( id.Release() ) == 0; }
		bool	ReleaseResource (UniqueID<BufferID> &id)		override	{ return _ReleaseResource( id.Release() ) == 0; }
		bool	ReleaseResource (UniqueID<PipelinePackID> &id)	override	{ return _ReleaseResource( id.Release() ) == 0; }
		
		template <typename IT, typename GT, uint UID>
		bool	ReleaseResource (UniqueID< HandleTmpl< IT, GT, UID >> &id, uint refCount = 1)	{ return _ReleaseResource( id.Release(), refCount ) == 0; }

		template <typename ID>
		ND_ auto const&		GetDescription (ID id) const;
		BufferDesc const&	GetBufferDescription (BufferID id) const override;
		ImageDesc const&	GetImageDescription (ImageID id) const override;

		VkBuffer			GetBufferHandle (BufferID id) const override;
		VkImage				GetImageHandle (ImageID id) const override;
		
		VulkanPipelineInfo	GetPipelineNativeDesc (GraphicsPipelineID id) const override;
		VulkanPipelineInfo	GetPipelineNativeDesc (MeshPipelineID id) const override;
		VulkanPipelineInfo	GetPipelineNativeDesc (ComputePipelineID id) const override;
		VulkanPipelineInfo	GetPipelineNativeDesc (RayTracingPipelineID id) const override;

		bool				GetMemoryInfo (ImageID id, OUT VulkanMemoryObjInfo &info) const override;
		bool				GetMemoryInfo (BufferID id, OUT VulkanMemoryObjInfo &info) const override;
		
		template <typename IT, typename GT, uint UID>
		ND_ auto			AcquireResource (HandleTmpl<IT, GT, UID> id);

		template <typename I, typename G, uint U>
		ND_ bool			IsAlive (HandleTmpl<I,G,U> id) const;
		ND_ bool			IsAlive (DescriptorSetID id, bool recursive) const;
		ND_ bool			IsAlive (const SamplerName &name) const;
		bool				IsResourceAlive (BufferID id) const override			{ return IsAlive( id ); }
		bool				IsResourceAlive (ImageID id) const override				{ return IsAlive( id ); }
		bool				IsResourceAlive (DescriptorSetID id) const override		{ return IsAlive( id, true ); }

		template <typename ID>
		ND_ auto const*		GetResource (ID id, Bool incRef = false, Bool quiet = false) const;
		
		template <typename ID>
		ND_ auto const*		GetResource (const UniqueID<ID> &id, Bool incRef = false, Bool quiet = false) const;

		ND_ VDevice const&			GetDevice ()			const	{ return _device; }
		ND_ VDescriptorManager&		GetDescriptorManager ()			{ return _descriptorMngr; }
		ND_ VStagingBufferManager&	GetStagingManager ()			{ return _stagingMngr; }
		
		void  RunResourceValidation (uint maxIterations) override;


	private:
		template <typename ID, typename FnInitialize, typename FnCreate>
		ND_ UniqueID<ID>  _CreateCachedResource (StringView errorStr, FnInitialize&& fnInit, FnCreate&& fnCreate);
		
		template <typename PplnID>
		bool  _InitPipelineResources (const PplnID &pplnId, const DescriptorSetName &name, OUT DescriptorSet &ds) const;

	// resource pool
		ND_ auto&  _GetResourcePool (const BufferID &)						{ return _resPool.buffers; }
		ND_ auto&  _GetResourcePool (const ImageID &)						{ return _resPool.images; }
		ND_ auto&  _GetResourcePool (const PipelinePackID &)				{ return _resPool.pipelinePacks; }
		ND_ auto&  _GetResourcePool (const DescriptorSetLayoutID &)			{ return _resPool.dsLayouts; }
		ND_ auto&  _GetResourcePool (const VPipelineLayoutID &)				{ return _resPool.pplnLayouts; }
		ND_ auto&  _GetResourcePool (const GraphicsPipelineID &)			{ return _resPool.graphicsPpln; }
		ND_ auto&  _GetResourcePool (const VGraphicsPipelineTemplateID &)	{ return _resPool.graphicsTempl; }
		ND_ auto&  _GetResourcePool (const ComputePipelineID &)				{ return _resPool.computePpln; }
		ND_ auto&  _GetResourcePool (const VComputePipelineTemplateID &)	{ return _resPool.computeTempl; }
		ND_ auto&  _GetResourcePool (const MeshPipelineID &)				{ return _resPool.meshPpln; }
		ND_ auto&  _GetResourcePool (const VMeshPipelineTemplateID &)		{ return _resPool.meshTempl; }
		ND_ auto&  _GetResourcePool (const VSamplerPackID &)				{ return _resPool.samplerPacks; }
		ND_ auto&  _GetResourcePool (const VSamplerID &)					{ return _resPool.samplers; }
		ND_ auto&  _GetResourcePool (const VRenderPassOutputID &)			{ return _resPool.renderPassOutputs; }
		ND_ auto&  _GetResourcePool (const RenderPassID &)					{ return _resPool.renderPassCache; }
		ND_ auto&  _GetResourcePool (const VFramebufferID &)				{ return _resPool.framebufferCache; }
		ND_ auto&  _GetResourcePool (const DescriptorSetID &)				{ return _resPool.descSetCache; }
		
		template <typename ID>
		ND_ const auto&  _GetResourceCPool (const ID &id)	const			{ return const_cast<VResourceManagerImpl *>(this)->_GetResourcePool( id ); }
		

	// memory managment
		ND_ VMemAllocatorPtr  _ChooseAllocator (const VMemAllocatorPtr &userDefined);

	// 
		template <typename ID>	ND_ bool   _Assign (OUT ID &id);
		template <typename ID>		void   _Unassign (ID id);


	// empty descriptor set layout
			bool  _CreateEmptyDescriptorSetLayout ();
		ND_ auto  _GetEmptyDescriptorSetLayout ()		{ return *_emptyDSLayout; }

			bool  _CreateDefaultSampler ();


	// pipeline creation & validation
		template <typename PplnTemplType, typename DescType>
		ND_ auto  _FindPipelineInCache (PplnTemplType* tmpl, const DescType &desc, HashVal hash) const;
		
		template <typename PplnTemplType, typename DescType, typename ID>
		void  _AddPipelineToCache (PplnTemplType* tmpl, const DescType &desc, HashVal hash, ID id) const;

		template <typename NameType, typename ValueType, typename ID, typename PplnTemplType>
		ND_ bool  _FindPipelineTemplate (const PipelineName &name, VPipelinePack::PipelineRefs::ItemsTmpl<NameType, ValueType> &items,
										 OUT UniqueID<ID> &pplnTmplId, OUT PplnTemplType* &pplnTmpl);
		
		GraphicsPipelineID	_GetGraphicsPipeline (const PipelineName &name, const GraphicsPipelineDesc &desc,
												  UniqueID<VGraphicsPipelineTemplateID> &pplnTemplId, UniqueID<RenderPassID> &renderPassId);
		MeshPipelineID		_GetMeshPipeline (const PipelineName &name, const MeshPipelineDesc &desc,
											  UniqueID<VMeshPipelineTemplateID> &pplnTemplId, UniqueID<RenderPassID> &renderPassId);
		ComputePipelineID	_GetComputePipeline (const PipelineName &name, const ComputePipelineDesc &desc,
												 UniqueID<VComputePipelineTemplateID> &pplnTemplId);
		//RayTracingPipelineID _GetRayTracingPipeline (const PipelineName &name, const RayTracingPipelineDesc &desc,
		//											 UniqueID<VRayTracingPipelineTemplateID> &pplnTemplId);

		template <typename ID>
		ND_ VulkanPipelineInfo  _GetPipelineNativeDesc (ID id) const;


	// 
		template <typename ID>
		int  _ReleaseResource (ID id, uint refCount = 1);

		template <typename DataT, usize CS, usize MC>
		int  _ReleaseResource (PoolTmpl<DataT,CS,MC> &pool, DataT& data, Index_t index, uint refCount);
		
		template <typename DataT, usize CS, usize MC>
		int  _ReleaseResource (CachedPoolTmpl<DataT,CS,MC> &pool, DataT& data, Index_t index, uint refCount);
		
		template <typename DataT, usize CS, usize MC>
		void  _DestroyResources (INOUT PoolTmpl<DataT,CS,MC> &pool);

		template <typename DataT, usize CS, usize MC>
		void  _DestroyResourceCache (INOUT CachedPoolTmpl<DataT,CS,MC> &pool);
	};

	
/*
=================================================
	GetResource
=================================================
*/
	template <typename ID>
	inline auto const*  VResourceManagerImpl::GetResource (ID id, Bool incRef, Bool quiet) const
	{
		auto&	pool = _GetResourceCPool( id );

		using Result_t = typename RemoveReference<decltype(pool)>::Value_t::Resource_t const*;

		if ( id.Index() < pool.size() )
		{
			auto&	data = pool[ id.Index() ];

			if ( data.IsCreated() and data.GetGeneration() == id.Generation() )
			{
				if ( incRef ) data.AddRef();
				return &data.Data();
			}

			Unused( quiet );
			ASSERT( quiet or data.IsCreated() );
			ASSERT( quiet or data.GetGeneration() == id.Generation() );
		}

		ASSERT( quiet and "resource index is out of range" );
		return static_cast< Result_t >(null);
	}
	
	template <typename ID>
	inline auto const*  VResourceManagerImpl::GetResource (const UniqueID<ID> &id, Bool incRef, Bool quiet) const
	{
		return GetResource( *id, incRef, quiet );
	}

/*
=================================================
	IsAlive
=================================================
*/
	template <typename I, typename G, uint U>
	inline bool  VResourceManagerImpl::IsAlive (HandleTmpl<I,G,U> id) const
	{
		ASSERT( id );
		auto&	pool = _GetResourceCPool( id );
		
		return	id.Index() < pool.size() and
				pool[id.Index()].GetGeneration() == id.Generation();
	}

/*
=================================================
	AcquireResource
=================================================
*/
	template <typename IT, typename GT, uint UID>
	inline auto  VResourceManagerImpl::AcquireResource (HandleTmpl<IT, GT, UID> id)
	{
		using Unique_t = UniqueID< HandleTmpl< IT, GT, UID >>;

		ASSERT( id );
		
		auto&	pool = _GetResourcePool( id );

		if ( id.Index() < pool.size() )
		{
			auto&	data = pool[ id.Index() ];

			if ( not data.IsCreated() or data.GetGeneration() != id.Generation() )
				return Unique_t{};

			data.AddRef();
			return Unique_t{ id };
		}

		return Unique_t{};
	}

/*
=================================================
	GetDescription
=================================================
*/
	template <>
	inline auto const&  VResourceManagerImpl::GetDescription (BufferID id) const
	{
		auto*	res = GetResource( id );
		return res ? res->Description() : _dummyDesc.buffer;
	}

	template <>
	inline auto const&  VResourceManagerImpl::GetDescription (ImageID id) const
	{
		auto*	res = GetResource( id );
		return res ? res->Description() : _dummyDesc.image;
	}

/*
=================================================
	_Assign
----
	acquire free index from cache (cache is local in thread),
	if cache empty then acquire new indices from main pool (internaly synchronized),
	if pool is full then error (false) will be returned.
=================================================
*/
	template <typename ID>
	inline bool  VResourceManagerImpl::_Assign (OUT ID &id)
	{
		auto&	pool = _GetResourcePool( id );
		
		Index_t	index = UMax;
		CHECK_ERR( pool.Assign( OUT index ));

		id = ID{ index, pool[index].GetGeneration() };
		return true;
	}
	
/*
=================================================
	_Unassign
=================================================
*/
	template <typename ID>
	inline void  VResourceManagerImpl::_Unassign (ID id)
	{
		ASSERT( id );
		auto&	pool = _GetResourcePool( id );

		pool.Unassign( id.Index() );
	}
	
/*
=================================================
	_ReleaseResource
=================================================
*/
	template <typename ID>
	inline int  VResourceManagerImpl::_ReleaseResource (ID id, uint refCount)
	{
		if ( not id )
			return -1;

		auto&	pool = _GetResourcePool( id );
		
		if ( id.Index() >= pool.size() )
			return -1;

		auto&	data = pool[ id.Index() ];
		
		if ( data.GetGeneration() != id.Generation() )
			return -1;	// this instance is already destroyed

		return _ReleaseResource( pool, data, id.Index(), refCount );
	}
	
	template <typename DataT, usize CS, usize MC>
	inline int  VResourceManagerImpl::_ReleaseResource (PoolTmpl<DataT,CS,MC> &pool, DataT& data, Index_t index, uint refCount)
	{
		int	count = data.ReleaseRef( refCount );

		if ( count == 0 and data.IsCreated() )
		{
			data.Destroy( *this );
			pool.Unassign( index );
		}
		return count;
	}
	
	template <typename DataT, usize CS, usize MC>
	inline int  VResourceManagerImpl::_ReleaseResource (CachedPoolTmpl<DataT,CS,MC> &pool, DataT& data, Index_t index, uint refCount)
	{
		int	count = data.ReleaseRef( refCount );

		if ( count == 0 and data.IsCreated() )
		{
			pool.RemoveFromCache( index );
			data.Destroy( *this );
			pool.Unassign( index );
		}
		return count;
	}


}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
