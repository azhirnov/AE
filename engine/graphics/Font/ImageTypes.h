// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "graphics/Public/Ids.h"

namespace AE::Graphics
{

	//
	// Image in Atlas
	//
	struct ImageInAtlas
	{
		GfxResourceID	imageId;
		RectF			texcoord;
		float2			dimension;	// dips

		ImageInAtlas () {}

		ImageInAtlas (GfxResourceID imageId, const RectF &texcoord, const float2 &dimension) :
			imageId{imageId}, texcoord{texcoord}, dimension{dimension} {}
	};



	//
	// Nine Patch Image in Atlas
	//
	struct NinePatchImageInAtlas
	{
		GfxResourceID	imageId;
		RectF			texcoord;
		RectF			border;
		float2			dimension;	// dips

		NinePatchImageInAtlas () {}

		NinePatchImageInAtlas (GfxResourceID imageId, const RectF &texcoord, const RectF &border, const float2 &dimension) :
			imageId{imageId}, texcoord{texcoord}, border{border}, dimension{dimension} {}
	};



	//
	// Image Frame Animation
	//
	struct ImageFrameAnimation
	{
	};


}	// AE::Graphics
