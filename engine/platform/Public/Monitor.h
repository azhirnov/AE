// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "platform/Public/Common.h"

namespace AE::App
{

	//
	// Coordinates and sizes
	//

	// TODO: use tags
	using Pixels2i		= int2;
	using Pixels2u		= uint2;
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
			Default	= ~0u,
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

		using Name_t			= FixedString< 64 >;
		using NativeMonitor_t	= void *;


	// variables
	public:
		ID					id;
		PixelsRectI			workArea;		// area available for window
		PixelsRectI			region;			// area available for fullscreen window only

		float2				physicalSize;	// in meters
		float2				ppi;			// pixels per inch
		float2				scale;			// content scale factor (Windows)
		uint				freq;			// update frequency in Hz
		EOrientation		orient;
		ubyte3				colorBits;		// r, g, b size in bits

		Name_t				name;
		NativeMonitor_t		native;


	// methods
	public:
		Monitor () : id{ID(~0u)}, freq{0}, orient{EOrientation::Default}, native{null} {}
		
		ND_ float	AspectRatio ()			const	{ return region.Width() / float(region.Height()); }
		ND_ bool	IsHorizontal ()			const	{ return region.Width() > region.Height(); }
		ND_ bool	IsVertical ()			const	{ return not IsHorizontal(); }

		ND_ float2	DipsPerPixel ()			const	{ return ppi / 160.0f; }
		ND_ float2	MillimetersPerPixel ()	const	{ return physicalSize / float2(region.Size()); }

		/*
		// converter
		ND_ Dips2f		PixelsToDips (const Pixels2f &c)	const	{ return Dips2f( c * (160.0f / ppi) ); }
		ND_ Dips2f		PixelsToDips (const Pixels2i &c)	const	{ return PixelsToDips( float2(c) ); }
		ND_ Dips2f		PixelsToDips (const Pixels2u &c)	const	{ return PixelsToDips( float2(c) ); }
		ND_ Dips2f		MetersToDips (const float2 &c)		const	{ return PixelsToDips( MetersToPixels( c )); }
		ND_ Dips2f		SNormToDips (const float2 &c)		const	{ return PixelsToDips( SNormToPixels( c )); }
		ND_ Dips2f		UNormToDips (const float2 &c)		const	{ return PixelsToDips( UNormToPixels( c )); }

		ND_ float2		DipsToMeters (const Dips2f &c)		const	{ return UNormToMeters( DipsToUNorm( c )); }
		ND_ float2		PixelsToMeters (const Pixels2f &c)	const	{ return UNormToMeters( PixelsToUNorm( c )); }
		ND_ float2		PixelsToMeters (const Pixels2i &c)	const	{ return UNormToMeters( PixelsToUNorm( c )); }
		ND_ float2		PixelsToMeters (const Pixels2u &c)	const	{ return UNormToMeters( PixelsToUNorm( c )); }
		ND_ float2		SNormToMeters (const float2 &c)		const	{ return UNormToMeters( c ); }
		ND_ float2		UNormToMeters (const float2 &c)		const	{ return c * physicalSize; }

		ND_ Pixels2f	DipsToPixels (const Dips2f &c)		const	{ return c * (ppi / 160.0f); }
		ND_ Pixels2f	MetersToPixels (const float2 &c)	const	{ return UNormToPixels( MetersToUNorm( c )); }
		ND_ Pixels2f	SNormToPixels (const float2 &c)		const	{ return UNormToPixels( c ); }
		ND_ Pixels2f	UNormToPixels (const float2 &c)		const	{ return c * float2(region.Size()); }

		ND_ float2		DipsToUNorm (const Dips2f &c)		const	{ return PixelsToUNorm( DipsToPixels( c )); }
		ND_ float2		MetersToUNorm (const float2 &c)		const	{ return c / physicalSize; }
		ND_ float2		PixelsToUNorm (const Pixels2f &c)	const	{ return c / float2(region.Size()); }
		ND_ float2		PixelsToUNorm (const Pixels2i &c)	const	{ return float2(c) / float2(region.Size()); }
		ND_ float2		PixelsToUNorm (const Pixels2u &c)	const	{ return float2(c) / float2(region.Size()); }
		
		ND_ float2		DipsToSNorm (const Dips2f &c)		const	{ return DipsToUNorm( c ); }
		ND_ float2		MetersToSNorm (const float2 &c)		const	{ return MetersToUNorm( c ); }
		ND_ float2		PixelsToSNorm (const Pixels2f &c)	const	{ return PixelsToUNorm( c ); }
		ND_ float2		PixelsToSNorm (const Pixels2i &c)	const	{ return PixelsToUNorm( float2(c) ); }
		ND_ float2		PixelsToSNorm (const Pixels2u &c)	const	{ return PixelsToUNorm( float2(c) ); }
		
		ND_ Pixels2i	ClampPixels (const Pixels2i &c)		const	{ return Math::Clamp( c, int2(0),   region.Size() ); }
		ND_ Pixels2f	ClampPixels (const Pixels2f &c)		const	{ return Math::Clamp( c, float2(0), float2(region.Size()) ); }
		ND_ float2		ClampMeters (const float2 &c)		const	{ return Math::Clamp( c, float2(0), physicalSize ); }
		ND_ Dips2f		ClampDips (const Dips2f &c)			const	{ return Math::Clamp( c, float2(0), PixelsToDips(region.Size()) ); }*/
	};


}	// AE::App
