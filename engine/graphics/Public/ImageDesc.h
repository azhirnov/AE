// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

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
		EImage				imageType	= Default;
		uint3				dimension;	// width, height, depth
		EPixelFormat		format		= Default;
		EImageUsage			usage		= Default;
		ImageLayer			arrayLayers;
		MipmapLevel			maxLevel;
		MultiSamples		samples;	// if > 1 then enabled multisampling
		EQueueMask			queues		= Default;
		EMemoryType			memType		= Default;

	// methods
		ImageDesc () {}
		
		ImageDesc (EImage		imageType,
				   const uint3 &dimension,
				   EPixelFormat	format,
				   EImageUsage	usage,
				   ImageLayer	arrayLayers	= Default,
				   MipmapLevel	maxLevel	= Default,
				   MultiSamples	samples		= Default,
				   EQueueMask	queues		= Default,
				   EMemoryType	memType		= Default);

		void Validate ();

		ImageDesc&  SetType (EImage value)				{ imageType = value;  return *this; }
		ImageDesc&  SetDimension (const uint2 &value)	{ dimension = uint3{ value, 1 };  return *this; }
		ImageDesc&  SetDimension (const uint3 &value)	{ dimension = value;  return *this; }
		ImageDesc&  SetUsage (EImageUsage value)		{ usage = value;  return *this; }
		ImageDesc&  SetFormat (EPixelFormat value)		{ format = value;  return *this; }
		ImageDesc&  SetArrayLayers (uint value)			{ arrayLayers = ImageLayer{value};  return *this; }
		ImageDesc&  SetMaxMipmaps (uint value)			{ maxLevel = MipmapLevel{value};  return *this; }
		ImageDesc&  SetSamples (uint value)				{ samples = MultiSamples{value};  return *this; }
		ImageDesc&  SetQueueMask (EQueueMask value)		{ queues = value;  return *this; }
		ImageDesc&  SetMemoryType (EMemoryType value)	{ memType = value;  return *this; }
	};
		

	//
	// Virtual Image Description
	//

	struct VirtualImageDesc
	{
	// variables
		EImage				imageType	= Default;
		EPixelFormat		format		= Default;
		uint3				dimension	{0};		// width, height, depth;  0 - viewport size
		FractionalI16		dimScale	{1, 0};		// dimension = viewport_size * scale
		ImageLayer			arrayLayers;
		MipmapLevel			maxLevel;
		MultiSamples		samples;				// if > 1 then enabled multisampling
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
		uint				levelCount	= 1;
		ImageLayer			baseLayer;
		uint				layerCount	= 1;
		ImageSwizzle		swizzle;
		EImageAspect		aspectMask	= Default;

	// methods
		ImageViewDesc () {}

		ImageViewDesc (EImage			viewType,
					   EPixelFormat		format,
					   MipmapLevel		baseLevel	= Default,
					   uint				levelCount	= 1,
					   ImageLayer		baseLayer	= Default,
					   uint				layerCount	= 1,
					   ImageSwizzle		swizzle		= Default,
					   EImageAspect		aspectMask	= Default);

		explicit ImageViewDesc (const ImageDesc &desc);

		void Validate (const ImageDesc &desc);

		ND_ bool operator == (const ImageViewDesc &rhs) const;
		
		ImageViewDesc&  SetViewType (EImage value)				{ viewType = value;  return *this; }
		ImageViewDesc&  SetFormat (EPixelFormat value)			{ format = value;  return *this; }
		ImageViewDesc&  SetBaseLevel (uint value)				{ baseLevel = MipmapLevel{value};  return *this; }
		ImageViewDesc&  SetLevels (uint base, uint count)		{ baseLevel = MipmapLevel{base};  levelCount = count;  return *this; }
		ImageViewDesc&  SetBaseLayer (uint value)				{ baseLayer = ImageLayer{value};  return *this; }
		ImageViewDesc&  SetArrayLayers (uint base, uint count)	{ baseLayer = ImageLayer{base};  layerCount = count;  return *this; }
		ImageViewDesc&  SetSwizzle (ImageSwizzle value)			{ swizzle = value;  return *this; }
		ImageViewDesc&  SetAspect (EImageAspect value)			{ aspectMask = value;  return *this; }
	};


}	// AE::Graphics


namespace std
{
	template <>
	struct hash< AE::Graphics::ImageDesc >
	{
		ND_ size_t  operator () (const AE::Graphics::ImageDesc &value) const
		{
		#if AE_FAST_HASH
			return size_t(AE::STL::HashOf( AddressOf(value), sizeof(value) ));
		#else
			AE::STL::HashVal	result;
			result << AE::STL::HashOf( value.imageType );
			result << AE::STL::HashOf( value.dimension );
			result << AE::STL::HashOf( value.format );
			result << AE::STL::HashOf( value.usage );
			result << AE::STL::HashOf( value.maxLevel );
			result << AE::STL::HashOf( value.samples );
			result << AE::STL::HashOf( value.queues );
			return size_t(result);
		#endif
		}
	};
	

	template <>
	struct hash< AE::Graphics::ImageViewDesc >
	{
		ND_ size_t  operator () (const AE::Graphics::ImageViewDesc &value) const
		{
		#if AE_FAST_HASH
			return size_t(AE::STL::HashOf( AddressOf(value), sizeof(value) ));
		#else
			AE::STL::HashVal	result;
			result << AE::STL::HashOf( value.viewType );
			result << AE::STL::HashOf( value.format );
			result << AE::STL::HashOf( value.baseLevel );
			result << AE::STL::HashOf( value.levelCount );
			result << AE::STL::HashOf( value.baseLayer );
			result << AE::STL::HashOf( value.layerCount );
			result << AE::STL::HashOf( value.swizzle );
			return size_t(result);
		#endif
		}
	};

}	// std

