// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "graphics/Public/MipmapLevel.h"
#include "graphics/Public/MultiSamples.h"
#include "graphics/Public/ImageLayer.h"
#include "graphics/Public/ImageSwizzle.h"
#include "graphics/Public/ResourceEnums.h"
#include "graphics/Public/Queue.h"
#include "stl/Math/Fractional.h"

namespace AE::Graphics
{

	//
	// Image Description
	//

	struct ImageDesc
	{
	// variables
		EImageDim			imageType	= Default;
		EImage				viewType	= Default;	// optional
		EImageFlags			flags		= Default;
		uint3				dimension;				// width, height, depth
		EPixelFormat		format		= Default;
		EImageUsage			usage		= Default;
		ImageLayer			arrayLayers	= 1_layer;
		MipmapLevel			maxLevel	= 1_mipmap;
		MultiSamples		samples;				// if > 1 then enabled multisampling
		EQueueMask			queues		= Default;
		EMemoryType			memType		= EMemoryType::DeviceLocal;

	// methods
		ImageDesc () {}

		void Validate ();
		
		ImageDesc&  SetType (EImageDim value)			{ imageType		= value;				return *this; }
		ImageDesc&  SetView (EImage value);
		ImageDesc&  AddFlags (EImageFlags value)		{ flags			|= value;				return *this; }
		ImageDesc&  SetDimension (const uint value);
		ImageDesc&  SetDimension (const uint2 &value);
		ImageDesc&  SetDimension (const uint3 &value);
		ImageDesc&  SetUsage (EImageUsage value)		{ usage			= value;				return *this; }
		ImageDesc&  SetFormat (EPixelFormat value)		{ format		= value;				return *this; }
		ImageDesc&  SetArrayLayers (uint value)			{ arrayLayers	= ImageLayer{value};	return *this; }
		ImageDesc&  SetMaxMipmaps (uint value)			{ maxLevel		= MipmapLevel{value};	return *this; }
		ImageDesc&  SetSamples (uint value)				{ samples		= MultiSamples{value};	return *this; }
		ImageDesc&  SetAllMipmaps ()					{ maxLevel		= MipmapLevel{~0u};		return *this; }
		ImageDesc&  SetQueues (EQueueMask value)		{ queues		= value;				return *this; }
	};
	


	//
	// Image View Description
	//

	struct ImageViewDesc
	{
	// variables
		EImage				viewType	= Default;
		EPixelFormat		format		= Default;
		MipmapLevel			baseLevel;
		uint				levelCount	= UMax;
		ImageLayer			baseLayer;
		uint				layerCount	= UMax;
		ImageSwizzle		swizzle;
		EImageAspect		aspectMask	= Default;

	// methods
		ImageViewDesc () {}

		explicit ImageViewDesc (EImage			viewType,
								EPixelFormat	format		= Default,
								MipmapLevel		baseLevel	= Default,
								uint			levelCount	= UMax,
								ImageLayer		baseLayer	= Default,
								uint			layerCount	= UMax,
								ImageSwizzle	swizzle		= Default,
								EImageAspect	aspectMask	= Default);

		explicit ImageViewDesc (const ImageDesc &desc);

		void Validate (const ImageDesc &desc);

		ND_ bool operator == (const ImageViewDesc &rhs) const;
		
		ImageViewDesc&  SetType (EImage value)					{ viewType	= value;				return *this; }
		ImageViewDesc&  SetFormat (EPixelFormat value)			{ format	= value;				return *this; }
		ImageViewDesc&  SetBaseMipmap (uint value)				{ baseLevel	= MipmapLevel{value};	return *this; }
		ImageViewDesc&  SetLevels (uint base, uint count)		{ baseLevel	= MipmapLevel{base};	levelCount = count;  return *this; }
		ImageViewDesc&  SetBaseLayer (uint value)				{ baseLayer	= ImageLayer{value};	return *this; }
		ImageViewDesc&  SetArrayLayers (uint base, uint count)	{ baseLayer	= ImageLayer{base};		layerCount = count;  return *this; }
		ImageViewDesc&  SetSwizzle (ImageSwizzle value)			{ swizzle	= value;				return *this; }
		ImageViewDesc&  SetAspect (EImageAspect value)			{ aspectMask= value;				return *this; }
	};


}	// AE::Graphics


namespace std
{
	template <>
	struct hash< AE::Graphics::ImageViewDesc > {
		ND_ size_t  operator () (const AE::Graphics::ImageViewDesc &value) const;
	};

}	// std
