// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef AE_ENABLE_VULKAN

# include "ecs-st/Renderer/Components.h"
# include "ecs-st/RendererVk/Common.h"
# include "graphics/Vulkan/VDevice.h"
# include "graphics/Public/PipelineDesc.h"

namespace AE::ECS::Components
{

	struct VPushConstant
	{
		using Data_t = StaticArray< uint, Renderer::Config::ObjectPushConstSize / 4 >;

		uint16_t	stageFlags	= 0;
		uint8_t		offset		= 0;	// multiple of 4
		uint8_t		size		= 0;	// multiple of 4
		Data_t		values		= {};
	};


	struct VPipeline
	{
		VkPipeline			handle	= VK_NULL_HANDLE;
		VkPipelineLayout	layout	= VK_NULL_HANDLE;
	};


	struct VDescriptorSets
	{
		enum {
			Material	= 0,	// per material descriptor set
			DrawCall	= 1,	// per draw call descriptor set
			_Count
		};
		StaticArray< VkDescriptorSet, _Count >	value;
	};


	struct VGraphicsPipelineRef
	{
		PipelineName			name;
		GraphicsPipelineDesc	desc;
	};
	

	struct VMeshPipelineRef
	{
		PipelineName			name;
		MeshPipelineDesc		desc;
	};
	

	struct VComputePipelineRef
	{
		PipelineName			name;
		ComputePipelineDesc		desc;
	};


	struct VMeshRef
	{
		// TODO
	};


	struct VMaterialRef
	{
		// TODO
	};


	/*struct VMeshletRef
	{
		// TODO
	};*/


	struct VRayTracingGeometryRef
	{
		// TODO
	};


	struct VVertexBuffers
	{
	};


	struct VDrawVertices
	{
		uint		vertexCount		= 0;
		//uint		instanceCount	= 1;
		uint		firstVertex		= 0;
		//uint		firstInstance	= 0;
	};


	struct VDrawIndexedVertices
	{
		VkBuffer		indexBuffer;
		VkDeviceSize	indexBufferOffset;

		uint			indexCount		= 0;
		//uint			instanceCount	= 1;
		uint			firstIndex		= 0;
		int				vertexOffset	= 0;
		//uint			firstInstance	= 0;
	};


	struct VDrawMeshes
	{
		uint		count	= 0;
		uint		first	= 0;
	};

	
	struct VDispatchCompute
	{
		uint3		groupCount;
	};


	struct VDispatchComputeBase
	{
		uint3		baseGroup;
		uint3		groupCount;
	};


}	// AE::ECS::Components

#endif	// AE_ENABLE_VULKAN
