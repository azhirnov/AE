// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef AE_ENABLE_VULKAN

# include "ecs-st/Renderer/IRenderPass.h"
# include "ecs-st/Core/Registry.h"
# include "ecs-st/RendererVk/VComponents.h"
# include "graphics/Vulkan/VResourceManager.h"

namespace AE::ECS::Systems
{

	//
	// Vulkan Compute Pass
	//
	
	template <Renderer::ERenderPass P>
	class VComputePass : public Vulkan::VulkanDeviceFn, public IRenderPass
	{
	// types
	private:
		using VResourceManager	= Graphics::VResourceManager;


	// variables
	private:
		VkCommandBuffer		_primaryCmd;
		Registry &			_owner;
		struct {
			QueryID				updatePpln;
			QueryID				dispatchPC;
			QueryID				dispatch;
			QueryID				dispatchBasePC;
			QueryID				dispatchBase;
		}					_query;
		VResourceManager &	_resMngr;


	// methods
	public:
		explicit VComputePass (Registry &reg, VResourceManager &rm);
		
		void Update () override;
		void Render (VkCommandBuffer cmdbuf);
	};
	
	
/*
=================================================
	constructor
=================================================
*/
	template <Renderer::ERenderPass P>
	inline VComputePass::VComputePass (Registry &reg, VResourceManager &rm) :
		_owner{reg}, _resMngr{rm}
	{
		using namespace ECS::Components;

		using RPTag = RenderPassTag<P>;
		
		_query.updatePpln		= _owner.CreateQuery< WriteAccess< VPipeline >,
													  WriteAccess< VComputePipelineRef >,
													  Require< RPTag, InvalidatePipelineTag >,
													  RequireAny< VDispatchCompute, VDispatchComputeBase >>();
		_query.dispatchPC		= _owner.CreateQuery< ReadAccess<VDispatchCompute>,
													  ReadAccess<VDescriptorSets>,
													  ReadAccess<VPipeline>,
													  ReadAccess<VPushConstant>,
													  Require<RPTag> >();
		_query.dispatch			= _owner.CreateQuery< ReadAccess<VDispatchComputeBase>,
													  ReadAccess<VDescriptorSets>,
													  ReadAccess<VPipeline>,
													  ReadAccess<VPushConstant>,
													  Require<RPTag> >();
		_query.dispatchBasePC	= _owner.CreateQuery< ReadAccess<VDispatchCompute>,
													  ReadAccess<VDescriptorSets>,
													  ReadAccess<VPipeline>,
													  Require<RPTag>,
													  Subtractive<VPushConstant> >();
		_query.dispatchBase		= _owner.CreateQuery< ReadAccess<VDispatchComputeBase>,
													  ReadAccess<VDescriptorSets>,
													  ReadAccess<VPipeline>,
													  Require<RPTag>,
													  Subtractive<VPushConstant> >();
	}

/*
=================================================
	Update
=================================================
*/
	template <Renderer::ERenderPass P>
	inline void  VComputePass<P>::Update ()
	{
		using namespace ECS::Components;

		_owner.Execute(
			_query.updatePpln,
			[this] (ArrayView<Tuple< size_t,
									 WriteAccess<VPipeline>,
									 WriteAccess<VComputePipelineRef> >> chunks)
			{
				for (auto& chunk : chunks)
				{
					size_t	count	= chunk.template Get<0>();
					auto&	dst		= chunk.template Get<1>();
					auto&	src		= chunk.template Get<2>();

					for (size_t i = 0; i < count; ++i)
					{
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

		_owner.RemoveComponents<InvalidatePipelineTag>( _query.updatePpln );
	}
	
/*
=================================================
	Render
=================================================
*/
	template <Renderer::ERenderPass P>
	inline void  VComputePass<P>::Render (VkCommandBuffer cmdbuf)
	{
		using namespace ECS::Components;
		
		_owner.Execute(
			_query.dispatchPC,
			[this] (ArrayView<Tuple< size_t,
									 ReadAccess<VDispatchCompute>,
									 ReadAccess<VDescriptorSets>,
									 ReadAccess<VPipeline>,
									 ReadAccess<VPushConstant> >> chunks)
			{
				for (auto& chunk : chunks)
				{
					size_t	count	= chunk.template Get<0>();
					auto	disp	= chunk.template Get<1>();
					auto	ds		= chunk.template Get<2>();
					auto	ppln	= chunk.template Get<3>();
					auto	pc		= chunk.template Get<4>();
					
					for (size_t i = 0; i < count; ++i)
					{
						vkCmdBindPipeline( _primaryCmd, VK_PIPELINE_BIND_POINT_COMPUTE, ppln[i].handle );
						vkCmdBindDescriptorSets( _primaryCmd, VK_PIPELINE_BIND_POINT_COMPUTE, ppln[i].layout, RendererVk::Config::DrawCallDSIndex, uint(ds[i].value.size()), ds[i].value.data(), 0, null );
						vkCmdPushConstants( _primaryCmd, ppln[i].layout, VK_SHADER_STAGE_COMPUTE_BIT, pc[i].offset*4, pc[i].size*4, pc[i].values.data() );
						vkCmdDispatch( _primaryCmd, disp[i].groupCount.x, disp[i].groupCount.y, disp[i].groupCount.z );
					}
				}
			});
		_owner.Execute(
			_query.dispatchBasePC,
			[this] (ArrayView<Tuple< size_t,
									 ReadAccess<VDispatchComputeBase>,
									 ReadAccess<VDescriptorSets>,
									 ReadAccess<VPipeline>,
									 ReadAccess<VPushConstant> >> chunks)
			{
				for (auto& chunk : chunks)
				{
					size_t	count	= chunk.template Get<0>();
					auto	disp	= chunk.template Get<1>();
					auto	ds		= chunk.template Get<2>();
					auto	ppln	= chunk.template Get<3>();
					auto	pc		= chunk.template Get<4>();
					
					for (size_t i = 0; i < count; ++i)
					{
						vkCmdBindPipeline( _primaryCmd, VK_PIPELINE_BIND_POINT_COMPUTE, ppln[i].handle );
						vkCmdBindDescriptorSets( _primaryCmd, VK_PIPELINE_BIND_POINT_COMPUTE, ppln[i].layout, RendererVk::Config::DrawCallDSIndex, uint(ds[i].value.size()), ds[i].value.data(), 0, null );
						vkCmdPushConstants( _primaryCmd, ppln[i].layout, VK_SHADER_STAGE_COMPUTE_BIT, pc[i].offset*4, pc[i].size*4, pc[i].values.data() );
						vkCmdDispatchBase( _primaryCmd, disp[i].baseGroup.x, disp[i].baseGroup.y, disp[i].baseGroup.z, disp[i].groupCount.x, disp[i].groupCount.y, disp[i].groupCount.z );
					}
				}
			});

		_owner.Execute(
			_query.dispatch,
			[this] (ArrayView<Tuple< size_t,
									 ReadAccess<VDispatchCompute>,
									 ReadAccess<VDescriptorSets>,
									 ReadAccess<VPipeline> >> chunks)
			{
				for (auto& chunk : chunks)
				{
					size_t	count	= chunk.template Get<0>();
					auto	disp	= chunk.template Get<1>();
					auto	ds		= chunk.template Get<2>();
					auto	ppln	= chunk.template Get<3>();
					
					for (size_t i = 0; i < count; ++i)
					{
						vkCmdBindPipeline( _primaryCmd, VK_PIPELINE_BIND_POINT_COMPUTE, ppln[i].handle );
						vkCmdBindDescriptorSets( _primaryCmd, VK_PIPELINE_BIND_POINT_COMPUTE, ppln[i].layout, RendererVk::Config::DrawCallDSIndex, uint(ds[i].value.size()), ds[i].value.data(), 0, null );
						vkCmdDispatch( _primaryCmd, disp[i].groupCount.x, disp[i].groupCount.y, disp[i].groupCount.z );
					}
				}
			});
		_owner.Execute(
			_query.dispatchBase,
			[this] (ArrayView<Tuple< size_t,
									 ReadAccess<VDispatchComputeBase>,
									 ReadAccess<VDescriptorSets>,
									 ReadAccess<VPipeline> >> chunks)
			{
				for (auto& chunk : chunks)
				{
					size_t	count	= chunk.template Get<0>();
					auto	disp	= chunk.template Get<1>();
					auto	ds		= chunk.template Get<2>();
					auto	ppln	= chunk.template Get<3>();
					
					for (size_t i = 0; i < count; ++i)
					{
						vkCmdBindPipeline( _primaryCmd, VK_PIPELINE_BIND_POINT_COMPUTE, ppln[i].handle );
						vkCmdBindDescriptorSets( _primaryCmd, VK_PIPELINE_BIND_POINT_COMPUTE, ppln[i].layout, RendererVk::Config::DrawCallDSIndex, uint(ds[i].value.size()), ds[i].value.data(), 0, null );
						vkCmdDispatchBase( _primaryCmd, disp[i].baseGroup.x, disp[i].baseGroup.y, disp[i].baseGroup.z, disp[i].groupCount.x, disp[i].groupCount.y, disp[i].groupCount.z );
					}
				}
			});
	}


}	// AE::ECS::Systems

#endif	// AE_ENABLE_VULKAN
