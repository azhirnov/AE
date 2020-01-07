// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "graphics/Public/IDs.h"
#include "graphics/Public/RenderState.h"
#include "graphics/Public/VertexInputState.h"

namespace AE::Graphics
{

	//
	// Graphics Pipeline description
	//

	struct GraphicsPipelineDesc
	{
	// types
		using SpecValues_t	= FixedMap< SpecializationName, /*bool/int/uint/float*/uint, 8 >;
		
	// variables
		RenderPassID			renderPassId;
		uint					subpassIndex		= 0;
		VertexInputState		vertexInput;
		RenderState				renderState;
		EPipelineDynamicState	dynamicState		= EPipelineDynamicState::Viewport | EPipelineDynamicState::Scissor;
		SpecValues_t			specialization;

	// methods
		GraphicsPipelineDesc () {}

		ND_ bool	operator == (const GraphicsPipelineDesc &rhs) const;
		ND_ HashVal	CalcHash () const;
	};



	//
	// Mesh Pipeline description
	//

	struct MeshPipelineDesc
	{
	// types
		using SpecValues_t	= GraphicsPipelineDesc::SpecValues_t;

	// variables
		RenderPassID			renderPassId;
		uint					subpassIndex		= 0;
		RenderState				renderState;
		EPipelineDynamicState	dynamicState		= EPipelineDynamicState::Viewport | EPipelineDynamicState::Scissor;
		uint3					taskGroupSize;
		uint3					meshGroupSize;
		SpecValues_t			specialization;

	// methods
		MeshPipelineDesc () {}

		ND_ bool	operator == (const MeshPipelineDesc &rhs) const;
		ND_ HashVal	CalcHash () const;
	};



	//
	// Compute Pipeline description
	//

	struct ComputePipelineDesc
	{
	// types
		using SpecValues_t	= GraphicsPipelineDesc::SpecValues_t;

	// variables
		uint3				localGroupSize;
		SpecValues_t		specialization;

	// methods
		ComputePipelineDesc () {}

		ND_ bool	operator == (const ComputePipelineDesc &rhs) const;
		ND_ HashVal	CalcHash () const;
	};



	//
	// Ray Tracing Pipeline description
	//

	struct RayTracingPipelineDesc
	{
	// types
		using SpecValues_t	= GraphicsPipelineDesc::SpecValues_t;

	// variables
		SpecValues_t		specialization;

	// methods
		RayTracingPipelineDesc () {}

		ND_ bool	operator == (const RayTracingPipelineDesc &rhs) const;
		ND_ HashVal	CalcHash () const;
	};


}	// AE::Graphics
