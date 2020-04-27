// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "graphics/Font/FontManager.h"
#include "graphics/Font/FormattedText.h"

namespace AE::Graphics
{

	//
	// Vector Font
	//

	class VectorFont final : public EnableRC<VectorFont>
	{
	// types
	public:
		using Glyph			= FontManager::Glyph;
		using GlyphKey		= FontManager::GlyphKey;
		using EFontStyle	= FontManager::EFontStyle;
		

		class PrecalculatedText final : public Serializing::ISerializable
		{
			friend class VectorFont;

		private:
			Array<float>	_lineHeight;		// in pixels	// TODO: small vector
			float2			_size;				// in pixels
			FormattedText	_text;
			float			_spacing	= 1.0f;	// in normalized coords
			bool			_wordWrap	= false;

		public:
			PrecalculatedText () {}
			
			bool Serialize (Serializing::Serializer &) const override;
			bool Deserialize (Serializing::Deserializer const &) override;

			PrecalculatedText&  SetText (const FormattedText &value)	{ _text = value;  return *this; }
			PrecalculatedText&  SetText (FormattedText &&value)			{ _text = std::move(value);  return *this; }
			PrecalculatedText&  SetSpacing (float value)				{ _spacing  = value;  return *this; }
			PrecalculatedText&  SetWordWrap (bool value)				{ _wordWrap = value;  return *this; }

			ND_ FormattedText const&	Text ()			const	{ return _text; }
			ND_ float					Spacing ()		const	{ return _spacing; }
			ND_ float2 const&			RegionSize ()	const	{ return _size; }
			ND_ bool					IsWordWrap ()	const	{ return _wordWrap; }
			ND_ ArrayView<float>		LineHeights ()	const	{ return _lineHeight; }
		};


	public:
		using MemStreamPtr	= UniquePtr< RStream >;
		using FT_Face_t		= FontManager::FT_Face_t;

		struct GlyphKeyHash {
			size_t  operator () (const GlyphKey &x) const {
				if constexpr( sizeof(size_t) > 4 )	return x.unicodeChar | (size_t(x.height) << 32);
				else								return x.unicodeChar ^ (x.height << 24);	// TODO
			}
		};

		using GlyphMap_t = std::unordered_map< GlyphKey, Glyph, GlyphKeyHash >;


	// variables
	private:
		GlyphMap_t			_glyphMap;
		FT_Face_t			_fontFace;
		MemStreamPtr		_memStream;
		FontManagerPtr		_fontMngr;


	// methods
	public:
		VectorFont ();
		VectorFont (const FontManagerPtr &mngr, FT_Face_t fontFace, MemStreamPtr &&stream);
		~VectorFont ();
		
		void CalculateDimensions (const float2 &areaSizeInPixels, INOUT PrecalculatedText &);
		bool LoadGlyph (const GlyphKey &key, OUT Ptr<Glyph const> &outGlyph);
		
		ND_ static uint  ValidateFontHeight (uint height);
	};

	
	
/*
=================================================
	LoadGlyph
=================================================
*/
	inline bool  VectorFont::LoadGlyph (const GlyphKey &key, OUT Ptr<Glyph const> &outGlyph)
	{
		auto&	glyph = _glyphMap[ key ];

		if ( not glyph.imageId )
		{
			CHECK_ERR( _fontMngr->_LoadGlyph( _fontFace, key, OUT glyph ));
		}

		outGlyph = &glyph;
		return true;
	}
	
/*
=================================================
	ValidateFontHeight
=================================================
*/
	inline uint  VectorFont::ValidateFontHeight (const uint height)
	{
		return height;
		//return 1u << (IntLog2( height ) + uint(not IsPowerOfTwo( height )));
		//return (1u << IntLog2( height ));	// largest power of two value not greater than 'height'
	}


}	// AE::Graphics
