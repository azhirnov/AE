// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef AE_ENABLE_VULKAN

# include "ecs-st/Renderer/IRenderPass.h"
# include "ecs-st/Core/Registry.h"
# include "ecs-st/RendererVk/VComponents.h"
# include "graphics/Vulkan/VResourceManager.h"
# include "graphics/Public/RenderGraph.h"

namespace AE::ECS::Systems
{

	//
	// Vulkan Graphics Pass
	//

	template <Renderer::ERenderPass P>
	class VGraphicsPass : public IRenderPass
	{
	// types
	private:
		using VResourceManager		= Graphics::VResourceManager;
		using RenderPassID			= Graphics::RenderPassID;
		using UniqueCommandBufferID	= Graphics::UniqueID< Graphics::BakedCommandBufferID >;


	// variables
	private:
		Registry &				_owner;
		VkCommandBuffer			_renderCmd;
		struct {
			QueryID					updateGraphicsPpln;
			QueryID					updateMeshPpln;
			QueryID					drawVerticesPC;
			QueryID					drawVertices;
			QueryID					drawIndexedPC;
			QueryID					drawIndexed;
			QueryID					drawMeshesPC;
			QueryID					drawMeshes;
		}						_query;
		VResourceManager &		_resMngr;
		UniqueCommandBufferID	_bakedCmdBuf;

	public:
		VkDescriptorSet			_perPassDS;
		RenderPassID			_renderPassId;

	// methods
	public:
		VGraphicsPass (Registry &reg, VResourceManager &rm);

		void  Update () override;
		void  Render (Graphics::IAsyncRenderContext &ctx);

	private:
		void  _RebuildCommandBuffer ();
	};
//-----------------------------------------------------------------------------


	
/*
=================================================
	constructor
=================================================
*/
	template <Renderer::ERenderPass P>
	inline VGraphicsPass<P>::VGraphicsPass (Registry &reg, VResourceManager &rm) :
		_owner{reg}, _resMngr{rm}
	{
		using namespace ECS::Components;

		using RPTag = RenderPassTag<P>;

		_query.updateGraphicsPpln	= _owner.CreateQuery< WriteAccess< VPipeline >,
														  WriteAccess< VGraphicsPipelineRef >,
														  Require< RPTag, InvalidatePipelineTag >,
														  RequireAny< VDrawVertices, VDrawIndexedVertices >>();
		_query.updateMeshPpln		= _owner.CreateQuery< WriteAccess< VPipeline >,
														  WriteAccess< VMeshPipelineRef >,
														  Require< RPTag, InvalidatePipelineTag, VDrawMeshes >>();
		_query.drawVerticesPC		= _owner.CreateQuery< ReadAccess< VDrawVertices >,
														  ReadAccess< VDescriptorSets >,
														  ReadAccess< VPipeline >,
														  ReadAccess< VPushConstant >,
														  Require< RPTag >>();
		_query.drawVertices			= _owner.CreateQuery< ReadAccess< VDrawVertices >,
														  ReadAccess< VDescriptorSets >,
														  ReadAccess< VPipeline >,
														  Require< RPTag >,
														  Subtractive< VPushConstant >>();
		_query.drawIndexedPC		= _owner.CreateQuery< ReadAccess< VDrawIndexedVertices >,
														  ReadAccess< VDescriptorSets >,
														  ReadAccess< VPipeline >,
														  ReadAccess< VPushConstant >,
														  Require< RPTag >>();
		_query.drawIndexed			= _owner.CreateQuery< ReadAccess< VDrawIndexedVertices >,
														  ReadAccess< VDescriptorSets >,
														  ReadAccess< VPipeline >,
														  Require< RPTag >,
														  Subtractive< VPushConstant >>();
		_query.drawMeshesPC			= _owner.CreateQuery< ReadAccess< VDrawMeshes >,
														  ReadAccess< VDescriptorSets >,
														  ReadAccess< VPipeline >,
														  ReadAccess< VPushConstant >,
														  Require< RPTag >>();
		_query.drawMeshes			= _owner.CreateQuery< ReadAccess< VDrawMeshes >,
														  ReadAccess< VDescriptorSets >,
														  ReadAccess< VPipeline >,
														  Require< RPTag >,
														  Subtractive< VPushConstant >>();
	}

/*
=================================================
	Update
=================================================
*/
	template <Renderer::ERenderPass P>
	inline void  VGraphicsPass<P>::Update ()
	{
		using namespace ECS::Components;

		if ( not _renderPassId )
			return;

		_owner.Execute(
			_query.updateGraphicsPpln,
			[this] (ArrayView<Tuple< size_t,
									 WriteAccess<VPipeline>,
									 WriteAccess<VGraphicsPipelineRef> >> chunks)
			{
				for (auto& chunk : chunks)
				{
					size_t	count	= chunk.template Get<0>();
					auto&	dst		= chunk.template Get<1>();
					auto&	src		= chunk.template Get<2>();

					for (size_t i = 0; i < count; ++i)
					{
						src[i].desc.renderPassId = _renderPassId;

						auto	ppln = _resMngr.GetGraphicsPipeline( src[i].name, src[i].desc );
						auto	desc = _resMngr.GetPipelineNativeDesc( ppln );

						if ( auto* p = UnionGetIf<VulkanPipelineInfo>( &desc ))
						{
							dst[i].handle = p->pipeline;
							dst[i].layout = p->layout;
						}
						else
						{
							dst[i].handle = VK_NULL_HANDLE;
							dst[i].layout = VK_NULL_HANDLE;
						}
					}
				}
			}
		);

		_owner.Execute(
			_query.updateMeshPpln,
			[this] (ArrayView<Tuple< size_t,
									 WriteAccess<VPipeline>,
									 WriteAccess<VMeshPipelineRef> >> chunks)
			{
				for (auto& chunk : chunks)
				{
					size_t	count	= chunk.template Get<0>();
					auto&	dst		= chunk.template Get<1>();
					auto&	src		= chunk.template Get<2>();
					
					for (size_t i = 0; i < count; ++i)
					{
						src[i].desc.renderPassId = _renderPassId;

						auto	ppln = _resMngr.GetMeshPipeline( src[i].name, src[i].desc );
						auto	desc = _resMngr.GetPipelineNativeDesc( ppln );
					
						if ( auto* p = UnionGetIf<VulkanPipelineInfo>( &desc ))
						{
							dst[i].handle = p->pipeline;
							dst[i].layout = p->layout;
						}
						else
						{
							dst[i].handle = VK_NULL_HANDLE;
							dst[i].layout = VK_NULL_HANDLE;
						}
					}
				}
			}
		);

		_owner.RemoveComponents<InvalidatePipelineTag>( _query.updateGraphicsPpln );
		_owner.RemoveComponents<InvalidatePipelineTag>( _query.updateMeshPpln );

		_resMngr.ReleaseResource( _bakedCmdBuf );
	}
	
/*
=================================================
	Render
=================================================
*/
	template <Renderer::ERenderPass P>
	inline void  VGraphicsPass<P>::Render (Graphics::IAsyncRenderContext &ctx)
	{
		if ( _bakedCmdBuf and ctx.ReuseCommandBuffer( _bakedCmdBuf ))
			return;

		_resMngr.ReleaseResource( _bakedCmdBuf );
		
		ctx.BeginCommandBuffer();

		auto	info = ctx.GetNativeContext();

		if ( auto* vk_info = UnionGetIf<VulkanRenderContext>( &info ))
		{
			_renderCmd = VkCommandBuffer(vk_info->cmdBuffer);
		}

		_RebuildCommandBuffer();

		_bakedCmdBuf = ctx.CacheCommandBuffer();
	}

/*
=================================================
	_RebuildCommandBuffer
=================================================
*/
	template <Renderer::ERenderPass P>
	inline void  VGraphicsPass<P>::_RebuildCommandBuffer ()
	{
		using namespace ECS::Components;

		ASSERT( _renderCmd );
		
		_owner.Execute(
			_query.drawVerticesPC,
			[this] (ArrayView<Tuple< size_t,
									 ReadAccess<VDrawVertices>,
									 ReadAccess<VDescriptorSets>,
									 ReadAccess<VPipeline>,
									 ReadAccess<VPushConstant> >> chunks)
			{
				auto&	dev = _resMngr.GetDevice();

				for (auto& chunk : chunks)
				{
					size_t	count	= chunk.template Get<0>();
					auto	draw	= chunk.template Get<1>();
					auto	ds		= chunk.template Get<2>();
					auto	ppln	= chunk.template Get<3>();
					auto	pc		= chunk.template Get<4>();

					for (size_t i = 0; i < count; ++i)
					{
						dev.vkCmdBindPipeline( _renderCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ppln[i].handle );
						dev.vkCmdBindDescriptorSets( _renderCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ppln[i].layout, RendererVk::Config::RenderPassDSIndex, 1, &_perPassDS, 0, null );
						dev.vkCmdBindDescriptorSets( _renderCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ppln[i].layout, RendererVk::Config::DrawCallDSIndex, uint(ds[i].value.size()), ds[i].value.data(), 0, null );
						dev.vkCmdPushConstants( _renderCmd, ppln[i].layout, VkShaderStageFlagBits(pc[i].stageFlags), pc[i].offset*4, pc[i].size*4, pc[i].values.data() );
						// TODO: bind VBO
						dev.vkCmdDraw( _renderCmd, draw[i].vertexCount, 1, draw[i].firstVertex, 0 );
					}
				}
			});
		_owner.Execute(
			_query.drawIndexedPC,
			[this] (ArrayView<Tuple< size_t,
									 ReadAccess<VDrawIndexedVertices>,
									 ReadAccess<VDescriptorSets>,
									 ReadAccess<VPipeline>,
									 ReadAccess<VPushConstant> >> chunks)
			{
				auto&	dev = _resMngr.GetDevice();

				for (auto& chunk : chunks)
				{
					size_t	count	= chunk.template Get<0>();
					auto	draw	= chunk.template Get<1>();
					auto	ds		= chunk.template Get<2>();
					auto	ppln	= chunk.template Get<3>();
					auto	pc		= chunk.template Get<4>();
					
					for (size_t i = 0; i < count; ++i)
					{
						dev.vkCmdBindPipeline( _renderCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ppln[i].handle );
						dev.vkCmdBindDescriptorSets( _renderCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ppln[i].layout, RendererVk::Config::RenderPassDSIndex, 1, &_perPassDS, 0, null );
						dev.vkCmdBindDescriptorSets( _renderCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ppln[i].layout, RendererVk::Config::DrawCallDSIndex, uint(ds[i].value.size()), ds[i].value.data(), 0, null );
						dev.vkCmdPushConstants( _renderCmd, ppln[i].layout, VkShaderStageFlagBits(pc[i].stageFlags), pc[i].offset*4, pc[i].size*4, pc[i].values.data() );
						// TODO: bind VBo & IBO
						dev.vkCmdDrawIndexed( _renderCmd, draw[i].indexCount, 1, draw[i].firstIndex, draw[i].vertexOffset, 0 );
					}
				}
			});

		_owner.Execute(
			_query.drawVertices,
			[this] (ArrayView<Tuple< size_t,
									 ReadAccess<VDrawVertices>,
									 ReadAccess<VDescriptorSets>,
									 ReadAccess<VPipeline> >> chunks)
			{
				auto&	dev = _resMngr.GetDevice();

				for (auto& chunk : chunks)
				{
					size_t	count	= chunk.template Get<0>();
					auto	draw	= chunk.template Get<1>();
					auto	ds		= chunk.template Get<2>();
					auto	ppln	= chunk.template Get<3>();
					
					for (size_t i = 0; i < count; ++i)
					{
						dev.vkCmdBindPipeline( _renderCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ppln[i].handle );
						dev.vkCmdBindDescriptorSets( _renderCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ppln[i].layout, RendererVk::Config::RenderPassDSIndex, 1, &_perPassDS, 0, null );
						dev.vkCmdBindDescriptorSets( _renderCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ppln[i].layout, RendererVk::Config::DrawCallDSIndex, uint(ds[i].value.size()), ds[i].value.data(), 0, null );
						// TODO: bind VBO
						dev.vkCmdDraw( _renderCmd, draw[i].vertexCount, 1, draw[i].firstVertex, 0 );
					}
				}
			});
		_owner.Execute(
			_query.drawIndexed,
			[this] (ArrayView<Tuple< size_t,
									 ReadAccess<VDrawIndexedVertices>,
									 ReadAccess<VDescriptorSets>,
									 ReadAccess<VPipeline> >> chunks)
			{
				auto&	dev = _resMngr.GetDevice();

				for (auto& chunk : chunks)
				{
					size_t	count	= chunk.template Get<0>();
					auto	draw	= chunk.template Get<1>();
					auto	ds		= chunk.template Get<2>();
					auto	ppln	= chunk.template Get<3>();
					
					for (size_t i = 0; i < count; ++i)
					{
						dev.vkCmdBindPipeline( _renderCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ppln[i].handle );
						dev.vkCmdBindDescriptorSets( _renderCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ppln[i].layout, RendererVk::Config::RenderPassDSIndex, 1, &_perPassDS, 0, null );
						dev.vkCmdBindDescriptorSets( _renderCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ppln[i].layout, RendererVk::Config::DrawCallDSIndex, uint(ds[i].value.size()), ds[i].value.data(), 0, null );
						// TODO: bind VBo & IBO
						dev.vkCmdDrawIndexed( _renderCmd, draw[i].indexCount, 1, draw[i].firstIndex, draw[i].vertexOffset, 0 );
					}
				}
			});

		// with mesh pipeline
	#ifdef VK_NV_mesh_shader
		_owner.Execute(
			_query.drawMeshesPC,
			[this] (ArrayView<Tuple< size_t,
									 ReadAccess<VDrawMeshes>,
									 ReadAccess<VDescriptorSets>,
									 ReadAccess<VPipeline>,
									 ReadAccess<VPushConstant> >> chunks)
			{
				auto&	dev = _resMngr.GetDevice();

				for (auto& chunk : chunks)
				{
					size_t	count	= chunk.template Get<0>();
					auto	draw	= chunk.template Get<1>();
					auto	ds		= chunk.template Get<2>();
					auto	ppln	= chunk.template Get<3>();
					auto	pc		= chunk.template Get<4>();
					
					for (size_t i = 0; i < count; ++i)
					{
						dev.vkCmdBindPipeline( _renderCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ppln[i].handle );
						dev.vkCmdBindDescriptorSets( _renderCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ppln[i].layout, RendererVk::Config::RenderPassDSIndex, 1, &_perPassDS, 0, null );
						dev.vkCmdBindDescriptorSets( _renderCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ppln[i].layout, RendererVk::Config::DrawCallDSIndex, uint(ds[i].value.size()), ds[i].value.data(), 0, null );
						dev.vkCmdPushConstants( _renderCmd, ppln[i].layout, VkShaderStageFlagBits(pc[i].stageFlags), pc[i].offset*4, pc[i].size*4, pc[i].values.data() );
						dev.vkCmdDrawMeshTasksNV( _renderCmd, draw[i].count, draw[i].first );
					}
				}
			});
		_owner.Execute(
			_query.drawMeshes,
			[this] (ArrayView<Tuple< size_t,
									 ReadAccess<VDrawMeshes>,
									 ReadAccess<VDescriptorSets>,
									 ReadAccess<VPipeline> >> chunks)
			{
				auto&	dev = _resMngr.GetDevice();

				for (auto& chunk : chunks)
				{
					size_t	count	= chunk.template Get<0>();
					auto	draw	= chunk.template Get<1>();
					auto	ds		= chunk.template Get<2>();
					auto	ppln	= chunk.template Get<3>();
					
					for (size_t i = 0; i < count; ++i)
					{
						dev.vkCmdBindPipeline( _renderCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ppln[i].handle );
						dev.vkCmdBindDescriptorSets( _renderCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ppln[i].layout, RendererVk::Config::RenderPassDSIndex, 1, &_perPassDS, 0, null );
						dev.vkCmdBindDescriptorSets( _renderCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ppln[i].layout, RendererVk::Config::DrawCallDSIndex, uint(ds[i].value.size()), ds[i].value.data(), 0, null );
						dev.vkCmdDrawMeshTasksNV( _renderCmd, draw[i].count, draw[i].first );
					}
				}
			});
	#endif
	}

}	// AE::ECS::Systems

#endif	// AE_ENABLE_VULKAN
