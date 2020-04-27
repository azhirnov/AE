// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "graphics/Font/FontManager.h"
#include "graphics/Font/VectorFont.h"
#include "graphics/Font/FreeType.h"
#include "stl/Stream/FileStream.h"
#include "stl/Stream/MemStream.h"
#include "stl/Algorithms/StringUtils.h"

#ifndef AE_ENABLE_FREETYPE
# error AE_ENABLE_FREETYPE required
#endif

namespace AE::Graphics
{

/*
=================================================
	constructor
=================================================
*/
	FontManager::FontManager (IResourceManager &resMngr) :
		_resMngr{ resMngr },
		_ftLibrary{ null }
	{
		EXLOCK( _drCheck );

		CHECK( FT_Init_FreeType( BitCast<FT_Library*>(&_ftLibrary) ) == 0 );

		_allocator.SetBlockSize( 64_Kb );
	}
	
/*
=================================================
	destructor
=================================================
*/
	FontManager::~FontManager ()
	{
		EXLOCK( _drCheck );

		_Destroy();
	}
	
/*
=================================================
	_Destroy
=================================================
*/
	void  FontManager::_Destroy ()
	{
		for (auto& atlas : _atlases) {
			atlas.Destroy( _resMngr );
		}
		_atlases.clear();

		if ( _ftLibrary != null )
		{
			FT_Done_FreeType( BitCast<FT_Library>(_ftLibrary) );
			_ftLibrary = null;
		}
	}

/*
=================================================
	Flush
=================================================
*/
	bool  FontManager::Flush (ITransferContext &ctx)
	{
		EXLOCK( _drCheck );

		FixedArray< ITransferContext::BufferImageCopy, 64 >		copy;

		for (auto& atlas : _atlases)
		{
			if ( atlas.initialState )
			{
				ctx.SetInitialState( atlas.Image(), EResourceState::Unknown );
				atlas.initialState = false;
			}

			if ( atlas.pendingGlyphs.size() )
			{
				GfxResourceID	curr_buf;

				for (auto& glyph : atlas.pendingGlyphs)
				{
					GfxResourceID	buf_id;
					BytesU			buf_offset;
					void *			mapped;
					uint			size	= glyph.extent.x * glyph.extent.y * BytesPerPixel;

					CHECK_ERR( ctx.AllocBuffer( BytesU{size}, 1_b, OUT buf_id, OUT buf_offset, OUT mapped ));

					if ( (curr_buf != buf_id or copy.size() == copy.capacity()) and copy.size() )
					{
						ctx.CopyBufferToImage( curr_buf, atlas.Image(), copy );
						copy.clear();
					}

					curr_buf = buf_id;
					std::memcpy( OUT mapped, glyph.memory, size );

					auto&	dst = copy.emplace_back();
					dst.bufferOffset		= buf_offset;
					dst.bufferRowLength		= glyph.extent.x;
					dst.bufferImageHeight	= glyph.extent.y;
					dst.imageSubresource.aspectMask = EImageAspect::Color;
					dst.imageOffset			= uint3{ glyph.offset.x, glyph.offset.y, 0 };
					dst.imageExtent			= uint3{ glyph.extent.x, glyph.extent.y, 1 };
				}

				if ( copy.size() )
				{
					ctx.CopyBufferToImage( curr_buf, atlas.Image(), copy );
					copy.clear();
				}

				atlas.pendingGlyphs.clear();
			}
		}

		_allocator.Discard();
		return true;
	}

/*
=================================================
	Load
=================================================
*/
	FontPtr  FontManager::Load (const SharedPtr<RStream> &stream)
	{
		CHECK_ERR( stream and stream->IsOpen() );
		EXLOCK( _drCheck );
		
		const BytesU	size = stream->RemainingSize();

		Array<uint8_t>	font_data;
		CHECK_ERR( stream->Read( size_t(size), OUT font_data ));

		FT_Face		face = null;
		CHECK_ERR( FT_New_Memory_Face( BitCast<FT_Library>(_ftLibrary), font_data.data(), FT_Long(font_data.size()), 0, OUT &face ) == 0 );
		
		CHECK_ERR( FT_Select_Charmap( face, FT_ENCODING_UNICODE ) == 0 );

		return MakeRC<VectorFont>( GetRC(), BitCast<FT_Face_t>(face), MakeUnique<MemRStream>( std::move(font_data) ));
	}
	
/*
=================================================
	Load
=================================================
*/
	FontPtr  FontManager::Load (const Path &path)
	{
		auto	file = MakeShared<FileRStream>( path );
		return Load( file );
	}

/*
=================================================
	_LoadGlyph
=================================================
*/
	bool  FontManager::_LoadGlyph (FT_Face_t tface, const GlyphKey &key, OUT Glyph &glyph)
	{
		EXLOCK( _drCheck );
		//ASSERT( IsPowerOfTwo( key.height ));
		
		if ( _CreateEmptyGlyph( key, OUT glyph ))
			return true;
		
		// setup freetype
		FT_Face		face = BitCast<FT_Face>( tface );
		CHECK_ERR( FT_Set_Pixel_Sizes( face, key.height, key.height ) == 0 );
		
		FT_UInt		char_index = FT_Get_Char_Index( face, key.unicodeChar );
		ASSERT( char_index != 0 );
		CHECK_ERR( FT_Load_Glyph( face, char_index, FT_LOAD_RENDER | FT_LOAD_FORCE_AUTOHINT ) == 0 );
		
		if ( face->glyph->bitmap.buffer == null )
			RETURN_ERR( "Symbol '"s << ToString(key.unicodeChar) << "' hasn't bitmap" );

		// calculate font metrics
		int ascent  =  face->size->metrics.ascender >> 6;
		int descent = -face->size->metrics.descender >> 6;

		if ( TT_OS2* os2 = Cast<TT_OS2>(FT_Get_Sfnt_Table( face, ft_sfnt_os2 )) )
		{
			ascent	= Max( ascent, os2->usWinAscent * face->size->metrics.y_ppem / face->units_per_EM );
			descent	= Max( descent, os2->usWinDescent * face->size->metrics.y_ppem / face->units_per_EM );

			ascent	= Max( ascent, os2->sTypoAscender * face->size->metrics.y_ppem / face->units_per_EM );
			descent	= Max( descent, -os2->sTypoDescender * face->size->metrics.y_ppem / face->units_per_EM );
		}

		FT_Load_Glyph( face, char_index, FT_LOAD_RENDER | FT_LOAD_NO_HINTING );	// TODO

		const uint	w			= face->glyph->bitmap.width;
		const uint	h			= face->glyph->bitmap.rows;
		const uint	aligned_w	= AlignToLarger( w + GlyphMarginX*2, 4u );
		const uint	aligned_h	= AlignToLarger( h + GlyphMarginY*2, 4u );
		const uint	img_size	= aligned_w * aligned_h;
		void *		data_ptr	= _allocator.Alloc( BytesU{img_size}, 1_b );

		// copy glyph data
		memset( OUT data_ptr, 0, img_size );

		for (uint j = 0; j < h; ++j)
		{
			std::memcpy( OUT data_ptr + BytesU{ (j + GlyphMarginY) * aligned_w + GlyphMarginX},
						 face->glyph->bitmap.buffer + BytesU{j*w},
						 w );
		}

		// allocate image region
		GfxResourceID	image_id;
		RectI			image_rect;
		uint2			atlas_dim;
		CHECK_ERR( _AllocGlyphImage( uint2{aligned_w, aligned_h}, data_ptr, OUT image_id, OUT image_rect, OUT atlas_dim ));

		glyph.imageId		= image_id;
		glyph.texcoord		= (RectF{{float(w), float(h)}} + float2{float(GlyphMarginX + image_rect.left), float(GlyphMarginY + image_rect.top)}) / float2{atlas_dim - 1u};
		glyph.offsetY		= float(face->glyph->bitmap_top);
		glyph.offsetX[0]	= float(face->glyph->bitmap_left);
		glyph.offsetX[1]	= (face->glyph->advance.x / 64.0f - int(w) - face->glyph->bitmap_left);
		glyph.size			= float2{uint2{ w, h }};
		
		return true;
	}
	
/*
=================================================
	_CreateEmptyGlyph
=================================================
*/
	bool  FontManager::_CreateEmptyGlyph (const GlyphKey &key, OUT Glyph &glyph)
	{
		const float	space_width = 0.5f;

		switch ( key.unicodeChar )
		{
			case ' ' :
				glyph = Glyph{ key.height * space_width };
				return true;

			case '\t' :
				glyph = Glyph{ key.height * 4 * space_width };
				return true;
		}
		return false;
	}
	
/*
=================================================
	_AllocGlyphImage
=================================================
*/
	bool  FontManager::_AllocGlyphImage (const uint2 &alignedDim, const void* glypgData, OUT GfxResourceID &imageId, OUT RectI &texcoord, OUT uint2 &atlasDim)
	{
		const uint2	img_dim	{ AtlasWidth, AtlasHeight };

		atlasDim = img_dim;

		const auto	AddGlyph = [glypgData] (AtlasData &atlas, const RectI &texcoord)
		{
			auto&	glyph = atlas.pendingGlyphs.emplace_back();
			glyph.offset	= ushort2(texcoord.LeftTop());
			glyph.extent	= ushort2(texcoord.Size());
			glyph.memory	= glypgData;
		};

		// search in existing image atlasses
		for (auto& atlas : _atlases)
		{
			if ( atlas.Insert( alignedDim, OUT imageId, OUT texcoord ))
			{
				AddGlyph( atlas, texcoord );
				return true;
			}
		}

		// create new atlas
		auto	img_id	= _resMngr.CreateImage( ImageDesc{ EImage::_2D, uint3{img_dim, 1}, PixelFormat,
														   EImageUsage::Transfer | EImageUsage::Storage | EImageUsage::Sampled },
												Default, "FontAtlas-"s << ToString(_atlases.size()) );
		CHECK_ERR( img_id );
		
		auto&	atlas = _atlases.emplace_back();

		CHECK_ERR( atlas.Create( std::move(img_id), img_dim ));
		CHECK_ERR( atlas.Insert( alignedDim, OUT imageId, OUT texcoord ));
		
		AddGlyph( atlas, texcoord );
		return true;
	}
	

}	// AE::Graphics
