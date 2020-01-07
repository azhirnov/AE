// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef AE_ENABLE_VULKAN

# include "graphics/Public/ResourceManager.h"
# include "graphics/Vulkan/VDevice.h"

# include "graphics/Vulkan/Resources/VResourceBase.h"
# include "graphics/Vulkan/Resources/VBuffer.h"
# include "graphics/Vulkan/Resources/VDependency.h"
# include "graphics/Vulkan/Resources/VImage.h"
# include "graphics/Vulkan/Resources/VSampler.h"
# include "graphics/Vulkan/Resources/VMemoryObj.h"
# include "graphics/Vulkan/Resources/VDescriptorSetLayout.h"
# include "graphics/Vulkan/Resources/VPipelineLayout.h"
# include "graphics/Vulkan/Resources/VPipelinePack.h"
# include "graphics/Vulkan/Resources/VGraphicsPipeline.h"
# include "graphics/Vulkan/Resources/VComputePipeline.h"
# include "graphics/Vulkan/Resources/VMeshPipeline.h"
# include "graphics/Vulkan/Resources/VSampler.h"

# include "threading/Containers/LfIndexedPool.h"
# include "threading/Containers/CachedIndexedPool.h"

namespace AE::Graphics
{

	//
	// Vulkan Resource Manager
	//

	class VResourceManager final : public IResourceManager
	{
	// types
	private:
		using Index_t	= MemoryID::Index_t;

		template <typename T, size_t ChunkSize, size_t MaxChunks>
		using PoolTmpl	= Threading::IndexedPool< T, Index_t, ChunkSize, MaxChunks, UntypedAlignedAllocator >;

		// chunk size
		static constexpr uint	MaxDeps			= 1u << 10;
		static constexpr uint	MaxImages		= 1u << 10;
		static constexpr uint	MaxBuffers		= 1u << 10;
		static constexpr uint	MaxMemoryObjs	= 1u << 10;
		static constexpr uint	MaxCached		= 1u << 9;
		static constexpr uint	MaxRTObjects	= 1u << 9;
		static constexpr uint	MaxPipelines	= 1u << 10;
		
		using DepsPool_t			= PoolTmpl< VResourceBase<VDependency>,					MaxDeps,		32 >;
		using BufferPool_t			= PoolTmpl< VResourceBase<VBuffer>,						MaxBuffers,		32 >;
		using ImagePool_t			= PoolTmpl< VResourceBase<VImage>,						MaxImages,		32 >;
		using SamplerPackPool_t		= PoolTmpl< VResourceBase<VSamplerPack>,				128,			1 >;
		using SamplerPool_t			= PoolTmpl< VResourceBase<VSampler>,					4096,			1 >;		// TODO: static array
		using PipelinePackPool_t	= PoolTmpl< VResourceBase<VPipelinePack>,				128,			1 >;		// TODO: static array
		using MemoryPool_t			= PoolTmpl< VResourceBase<VMemoryObj>,					MaxMemoryObjs,	63 >;
		using DSLayoutPool_t		= PoolTmpl<	VResourceBase<VDescriptorSetLayout>,		MaxCached,		8 >;
		using PplnLayoutPool_t		= PoolTmpl<	VResourceBase<VPipelineLayout>,				MaxCached,		8 >;
		using GPipelinePool_t		= PoolTmpl< VResourceBase<VGraphicsPipeline>,			MaxPipelines,	32 >;
		using GPTemplatePool_t		= PoolTmpl< VResourceBase<VGraphicsPipelineTemplate>,	MaxCached,		8 >;
		using CPipelinePool_t		= PoolTmpl< VResourceBase<VComputePipeline>,			MaxPipelines,	32 >;
		using CPTemplatePool_t		= PoolTmpl< VResourceBase<VComputePipelineTemplate>,	MaxCached,		8 >;
		using MPipelinePool_t		= PoolTmpl< VResourceBase<VMeshPipeline>,				MaxPipelines,	32 >;
		using MPTemplatePool_t		= PoolTmpl< VResourceBase<VMeshPipelineTemplate>,		MaxCached,		8 >;
	/*
		using RTPipelinePool_t		= PoolTmpl<			VResourceBase<VRayTracingPipeline>,		MaxCached,		8 >;
		using RenderPassPool_t		= CachedPoolTmpl<	VResourceBase<VRenderPass>,				MaxCached,		8 >;
		using FramebufferPool_t		= CachedPoolTmpl<	VResourceBase<VFramebuffer>,			MaxCached,		8 >;
		using PplnResourcesPool_t	= CachedPoolTmpl<	VResourceBase<VPipelineResources>,		MaxCached,		8 >;
		using RTGeometryPool_t		= PoolTmpl<			VResourceBase<VRayTracingGeometry>,		MaxRTObjects,	16 >;
		using RTScenePool_t			= PoolTmpl<			VResourceBase<VRayTracingScene>,		MaxRTObjects,	16 >;
		using RTShaderTablePool_t	= PoolTmpl<			VResourceBase<VRayTracingShaderTable>,	MaxRTObjects,	16 >;
		using SwapchainPool_t		= PoolTmpl<			VResourceBase<VSwapchain>,				64,				1 >;
		*/

		using PipelineRefs			= VPipelinePack::PipelineRefs;
		using SamplerRefs			= VSamplerPack::SamplerRefs;
		using ShaderModule			= VGraphicsPipelineTemplate::ShaderModule;


	// variables
	private:
		VDevice const&			_device;

		struct {
			DepsPool_t				deps;

			BufferPool_t			buffers;
			ImagePool_t				images;
			MemoryPool_t			memoryObjs;
			
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

			/*
			RTPipelinePool_t		_rayTracingPplnPool;

			PplnResourcesPool_t		_pplnResourcesCache;

			RenderPassPool_t		_renderPassCache;
			FramebufferPool_t		_framebufferCache;

			RTGeometryPool_t		_rtGeometryPool;
			RTScenePool_t			_rtScenePool;
			RTShaderTablePool_t		_rtShaderTablePool;*/
		}						_resPool;
		
		SamplerRefs				_samplerRefs;	// TODO: guard
		PipelineRefs			_pipelineRefs;	// TODO: guard
		
		VSamplerID				_defaultSampler;
		VDescriptorSetLayoutID	_emptyDSLayout;

		// cached resources validation
		/*struct {
			Atomic<uint>			createdFramebuffers			{0};
			Atomic<uint>			lastCheckedFramebuffer		{0};

			Atomic<uint>			createdPplnResources		{0};
			Atomic<uint>			lastCheckedPipelineResource	{0};
		}						_validation;*/


	// public
	public:
		GfxResourceID		CreateDependency (StringView dbgName) override;
		GfxResourceID		CreateImage (const VirtualImageDesc &desc, StringView dbgName) override;
		GfxResourceID		CreateBuffer (const VirtualBufferDesc &desc, StringView dbgName) override;

		GfxResourceID		CreateImage (const ImageDesc &desc, EResourceState defaultState, StringView dbgName) override;
		GfxResourceID		CreateBuffer (const BufferDesc &desc, StringView dbgName) override;
		
		PipelinePackID		LoadPipelinePack (const SharedPtr<RStream> &stream) override;
		bool				ReloadPipelinePack (const SharedPtr<RStream> &stream, PipelinePackID id) override;
		
		GraphicsPipelineID	GetGraphicsPipeline (const PipelineName &name, const GraphicsPipelineDesc &desc) override;
		MeshPipelineID		GetMeshPipeline (const PipelineName &name, const MeshPipelineDesc &desc) override;
		ComputePipelineID	GetComputePipeline (const PipelineName &name, const ComputePipelineDesc &desc) override;
		RayTracingPipelineID GetRayTracingPipeline (const PipelineName &name, const RayTracingPipelineDesc &desc) override;

		ND_ VDescriptorSetLayoutID		CreateDescriptorSetLayout (const VDescriptorSetLayout::Uniforms_t &uniforms, ArrayView<VkSampler> samplerStorage, StringView dbgName = Default);
		ND_ VPipelineLayoutID			CreatePipelineLayout (const VPipelineLayout::DescriptorSets_t &descSetLayouts, const VPipelineLayout::PushConstants_t &pusConstants, StringView dbgName = Default);
		ND_ VGraphicsPipelineTemplateID	CreateGPTemplate (VPipelineLayoutID layoutId, const PipelineCompiler::GraphicsPipelineDesc &desc, ArrayView<ShaderModule> modules, StringView dbgName = Default);
		ND_ VMeshPipelineTemplateID		CreateMPTemplate (VPipelineLayoutID layoutId, const PipelineCompiler::MeshPipelineDesc &desc, ArrayView<ShaderModule> modules, StringView dbgName = Default);
		ND_ VComputePipelineTemplateID	CreateCPTemplate (VPipelineLayoutID layoutId, const PipelineCompiler::ComputePipelineDesc &desc, VkShaderModule modules, StringView dbgName = Default);

		bool				LoadSamplerPack (const SharedPtr<RStream> &stream) override;
		ND_ VSamplerID		CreateSampler (const VkSamplerCreateInfo &info, StringView dbgName = Default);
		ND_ VSamplerID		GetSampler (const SamplerName &name) const;
		ND_ VkSampler		GetVkSampler (const SamplerName &name) const;

		bool				ReleaseResource (GfxResourceID id) override;
		bool				ReleaseResource (MemoryID id) override;
		bool				ReleaseResource (PipelinePackID id) override;
		bool				ReleaseResource (VDescriptorSetLayoutID id);
		bool				ReleaseResource (VPipelineLayoutID id);
		bool				ReleaseResource (VGraphicsPipelineTemplateID id);
		bool				ReleaseResource (VMeshPipelineTemplateID id);
		bool				ReleaseResource (VComputePipelineTemplateID id);
		bool				ReleaseResource (VSamplerID id);
		
		template <typename ID>
		ND_ bool			IsResourceAlive (ID id) const;

		template <typename ID>
		ND_ auto const*		GetResource (ID id, bool incRef = false, bool quiet = false) const;

		ND_ VDevice const&	GetDevice ()	const	{ return _device; }


	private:
		bool  _CreateMemory (OUT MemoryID &id, OUT VResourceBase<VMemoryObj>* &memPtr, EMemoryType type, StringView dbgName);
		
		template <typename ID, typename FnInitialize, typename FnCreate>
		ND_ ID  _CreateCachedResource (StringView errorStr, FnInitialize&& fnInit, FnCreate&& fnCreate);

	// resource pool
		ND_ auto&  _GetResourcePool (const GfxResourceID &id)				{ CHECK( id.ResourceType() == GfxResourceID::EType::Unknown ); return _resPool.deps; }
		ND_ auto&  _GetResourcePool (const VBufferID &)						{ return _resPool.buffers; }
		ND_ auto&  _GetResourcePool (const VImageID &)						{ return _resPool.images; }
		ND_ auto&  _GetResourcePool (const MemoryID &)						{ return _resPool.memoryObjs; }
		ND_ auto&  _GetResourcePool (const PipelinePackID &)				{ return _resPool.pipelinePacks; }
		ND_ auto&  _GetResourcePool (const VDescriptorSetLayoutID &)		{ return _resPool.dsLayouts; }
		ND_ auto&  _GetResourcePool (const VPipelineLayoutID &)				{ return _resPool.pplnLayouts; }
		ND_ auto&  _GetResourcePool (const GraphicsPipelineID &)			{ return _resPool.graphicsPpln; }
		ND_ auto&  _GetResourcePool (const VGraphicsPipelineTemplateID &)	{ return _resPool.graphicsTempl; }
		ND_ auto&  _GetResourcePool (const ComputePipelineID &)				{ return _resPool.computePpln; }
		ND_ auto&  _GetResourcePool (const VComputePipelineTemplateID &)	{ return _resPool.computeTempl; }
		ND_ auto&  _GetResourcePool (const MeshPipelineID &)				{ return _resPool.meshPpln; }
		ND_ auto&  _GetResourcePool (const VMeshPipelineTemplateID &)		{ return _resPool.meshTempl; }
		ND_ auto&  _GetResourcePool (const VSamplerPackID &)				{ return _resPool.samplerPacks; }
		ND_ auto&  _GetResourcePool (const VSamplerID &)					{ return _resPool.samplers; }

		/*ND_ auto&  _GetResourcePool (const RawRenderPassID &)			{ return _renderPassCache; }
		ND_ auto&  _GetResourcePool (const RawFramebufferID &)			{ return _framebufferCache; }
		ND_ auto&  _GetResourcePool (const RawPipelineResourcesID &)	{ return _pplnResourcesCache; }
		ND_ auto&  _GetResourcePool (const RawRTGeometryID &)			{ return _rtGeometryPool; }
		ND_ auto&  _GetResourcePool (const RawRTSceneID &)				{ return _rtScenePool; }
		ND_ auto&  _GetResourcePool (const RawRTShaderTableID &)		{ return _rtShaderTablePool; }*/
		
		template <typename ID>
		ND_ const auto&  _GetResourceCPool (const ID &id)	const		{ return const_cast<VResourceManager *>(this)->_GetResourcePool( id ); }
		

	// 
		template <typename ID>	ND_ bool   _Assign (OUT ID &id);
		template <typename ID>		void   _Unassign (ID id);

		ND_ EQueueFamilyMask  _GetQueueFamilies (EQueueMask mask) const;


	// empty descriptor set layout
			bool  _CreateEmptyDescriptorSetLayout ();
		ND_ auto  _GetEmptyDescriptorSetLayout ()		{ return _emptyDSLayout; }

			bool  _CreateDefaultSampler ();

	// pipeline creation & validation
	
	};

	
/*
=================================================
	GetResource
=================================================
*/
	template <typename ID>
	inline auto const*  VResourceManager::GetResource (ID id, bool incRef, bool quiet) const
	{
		auto&	pool = _GetResourceCPool( id );

		using Result_t = typename std::remove_reference_t<decltype(pool)>::Value_t::Resource_t const*;

		if ( id.Index() < pool.size() )
		{
			auto&	data = pool[ id.Index() ];

			if ( data.IsCreated() and data.GetGeneration() == id.Generation() )
			{
				if ( incRef ) data.AddRef();
				return &data.Data();
			}

			ASSERT( quiet or data.IsCreated() );
			ASSERT( quiet or data.GetGeneration() == id.Generation() );
		}

		ASSERT( quiet and "resource index is out of range" );
		return static_cast< Result_t >(null);
	}
	
/*
=================================================
	IsResourceAlive
=================================================
*/
	template <typename ID>
	inline bool  VResourceManager::IsResourceAlive (ID id) const
	{
		ASSERT( id );
		auto&	pool = _GetResourceCPool( id );
		
		return	id.Index() < pool.size() and
				pool[id.Index()].GetGeneration() == id.Generation();
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
	inline bool  VResourceManager::_Assign (OUT ID &id)
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
	inline void  VResourceManager::_Unassign (ID id)
	{
		ASSERT( id );
		auto&	pool = _GetResourcePool( id );

		pool.Unassign( id.Index() );
	}


}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
