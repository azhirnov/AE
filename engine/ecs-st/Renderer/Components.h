// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "ecs-st/Renderer/Common.h"
#include "stl/Math/Rectangle.h"
#include "stl/Math/Matrix.h"

namespace AE::ECS::Components
{
	using namespace AE::Graphics;


	template <Renderer::ERenderPass Pass>
	struct RenderPassTag
	{};

	struct InvalidatePipelineTag
	{};
//-----------------------------------------------------------------------------
	

	struct Camera2D
	{
		float3x3		view;
	};

	struct Camera3D
	{
		float4x4		view;
		float4x4		projection;
	};

	struct Camera3DShadow
	{
	};

	struct CameraVR
	{
		struct PerEye {
			float4x4	proj;
			float4x4	view;
		};
		PerEye		left;
		PerEye		right;
		float3x3	headRotation;
		float3		headPosition;
	};

	struct CameraVR180
	{
		float3x3	headRotation;
		float3		headPosition;
	};

	struct CameraVR360
	{
		float3x3	headRotation;
		float3		headPosition;
	};

	template <Renderer::ECameraType Type>
	struct CameraTag
	{};

	struct CameraSystemTag
	{};

	struct Viewport
	{
		GfxResourceID	image;
		ImageLayer		layer;
		MipmapLevel		mipmap;
		RectU			region;
	};
//-----------------------------------------------------------------------------



	/*struct DrawCallRenderState
	{
		Renderer::EStencilOp	stencilFailOp;					// stencil test failed
		Renderer::EStencilOp	stencilDepthFailOp;				// depth and stencil tests are passed
		Renderer::EStencilOp	stencilPassOp;					// stencil test passed and depth test failed
		uint8_t					stencilReference;				// front & back
		uint8_t					stencilWriteMask;				// front & back
		uint8_t					stencilCompareMask;				// front & back

		Renderer::ECullMode		cullMode;

		Renderer::ECompareOp	depthCompareOp;
		bool					depthTest				: 1;
		bool					depthWrite				: 1;
		bool					stencilTest				: 1;	// enable stencil test

		bool					rasterizerDiscard		: 1;
		bool					frontFaceCCW			: 1;

		bool					hasStencilTest			: 1;
		bool					hasStencilFailOp		: 1;
		bool					hasStencilDepthFailOp	: 1;
		bool					hasStencilPassOp		: 1;
		bool					hasStencilReference		: 1;
		bool					hasStencilWriteMask		: 1;
		bool					hasStencilCompareMask	: 1;
		bool					hasDepthCompareOp		: 1;
		bool					hasDepthTest			: 1;
		bool					hasDepthWrite			: 1;
		bool					hasCullMode				: 1;
		bool					hasRasterizedDiscard	: 1;
		bool					hasFrontFaceCCW			: 1;
		bool					primitiveRestart		: 1;	// if 'true' then index with -1 value will restarting the assembly of primitives

		DrawCallRenderState ()
		{
			memset( this, 0, sizeof(*this) );
		}
	};*/


	struct DrawWithScissor
	{
		RectI		value;
	};
	

	struct GraphicsShaderDebugMode
	{
		// TODO
	};

	struct ComputeShaderDebugMode
	{
		// TODO
	};

	struct RayTracingShaderDebugMode
	{
		//EShaderDebugMode	mode		= Default;
		//uint3				launchID	{ ~0u };
	};

}	// AE::ECS::Components
