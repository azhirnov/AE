// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "graphics/Public/ResourceManager.h"
#include "graphics/Font/ImageTypes.h"
#include "stl/CompileTime/Math.h"

namespace AE::Graphics
{

	//
	// Image Atlas
	//

	class ImageAtlas
	{
	// types
	private:
		static constexpr uint	MaxSize		= 512;
		static constexpr uint	MaxSizePOT	= CT_IntLog2< MaxSize >;

		struct ImageLine
		{
			uint	y		= UMax;
			uint	maxX	= 0;
		};

		using ImageLines_t	= Array< ImageLine >;
		using LineMap_t		= StaticArray< ImageLines_t, MaxSizePOT >;


	// variables
	private:
		LineMap_t					_lineMap;
		UniqueID<GfxResourceID>		_imageId;
		uint2						_imageDim;
		uint						_maxY	= 0;


	// methods
	public:
		ImageAtlas () {}
		ImageAtlas (const ImageAtlas &) = delete;
		ImageAtlas (ImageAtlas &&) = default;

		bool  Create (UniqueID<GfxResourceID> &&image, const uint2 &dim);
		void  Destroy (IResourceManager &);

		bool  Insert (const uint2 &dim, OUT GfxResourceID &id, OUT RectI &texcoord);
		bool  Insert (const uint2 &dim, OUT GfxResourceID &id, OUT RectF &texcoord);
		bool  Insert (const uint2 &dim, OUT ImageInAtlas &image);
		bool  Insert (const uint2 &dim, OUT NinePatchImageInAtlas &image);
		void  Clear ();

		ND_ GfxResourceID	Image ()		const	{ return _imageId; }
		ND_ uint2			Dimension ()	const	{ return _imageDim; }
	};


}	// AE::Graphics
