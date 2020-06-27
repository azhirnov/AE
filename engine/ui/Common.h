// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Algorithms/EnumUtils.h"
#include "stl/Utils/NamedID.h"

#include "threading/Primitives/DataRaceCheck.h"

#include "graphics/Canvas/Canvas.h"
#include "graphics/Canvas/Primitives.h"
#include "graphics/Font/VectorFont.h"

#include "platform/Public/InputEventQueue.h"

namespace AE::UI
{
	using namespace AE::STL;

	using AE::Threading::RWDataRaceCheck;
	
	using AE::Graphics::EBlendMode;
	using AE::Graphics::PipelineName;
	using AE::Graphics::SurfaceDimensions;
	using AE::Graphics::Canvas;
	using AE::Graphics::FontPtr;
	using AE::Graphics::FontManager;
	using AE::Graphics::FontManagerPtr;
	using AE::Graphics::FontPtr;
	using AE::Graphics::IResourceManager;
	using AE::Graphics::GfxResourceID;

	using AE::Graphics::Rectangle2D;
	using AE::Graphics::Circle2D;
	using AE::Graphics::FilledCircle2D;
	using AE::Graphics::NinePatch2D;

	using AE::App::Nanoseconds;
	using AE::App::EKeyState;
	using AE::App::EKey;


	using UIActionID	= NamedID< 32, 0x20000001, AE_OPTIMIZE_IDS >;
	using UIActionName	= NamedID< 32, 0x20000001, false >;

	using StyleID		= NamedID< 32, 0x20000002, AE_OPTIMIZE_IDS >;
	using StyleName		= NamedID< 32, 0x20000002, false >;
	
	using ResourceName	= NamedID< 64, 0x20000003, true >;

}	// AE::UI
