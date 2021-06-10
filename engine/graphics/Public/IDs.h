// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "graphics/Public/Common.h"
#include "stl/Utils/HandleTmpl.h"
#include "stl/Utils/NamedID.h"

namespace AE::Graphics::_hidden_
{
	static constexpr uint	GraphicsIDs_Start	= 0x10000000;
	static constexpr uint	VulkanIDs_Start		= 0x20000000;

	static constexpr uint	NamedIDs_Start		= 0x10000000;

}	// AE::Graphics::_hidden_


namespace AE::Graphics
{
	using RenderPassID			= HandleTmpl< uint16_t, uint16_t, Graphics::_hidden_::GraphicsIDs_Start + 1 >;

	using GraphicsPipelineID	= HandleTmpl< uint16_t, uint16_t, Graphics::_hidden_::GraphicsIDs_Start + 4 >;
	using MeshPipelineID		= HandleTmpl< uint16_t, uint16_t, Graphics::_hidden_::GraphicsIDs_Start + 5 >;
	using ComputePipelineID		= HandleTmpl< uint16_t, uint16_t, Graphics::_hidden_::GraphicsIDs_Start + 6 >;
	using RayTracingPipelineID	= HandleTmpl< uint16_t, uint16_t, Graphics::_hidden_::GraphicsIDs_Start + 7 >;
	using DescriptorSetID		= HandleTmpl< uint16_t, uint16_t, Graphics::_hidden_::GraphicsIDs_Start + 8 >;
	using PipelinePackID		= HandleTmpl< uint16_t, uint16_t, Graphics::_hidden_::GraphicsIDs_Start + 9 >;
	using DescriptorSetLayoutID	= HandleTmpl< uint16_t, uint16_t, Graphics::_hidden_::GraphicsIDs_Start + 10 >;
	using BufferID				= HandleTmpl< uint16_t, uint16_t, Graphics::_hidden_::GraphicsIDs_Start + 11 >;
	using ImageID				= HandleTmpl< uint16_t, uint16_t, Graphics::_hidden_::GraphicsIDs_Start + 12 >;


	using UniformName			= NamedID< 32, Graphics::_hidden_::NamedIDs_Start + 1,  AE_OPTIMIZE_IDS >;
	using PushConstantName		= NamedID< 32, Graphics::_hidden_::NamedIDs_Start + 2,  AE_OPTIMIZE_IDS >;
	using SpecializationName	= NamedID< 32, Graphics::_hidden_::NamedIDs_Start + 3,  AE_OPTIMIZE_IDS >;
	using DescriptorSetName		= NamedID< 32, Graphics::_hidden_::NamedIDs_Start + 4,  AE_OPTIMIZE_IDS >;
	using PipelineName			= NamedID< 64, Graphics::_hidden_::NamedIDs_Start + 5,  AE_OPTIMIZE_IDS >;
	
	using VertexName			= NamedID< 32, Graphics::_hidden_::NamedIDs_Start + 6,  AE_OPTIMIZE_IDS >;
	using VertexBufferName		= NamedID< 32, Graphics::_hidden_::NamedIDs_Start + 7,  AE_OPTIMIZE_IDS >;

	using SamplerName			= NamedID< 64, Graphics::_hidden_::NamedIDs_Start + 8,  AE_OPTIMIZE_IDS >;
	using RenderTargetName		= NamedID< 32, Graphics::_hidden_::NamedIDs_Start + 9,  AE_OPTIMIZE_IDS >;
	using RenderPassName		= NamedID< 32, Graphics::_hidden_::NamedIDs_Start + 10, AE_OPTIMIZE_IDS >;


	static constexpr RenderTargetName	RenderTarget_Depth {"Depth"};
	static constexpr RenderTargetName	RenderTarget_DepthStencil {"DepthStencil"};

}	// AE::Graphics
