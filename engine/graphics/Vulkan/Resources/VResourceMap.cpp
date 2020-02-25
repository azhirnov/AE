// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#ifdef AE_ENABLE_VULKAN

# include "graphics/Vulkan/Resources/VResourceMap.h"
# include "graphics/Vulkan/VResourceManager.h"

namespace AE::Graphics
{
	
/*
=================================================
	Release
=================================================
*/
	void  VResourceMap::Release (VResourceManager &resMngr)
	{
		using EResType = Resource::EType;

		for (auto& res : _items)
		{
			BEGIN_ENUM_CHECKS();
			switch ( res.ResType() )
			{
				case EResType::Buffer : {
					UniqueID<VBufferID>		id{ VBufferID{ res.Index(), res.Generation() }};
					resMngr.ReleaseResource( id, res.RefCount() );
					break;
				}
				case EResType::Image : {
					UniqueID<VImageID>		id{ VImageID{ res.Index(), res.Generation() }};
					resMngr.ReleaseResource( id, res.RefCount() );
					break;
				}
				case EResType::GraphicsPipeline : {
					UniqueID<GraphicsPipelineID>	id{ GraphicsPipelineID{ res.Index(), res.Generation() }};
					resMngr.ReleaseResource( id, res.RefCount() );
					break;
				}
				case EResType::ComputePipeline : {
					UniqueID<ComputePipelineID>		id{ ComputePipelineID{ res.Index(), res.Generation() }};
					resMngr.ReleaseResource( id, res.RefCount() );
					break;
				}
				case EResType::MeshPipeline : {
					UniqueID<MeshPipelineID>	id{ MeshPipelineID{ res.Index(), res.Generation() }};
					resMngr.ReleaseResource( id, res.RefCount() );
					break;
				}
				case EResType::RayTracingPipeline : {
					break;
				}
				case EResType::DescriptorSet : {
					break;
				}
				case EResType::RenderPass : {
					UniqueID<RenderPassID>		id{ RenderPassID{ res.Index(), res.Generation() }};
					resMngr.ReleaseResource( id, res.RefCount() );
					break;
				}
				case EResType::Framebuffer : {
					UniqueID<VFramebufferID>	id{ VFramebufferID{ res.Index(), res.Generation() }};
					resMngr.ReleaseResource( id, res.RefCount() );
					break;
				}
				case EResType::BackedCommandBuffer : {
					UniqueID<BakedCommandBufferID>	id{ BakedCommandBufferID{ res.Index(), res.Generation() }};
					resMngr.ReleaseResource( id, res.RefCount() );
					break;
				}
				default :
					AE_LOGE( "unknown resource type!" );
					break;
			}
			END_ENUM_CHECKS();
		}
		_items.clear();
	}
	
/*
=================================================
	Merge
=================================================
*/
	void  VResourceMap::Merge (INOUT VResourceMap &other)
	{
		for (auto& item : other._items)
		{
			auto[iter, inserted] = _items.insert( item );

			if ( not inserted )
				iter->IncRefCount();
		}
		other._items.clear();
	}


}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
