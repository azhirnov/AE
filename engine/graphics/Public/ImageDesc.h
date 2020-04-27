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
		EImageFlags			flags		= Default;
		uint3				dimension;	// width, height, depth
		EPixelFormat		format		= Default;
		EImageUsage			usage		= Default;
		ImageLayer			arrayLayers	= 1_layer;
		MipmapLevel			maxLevel	= 1_mipmap;
		MultiSamples		samples;	// if > 1 then enabled multisampling
		EQueueMask			queues		= Default;
		EMemoryType			memType		= EMemoryType::DeviceLocal;

	// methods
		ImageDesc () {}
		
		ImageDesc (EImage		imageType,
				   const uint3 &dimension,
				   EPixelFormat	format,
				   EImageUsage	usage,
				   EImageFlags	flags		= Default,
				   ImageLayer	arrayLayers	= 1_layer,
				   MipmapLevel	maxLevel	= 1_mipmap,
				   MultiSamples	samples		= Default,
				   EQueueMask	queues		= Default,
				   EMemoryType	memType		= EMemoryType::DeviceLocal);

		void Validate ();

		ImageDesc&  SetType (EImage value)						{ imageType = value;  return *this; }
		ImageDesc&  SetFlags (EImageFlags value)				{ flags = value;  return *this; }
		ImageDesc&  SetDimension (const uint value)				{ return SetDimension( uint3{ value, 0, 0 }); }
		ImageDesc&  SetDimension (const uint2 &value)			{ return SetDimension( uint3{ value, 0 }); }
		ImageDesc&  SetDimension (const uint3 &value)			{ dimension = value;  return *this; }
		ImageDesc&  SetUsage (EImageUsage value)				{ usage = value;  return *this; }
		ImageDesc&  SetFormat (EPixelFormat value)				{ format = value;  return *this; }
		ImageDesc&  SetArrayLayers (uint value)					{ arrayLayers = ImageLayer{value};  return *this; }
		ImageDesc&  SetMaxMipmaps (uint value)					{ maxLevel = MipmapLevel{value};  return *this; }
		ImageDesc&  SetSamples (uint value)						{ samples = MultiSamples{value};  return *this; }
		ImageDesc&  SetQueueMask (EQueueMask value)				{ queues = value;  return *this; }
		ImageDesc&  SetMemoryType (EMemoryType value)			{ memType = value;  return *this; }
		ImageDesc&  SetDimensionAndType (const uint value)		{ return SetDimensionAndType( uint3{ value, 0, 0 }); }
		ImageDesc&  SetDimensionAndType (const uint2 &value)	{ return SetDimensionAndType( uint3{ value, 0 }); }
		ImageDesc&  SetDimensionAndType (const uint3 &value);
	};
		

	//
	// Virtual Image Description
	//

	struct VirtualImageDesc
	{
	// variables
		EImage				imageType		= Default;
		EImageFlags			flags			= Default;
		EPixelFormat		format			= Default;
		uint3				dimension		{ ~0u };		// width, height, depth;  ~0u - viewport size
		FractionalI16		dimScale		{1, 1};			// dimension = viewport_size * scale
		ImageLayer			arrayLayers		= 1_layer;
		MipmapLevel			maxLevel		= 1_mipmap;
		MultiSamples		samples;						// if > 1 then enabled multisampling
		
	// methods
		VirtualImageDesc () {}

		VirtualImageDesc&  SetType (EImage value)						{ imageType = value;  return *this; }
		VirtualImageDesc&  SetFlags (EImageFlags value)					{ flags = value;  return *this; }
		VirtualImageDesc&  SetDimension (const uint value)				{ return SetDimension( uint3{ value, 0, 0 }); }
		VirtualImageDesc&  SetDimension (const uint2 &value)			{ return SetDimension( uint3{ value, 0 }); }
		VirtualImageDesc&  SetDimension (const uint3 &value)			{ dimension = value;  return *this; }
		VirtualImageDesc&  SetFormat (EPixelFormat value)				{ format = value;  return *this; }
		VirtualImageDesc&  SetArrayLayers (uint value)					{ arrayLayers = ImageLayer{value};  return *this; }
		VirtualImageDesc&  SetMaxMipmaps (uint value)					{ maxLevel = MipmapLevel{value};  return *this; }
		VirtualImageDesc&  SetSamples (uint value)						{ samples = MultiSamples{value};  return *this; }
		VirtualImageDesc&  SetDimensionAndType (const uint value)		{ return SetDimensionAndType( uint3{ value, 0, 0 }); }
		VirtualImageDesc&  SetDimensionAndType (const uint2 &value)		{ return SetDimensionAndType( uint3{ value, 0 }); }
		VirtualImageDesc&  SetDimensionAndType (const uint3 &value);

		ND_ ImageDesc  ToPhysical (uint2 viewportSize, EVirtualResourceUsage usage) const;
	};


	//
	// Image View Description
	//

	struct ImageViewDesc
	{
	// variables
		EImageView			viewType	= Default;
		EPixelFormat		format		= Default;
		MipmapLevel			baseLevel;
		uint				levelCount	= UMax;
		ImageLayer			baseLayer;
		uint				layerCount	= UMax;
		ImageSwizzle		swizzle;
		EImageAspect		aspectMask	= Default;

	// methods
		ImageViewDesc () {}

		ImageViewDesc (EImageView		viewType,
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
		
		ImageViewDesc&  SetViewType (EImageView value)			{ viewType = value;  return *this; }
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
	struct hash< AE::Graphics::ImageViewDesc > {
		ND_ size_t  operator () (const AE::Graphics::ImageViewDesc &value) const;
	};

}	// std
