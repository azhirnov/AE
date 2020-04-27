// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "graphics/Font/ImageAtlas.h"

namespace AE::Graphics
{

/*
=================================================
	Create
=================================================
*/
	bool  ImageAtlas::Create (UniqueID<GfxResourceID> &&image, const uint2 &dim)
	{
		_imageId	= std::move(image);
		_imageDim	= dim;

		return bool(_imageId);
	}
	
/*
=================================================
	Destroy
=================================================
*/
	void  ImageAtlas::Destroy (IResourceManager &resMngr)
	{
		CHECK( resMngr.ReleaseResource( _imageId ));
	}

/*
=================================================
	Insert
=================================================
*/
	bool  ImageAtlas::Insert (const uint2 &dim, OUT GfxResourceID &id, OUT RectI &texcoord)
	{
		CHECK_ERR( dim.x <= MaxSize and dim.y <= MaxSize );

		uint	index = IntLog2( dim.y ) + uint(not IsPowerOfTwo( dim.y ));
		auto&	lines = _lineMap[index];

		for (auto& line : lines)
		{
			if ( dim.x <= (_imageDim.x - line.maxX) )
			{
				id				= _imageId;
				texcoord.left	= line.maxX;
				texcoord.top	= line.y;
				texcoord.right	= texcoord.left + dim.x;
				texcoord.bottom	= texcoord.top  + dim.y;

				line.maxX += dim.x;
				return true;
			}
		}

		// create new line
		if ( dim.y > (_imageDim.y - _maxY) )
			return false;
		
		ImageLine&	line	= lines.emplace_back();
		line.y		= _maxY;
		line.maxX	= dim.x;
		
		id				= _imageId;
		texcoord.left	= 0;
		texcoord.top	= line.y;
		texcoord.right	= texcoord.left + dim.x;
		texcoord.bottom	= texcoord.top  + dim.y;

		_maxY += (1u << index);
		return true;
	}
	
/*
=================================================
	Insert
=================================================
*/
	bool  ImageAtlas::Insert (const uint2 &dim, OUT GfxResourceID &id, OUT RectF &texcoord)
	{
		RectI	texc;
		CHECK_ERR( Insert( dim, OUT id, OUT texc ));

		texcoord = RectF{texc} / float2(_imageDim - 1u);
		return true;
	}
	
	bool  ImageAtlas::Insert (const uint2 &dim, OUT ImageInAtlas &image)
	{
		RectI	texc;
		CHECK_ERR( Insert( dim, OUT image.imageId, OUT texc ));

		image.dimension	= float2{dim};	// TODO: convert pixels to dips
		image.texcoord	= RectF{texc} / float2(_imageDim - 1u);
		return true;
	}

	bool  ImageAtlas::Insert (const uint2 &dim, OUT NinePatchImageInAtlas &image)
	{
		RectI	texc;
		CHECK_ERR( Insert( dim, OUT image.imageId, OUT texc ));
		
		image.dimension	= float2{dim};	// TODO: convert pixels to dips
		image.texcoord	= RectF{texc} / float2(_imageDim - 1u);
		return true;
	}

/*
=================================================
	Clear
=================================================
*/
	void  ImageAtlas::Clear ()
	{
		for (auto& lines : _lineMap) {
			lines.clear();
		}
		_maxY = 0;
	}


}	// AE::Graphics
