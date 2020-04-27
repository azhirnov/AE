// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "graphics/Font/VectorFont.h"
#include "graphics/Font/FreeType.h"

#include "serializing/ObjectFactory.h"

#include "stl/Algorithms/Utf8.h"

#ifndef AE_ENABLE_FREETYPE
# error AE_ENABLE_FREETYPE required
#endif
#ifndef AE_ENABLE_UTF8PROC
# error AE_ENABLE_UTF8PROC required
#endif

namespace AE::Graphics
{

/*
=================================================
	Serialize
=================================================
*/
	bool  VectorFont::PrecalculatedText::Serialize (Serializing::Serializer &ser) const
	{
		return ser( _text, _spacing, _wordWrap );
	}
	
/*
=================================================
	Deserialize
=================================================
*/
	bool  VectorFont::PrecalculatedText::Deserialize (Serializing::Deserializer const &des)
	{
		return des( _text, _spacing, _wordWrap );
	}
//-----------------------------------------------------------------------------



/*
=================================================
	constructor
=================================================
*/
	VectorFont::VectorFont () :
		_fontFace{ null }
	{}

	VectorFont::VectorFont (const FontManagerPtr &mngr, FT_Face_t fontFace, MemStreamPtr &&stream) :
		_fontFace{ fontFace }, _memStream{ std::move(stream) }, _fontMngr{ mngr }
	{}
		
/*
=================================================
	destructor
=================================================
*/
	VectorFont::~VectorFont ()
	{
		if ( _fontFace )
			FT_Done_Face( BitCast<FT_Face>(_fontFace) );
	}
	
/*
=================================================
	CalculateDimensions
=================================================
*/
	void  VectorFont::CalculateDimensions (const float2 &areaSizePx, INOUT PrecalculatedText &result)
	{
		using Chunk = FormattedText::Chunk;

		float2		line;
		uint		max_height	= 0;
		char32_t	prev_c		= '\0';

		result._size = float2{0.0f};
		result._lineHeight.clear();

		const auto	ToNextLine = [&] (Chunk const* chunk)
		{
			float	height	= result.Spacing() * max_height;
			result._lineHeight.push_back( height );

			result._size.x	= Max( result._size.x, line.x );
			line.y			+= height;
			line.x			= 0.0f;
			max_height		= chunk ? chunk->height : 0;
		};

		for (auto* chunk = result.Text().GetFirst(); chunk; chunk = chunk->next)
		{
			const uint		font_height	= ValidateFontHeight( chunk->height );
			const float		font_scale	= float(chunk->height) / font_height;

			max_height = Max( max_height, chunk->height );

			for (size_t pos = 0; pos < chunk->length;)
			{
				const char32_t		c = Utf8Decode( chunk->string, chunk->length, INOUT pos );
				Ptr<Glyph const>	glyph;
			
				// windows style "\r\n" or linux style "\n"
				if ( ((prev_c == '\r') & (c == '\n')) | (c == '\n') )
				{
					ToNextLine( chunk );
					prev_c = c;
					continue;
				}

				prev_c = c;

				if ( not LoadGlyph( GlyphKey{ c, font_height }, OUT glyph ))
				{
					ASSERT( !"Char is out of range!" );
					continue;
				}

				const float	width = (glyph->offsetX[0] + glyph->size.x + glyph->offsetX[1]) * font_scale;

				if ( result.IsWordWrap() & (line.x + width > areaSizePx.x) )
					ToNextLine( chunk );
				else
					line.x += width;
			}
		}

		//if ( line.x > 0.0f )
		ToNextLine( null );

		result._size.y = Max( result._size.y, line.y );
	}

}	// AE::Graphics
