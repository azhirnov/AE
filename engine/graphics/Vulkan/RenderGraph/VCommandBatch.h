// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef AE_ENABLE_VULKAN

# include "graphics/Vulkan/VResourceManager.h"
# include "graphics/Vulkan/RenderGraph/VCommandPool.h"

namespace AE::Graphics
{

	//
	// Vulkan Command Batch
	//

	class VCommandBatch
	{
	// types
	public:
		using OutFences_t	= FixedArray< VkFence, 32 >;

	private:
		struct Resource
		{
		// types
			enum class EType
			{
				Buffer,
				Image,
				GraphicsPipeline,
				ComputePipeline,
				MeshPipeline,
				RayTracingPipeline,
				DescriptorSet,
				RenderPass,
				Framebuffer
			};

		// constants
			static constexpr uint		IndexOffset			= 0;
			static constexpr uint		GenerationOffset	= 16;
			static constexpr uint		ResTypeOffset		= 32;
			static constexpr uint64_t	IndexMask			= 0xFFFF;
			static constexpr uint64_t	GenerationMask		= 0xFFFF;
			static constexpr uint64_t	ResTypeMask			= 0xFF;

			STATIC_ASSERT( ((IndexMask << IndexOffset) | (GenerationMask << GenerationOffset) | (ResTypeMask << ResTypeOffset)) == 0xFF'FFFF'FFFFull );

		// variables
			uint64_t	value	= UMax;
			
		// methods
			Resource () {}
			
			Resource (uint64_t index, uint64_t generation, EType type) :
				value{ (index << IndexOffset) | (generation << GenerationOffset) | (uint64_t(type) << ResTypeOffset) } {}

			ND_ bool  operator == (const Resource &rhs)	const	{ return value == rhs.value; }

			ND_ uint16_t	Index ()				const	{ return CheckCast<uint16_t>((value >> IndexOffset) & IndexMask); }
			ND_ uint16_t	Generation ()			const	{ return CheckCast<uint16_t>((value >> GenerationOffset) & GenerationMask); }
			ND_ EType		ResType ()				const	{ return EType((value >> ResTypeOffset) & ResTypeMask); }
		};

		struct ResourceHash {
			ND_ size_t  operator () (const Resource &x) const {
				return std::hash<decltype(x.value)>{}( x.value );
			}
		};

		using ResourceMap_t		= HashSet< Resource, ResourceHash >;		// TODO: custom allocator
		using Semaphores_t		= FixedArray< VkSemaphore, 16 >;
		using Fences_t			= FixedArray< VkFence, 8 >;
		using CmdBuffers_t		= FixedArray< Pair<VCommandPool*, VkCommandBuffer>, 8 >;


	// variables
	private:
		VkFence				_fence		= VK_NULL_HANDLE;

		Atomic<uint16_t>	_generation	{0};

		ResourceMap_t		_resourceMap;
		Semaphores_t		_semaphores;
		Fences_t			_fences;
		CmdBuffers_t		_cmdBuffers;

		bool				_isComplete	= false;
		
		RWDataRaceCheck		_drCheck;


	// methods
	public:
		VCommandBatch () {}
		~VCommandBatch () {}

		void  Initialize ();
		bool  OnComplete (VResourceManager &resMngr);

		void  AddSemaphore (VkSemaphore sem);
		void  AddFence (VkFence fence);
		void  AddCmdBuffer (VCommandPool *pool, VkCommandBuffer cmdbuf);

		// returns 'true' if new resource has been added
		bool  AddResource (VBufferID id);
		bool  AddResource (VImageID id);
		bool  AddResource (GraphicsPipelineID id);
		bool  AddResource (MeshPipelineID id);
		bool  AddResource (ComputePipelineID id);
		bool  AddResource (RayTracingPipelineID id);
		bool  AddResource (RenderPassID id);
		bool  AddResource (VFramebufferID id);

		ND_ bool	IsComplete ()	const	{ SHAREDLOCK( _drCheck );  return _isComplete; }
		ND_ uint	Generation ()	const	{ SHAREDLOCK( _drCheck );  return _generation.load( EMemoryOrder::Relaxed ); }
	};

	
/*
=================================================
	AddResource
=================================================
*/
	bool  VCommandBatch::AddResource (VBufferID id)
	{
		EXLOCK( _drCheck );
		return _resourceMap.insert(Resource{ id.Index(), id.Generation(), Resource::EType::Buffer }).second;
	}

	bool  VCommandBatch::AddResource (VImageID id)
	{
		EXLOCK( _drCheck );
		return _resourceMap.insert(Resource{ id.Index(), id.Generation(), Resource::EType::Image }).second;
	}

	bool  VCommandBatch::AddResource (GraphicsPipelineID id)
	{
		EXLOCK( _drCheck );
		return _resourceMap.insert(Resource{ id.Index(), id.Generation(), Resource::EType::GraphicsPipeline }).second;
	}

	bool  VCommandBatch::AddResource (MeshPipelineID id)
	{
		EXLOCK( _drCheck );
		return _resourceMap.insert(Resource{ id.Index(), id.Generation(), Resource::EType::MeshPipeline }).second;
	}

	bool  VCommandBatch::AddResource (ComputePipelineID id)
	{
		EXLOCK( _drCheck );
		return _resourceMap.insert(Resource{ id.Index(), id.Generation(), Resource::EType::ComputePipeline }).second;
	}

	bool  VCommandBatch::AddResource (RayTracingPipelineID id)
	{
		EXLOCK( _drCheck );
		return _resourceMap.insert(Resource{ id.Index(), id.Generation(), Resource::EType::RayTracingPipeline }).second;
	}
	
	bool  VCommandBatch::AddResource (RenderPassID id)
	{
		EXLOCK( _drCheck );
		return _resourceMap.insert(Resource{ id.Index(), id.Generation(), Resource::EType::RenderPass }).second;
	}

	bool  VCommandBatch::AddResource (VFramebufferID id)
	{
		EXLOCK( _drCheck );
		return _resourceMap.insert(Resource{ id.Index(), id.Generation(), Resource::EType::Framebuffer }).second;
	}
	
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
	bool  VCommandBatch::OnComplete (VResourceManager &resMngr)
	{
		using EResType = Resource::EType;

		EXLOCK( _drCheck );

		if ( _isComplete )
			return true;

		auto&	dev = resMngr.GetDevice();

		CHECK( _fences.size() );
		VK_CHECK( dev.vkWaitForFences( dev.GetVkDevice(), uint(_fences.size()), _fences.data(), true, UMax ));

		resMngr.ReleaseFences( _fences );
		resMngr.ReleaseSemaphores( _semaphores );

		for (auto[pool, cmdbuf] : _cmdBuffers) {
			pool->RecyclePrimary( dev, cmdbuf );
		}

		_fences.clear();
		_semaphores.clear();
		_cmdBuffers.clear();

		for (auto& res : _resourceMap)
		{
			BEGIN_ENUM_CHECKS();
			switch ( res.ResType() )
			{
				case EResType::Buffer : {
					UniqueID<GfxResourceID>			id{ GfxResourceID{ res.Index(), res.Generation(), GfxResourceID::EType::Buffer }};
					resMngr.ReleaseResource( id );
					break;
				}
				case EResType::Image : {
					UniqueID<GfxResourceID>			id{ GfxResourceID{ res.Index(), res.Generation(), GfxResourceID::EType::Image }};
					resMngr.ReleaseResource( id );
					break;
				}
				case EResType::GraphicsPipeline : {
					UniqueID<GraphicsPipelineID>	id{ GraphicsPipelineID{ res.Index(), res.Generation() }};
					resMngr.ReleaseResource( id );
					break;
				}
				case EResType::ComputePipeline : {
					UniqueID<ComputePipelineID>		id{ ComputePipelineID{ res.Index(), res.Generation() }};
					resMngr.ReleaseResource( id );
					break;
				}
				case EResType::MeshPipeline : {
					UniqueID<MeshPipelineID>		id{ MeshPipelineID{ res.Index(), res.Generation() }};
					resMngr.ReleaseResource( id );
					break;
				}
				case EResType::RayTracingPipeline : {
					//resMngr.ReleaseResource( RayTracingPipelineID{ res.Index(), res.Generation() });
					break;
				}
				case EResType::DescriptorSet : {
					//resMngr.ReleaseResource( DescriptorSetID{ res.Index(), res.Generation() });
					break;
				}
				case EResType::RenderPass : {
					UniqueID<RenderPassID>		id{ RenderPassID{ res.Index(), res.Generation() }};
					resMngr.ReleaseResource( id );
					break;
				}
				case EResType::Framebuffer : {
					UniqueID<VFramebufferID>	id{ VFramebufferID{ res.Index(), res.Generation() }};
					resMngr.ReleaseResource( id );
					break;
				}
			}
			END_ENUM_CHECKS();
		}
		_resourceMap.clear();

		_isComplete = true;
		
		_generation.fetch_add( 1, EMemoryOrder::Relaxed );
		return true;
	}
	
/*
=================================================
	AddSemaphore
=================================================
*/
	void  VCommandBatch::AddSemaphore (VkSemaphore sem)
	{
		EXLOCK( _drCheck );
		_semaphores.push_back( sem );	// TODO: check for overflow
	}
	
/*
=================================================
	AddFence
=================================================
*/
	void  VCommandBatch::AddFence (VkFence fence)
	{
		EXLOCK( _drCheck );
		_fences.push_back( fence );		// TODO: check for overflow
	}
	
/*
=================================================
	AddCmdBuffer
=================================================
*/
	void  VCommandBatch::AddCmdBuffer (VCommandPool *pool, VkCommandBuffer cmdbuf)
	{
		EXLOCK( _drCheck );
		_cmdBuffers.push_back({ pool, cmdbuf });
	}


}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
