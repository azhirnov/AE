// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "ecs-st/Common.h"
#include "graphics/Public/Common.h"
#include "graphics/Public/IDs.h"
#include "graphics/Public/ImageLayer.h"
#include "graphics/Public/MipmapLevel.h"

namespace AE::ECS::Renderer
{
	using namespace AE::Graphics;


	struct Config
	{
		//static constexpr uint	MaxFrames				= 3;
		//static constexpr uint	RenderPassPushConstSize	= 32;
		static constexpr uint	ObjectPushConstSize		= 128;
	};


	enum class ERenderPass : uint
	{
		Shadow,
		DepthPrePass,

		Background,
		Opaque_1,
		Opaque_2,
		Opaque_3,
		Foreground,

		_Custom
	};

	enum class ECameraType : uint
	{
		Primary,

		// secondary
		RayTracing,		// for hybrid rendering
		Shadow,
		Reflection,

		_Custom
	};


}	// AE::ECS::Renderer
