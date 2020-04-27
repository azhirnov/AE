// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "graphics/Public/Common.h"

namespace AE::Graphics
{

	//
	// Surface Dimensions
	//

	class SurfaceDimensions
	{
	// variables
	private:
		float2		_dipsToViewport;
		float2		_dipsToPixels;
		float2		_pixelsToDips;
		float2		_pixelsToPhysical;	// to mm
		float2		_unormPxToDips;
		float2		_invSurfaceSize;
		

	// methods
	public:
		SurfaceDimensions ();

		void SetDimensions (const uint2 &surfaceSizeInPix, const float2 &dipsPerPixel, const float2 &mmPerPixel);
		void CopyDimensions (const SurfaceDimensions &);

		ND_ float2	DipsToViewport (const float2 &dips) const;
		ND_ RectF	DipsToViewport (const RectF &dips) const;
		ND_ float2	DipsSizeToViewport (const float2 &dips) const;
		ND_ RectF	DipsSizeToViewport (const RectF &dips) const;

		ND_ float2	AlignDipsToPixel (const float2 &dips) const;
		ND_ RectF	AlignDipsToPixel (const RectF &dips) const;

		//ND_ float2	PixelsToPhysical (const float2 &value) const;

		ND_ float2	DipsToPixels (const float2 &value) const;
		ND_ RectF	DipsToPixels (const RectF &value) const;

		ND_ float2	PixelsToDips (const float2 &value) const;
		ND_ float2	PixelsToUNorm (const float2 &value) const;
		ND_ float2	UNormPixelsToDips (const float2 &value) const;

		ND_ float2	GetDipsPerPixel ()			const	{ return _dipsToPixels; }
		ND_ float2	GetMillimetersPerPixel ()	const	{ return _pixelsToPhysical; }
		ND_ float2	GetDipsToViewport ()		const	{ return _dipsToViewport; }
		ND_ float2	GetPixelsToViewport ()		const	{ return _pixelsToDips * _dipsToViewport; }
	};

	
/*
=================================================
	constructor
=================================================
*/
	inline SurfaceDimensions::SurfaceDimensions () :
		_dipsToPixels{1.0f},		_pixelsToDips{1.0f},
		_pixelsToPhysical{1.0f}
	{}

/*
=================================================
	DipsToViewport
=================================================
*/
	inline void  SurfaceDimensions::SetDimensions (const uint2 &surfaceSizeInPix, const float2 &dipsPerPixel, const float2 &mmPerPixel)
	{
		_invSurfaceSize		= 1.0f / float2(surfaceSizeInPix);
		_dipsToPixels		= dipsPerPixel;
		_pixelsToDips		= 1.0f / dipsPerPixel;
		_dipsToViewport		= 2.0f * _invSurfaceSize * dipsPerPixel;
		_pixelsToPhysical	= mmPerPixel;
		_unormPxToDips		= _invSurfaceSize * _pixelsToDips;
	}
	
/*
=================================================
	CopyDimensions
=================================================
*/
	inline void  SurfaceDimensions::CopyDimensions (const SurfaceDimensions &other)
	{
		_dipsToViewport		= other._dipsToViewport;
		_dipsToPixels		= other._dipsToPixels;
		_pixelsToDips		= other._pixelsToDips;
		_pixelsToPhysical	= other._pixelsToPhysical;
		_unormPxToDips		= other._unormPxToDips;
		_invSurfaceSize		= other._invSurfaceSize;
	}

/*
=================================================
	DipsToViewport
=================================================
*/
	forceinline float2  SurfaceDimensions::DipsToViewport (const float2 &dips) const
	{
		return float2{ dips.x * _dipsToViewport.x - 1.0f,
					   dips.y * _dipsToViewport.y - 1.0f };
	}

	forceinline RectF  SurfaceDimensions::DipsToViewport (const RectF &dips) const
	{
		return RectF{ dips.left   * _dipsToViewport.x - 1.0f,
					  dips.top    * _dipsToViewport.y - 1.0f,
					  dips.right  * _dipsToViewport.x - 1.0f,
					  dips.bottom * _dipsToViewport.y - 1.0f };
	}
	
/*
=================================================
	DipsSizeToViewport
=================================================
*/
	forceinline float2  SurfaceDimensions::DipsSizeToViewport (const float2 &dips) const
	{
		return float2{ dips.x * _dipsToViewport.x,
					   dips.y * _dipsToViewport.y };
	}

	forceinline RectF  SurfaceDimensions::DipsSizeToViewport (const RectF &dips) const
	{
		return RectF{ dips.left   * _dipsToViewport.x,
					  dips.top    * _dipsToViewport.y,
					  dips.right  * _dipsToViewport.x,
					  dips.bottom * _dipsToViewport.y };
	}

/*
=================================================
	AlignDipsToPixel
=================================================
*/
	forceinline float2  SurfaceDimensions::AlignDipsToPixel (const float2 &dips) const
	{
		return float2{ Round( dips.x * _dipsToPixels.x ) * _pixelsToDips.x,
					   Round( dips.y * _dipsToPixels.y ) * _pixelsToDips.y };
	}
	
	forceinline RectF  SurfaceDimensions::AlignDipsToPixel (const RectF &dips) const
	{
		return RectF{ Round( dips.left   * _dipsToPixels.x ) * _pixelsToDips.x,
					  Round( dips.top    * _dipsToPixels.y ) * _pixelsToDips.y,
					  Round( dips.right  * _dipsToPixels.x ) * _pixelsToDips.x,
					  Round( dips.bottom * _dipsToPixels.y ) * _pixelsToDips.y };
	}
	
/*
=================================================
	PixelsToUNorm
=================================================
*/
	forceinline float2  SurfaceDimensions::PixelsToUNorm (const float2 &value) const
	{
		return value * _invSurfaceSize;
	}
	
/*
=================================================
	PixelsToDips
=================================================
*/
	forceinline float2  SurfaceDimensions::PixelsToDips (const float2 &value) const
	{
		return value * _pixelsToDips;
	}

/*
=================================================
	UNormPixelsToDips
=================================================
*/
	forceinline float2  SurfaceDimensions::UNormPixelsToDips (const float2 &value) const
	{
		return value * _unormPxToDips;
	}
	
/*
=================================================
	DipsToPixels
=================================================
*/
	forceinline float2  SurfaceDimensions::DipsToPixels (const float2 &value) const
	{
		return value * _dipsToPixels;
	}
	
	forceinline RectF  SurfaceDimensions::DipsToPixels (const RectF &value) const
	{
		return value * _dipsToPixels;
	}


}	// AE::Graphics
