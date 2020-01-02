// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "platform/App/Public/Common.h"

namespace AE::App
{

	//
	// Coordinates and sizes
	//

	// TODO: use tags
	using Pixels2i		= int2;
	using Pixels2f		= float2;
	using PixelsRectI	= RectI;
	using PixelsRectF	= RectF;

	using Dips2f		= float2;
	using DipsRectF		= RectF;



	//
	// Monitor
	//
	struct Monitor
	{
	// types
	public:
		enum class ID : uint
		{
			Default	= 0,
			Unknown	= Default,
		};

		enum class EOrientation
		{
			Default				= -1,
			Sensor				= -2,
			Portrait			= 0,
			PortraitReverse		= 180,
			Landscape			= 90,
			LandscapeReverse	= 270,
			
			Orient_0_deg		= Portrait,
			Orient_90_deg		= Landscape,
			Orient_180_deg		= PortraitReverse,
			Orient_270_deg		= LandscapeReverse,
		};

		using Name_t = StaticString< 64 >;


	// variables
	public:
		ID				id;
		PixelsRectI		workArea;		// area available for window
		PixelsRectI		region;			// area available for fullscreen window only

		float2			physicalSize;	// in meters
		float2			ppi;			// pixels per inch
		float2			scale;			// content scale factor (Windows)
		uint			freq;			// update frequency in Hz
		EOrientation	orient;
		ubyte3			colorBits;		// r, g, b size in bits

		Name_t			name;


	// methods
	public:
		Monitor () : id{ID(~0u)}, freq{0}, orient{EOrientation::Default} {}
		
		ND_ float	AspectRatio ()	const	{ return region.Width() / float(region.Height()); }
		ND_ bool	IsHorizontal ()	const	{ return region.Width() > region.Height(); }
		ND_ bool	IsVertical ()	const	{ return not IsHorizontal(); }
	};


}	// AE::App
