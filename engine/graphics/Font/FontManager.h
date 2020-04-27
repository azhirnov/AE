// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "graphics/Font/ImageAtlas.h"
#include "graphics/Public/ResourceManager.h"
#include "graphics/Public/RenderGraph.h"

#include "stl/Types/FileSystem.h"
#include "stl/Stream/Stream.h"
#include "stl/Memory/LinearAllocator.h"

#include "threading/Primitives/DataRaceCheck.h"

namespace AE::Graphics
{
	using FontPtr		 = RC< class VectorFont >;
	using FontManagerPtr = RC< class FontManager >;



	//
	// Font Manager
	//

	class FontManager final : public EnableRC< FontManager >
	{
		friend class VectorFont;

	// types
	private:
		using FT_Library_t	= struct _FT_Library_ *;
		using FT_Face_t		= struct _FT_Face_ *;
		
		enum class EFontStyle : uint8_t
		{
			Unknown	= 0,
			Italic	= 1,
			Bold	= 2,
		};

		struct GlyphKey
		{
			char32_t	unicodeChar	= 0;
			uint		height		= 0;

			GlyphKey () {}
			GlyphKey (char32_t unicodeChar, uint height) : unicodeChar{unicodeChar}, height{height} {}

			ND_ bool  operator == (const GlyphKey &rhs) const	{ return (unicodeChar == rhs.unicodeChar) & (height == rhs.height); }
		};

		struct Glyph
		{
			GfxResourceID	imageId;
			RectF			texcoord;
			float2			size;
			float2			offsetX;			// from left, from right
			float			offsetY		= 0.0f;	// from top coord to baseline
			
			Glyph () {}
			explicit Glyph (float advanceX) : offsetX{0.0f, advanceX} {}
		};
		
		struct GlyphData
		{
			const void*		memory		= null;
			ushort2			offset;
			ushort2			extent;
		};

		class AtlasData final : public ImageAtlas
		{
		public:
			Array<GlyphData>	pendingGlyphs;
			bool				initialState	= true;
		};

		using ImageAtlases_t	= Array< AtlasData >;

		static constexpr uint	GlyphMarginX	= 4;
		static constexpr uint	GlyphMarginY	= 4;
		static constexpr uint	AtlasWidth		= 1024;
		static constexpr uint	AtlasHeight		= 1024;
		static constexpr auto	PixelFormat		= EPixelFormat::R8_UNorm;
		static constexpr uint	BytesPerPixel	= 1;


	// variables
	private:
		IResourceManager &			_resMngr;

		ImageAtlases_t				_atlases;		// TODO: use texture array / sparse memory
		FT_Library_t				_ftLibrary;

		LinearAllocator<>			_allocator;

		Threading::DataRaceCheck	_drCheck;


	// methods
	public:
		explicit FontManager (IResourceManager &resMngr);
		~FontManager ();

		bool  Flush (ITransferContext &ctx);

		ND_ FontPtr  Load (const SharedPtr<RStream> &stream);
		ND_ FontPtr  Load (const Path &path);


	private:
		void  _Destroy ();

		bool  _LoadGlyph (FT_Face_t tface, const GlyphKey &key, OUT Glyph &glyph);
		bool  _CreateEmptyGlyph (const GlyphKey &key, OUT Glyph &glyph);

		bool  _AllocGlyphImage (const uint2 &alignedDim, const void* glypgData, OUT GfxResourceID &imageId, OUT RectI &texcoord, OUT uint2 &atlasDim);
	};
	

}	// AE::Graphics
