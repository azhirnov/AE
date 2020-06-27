// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "ui/LayoutEnums.h"
#include "serializing/ISerializable.h"
#include "graphics/Canvas/Canvas.h"
#include "stl/Utils/NamedID_HashCollisionCheck.h"

namespace AE::UI
{

	//
	// Style Collection
	//

	class StyleCollection final : public Serializing::ISerializable
	{
	// types
	public:
		struct Image
		{
			GfxResourceID	imageId;
			RectF			texcoord;
			RectF			border;
			float2			dimension;	// dips
			RGBA8u			color;
		};


		class NamedImage final : public Serializing::ISerializable
		{
		// variables
		public:
			ResourceName			name;
			RGBA8u					color;
		private:
			mutable GfxResourceID	_imageId;
			mutable RectF			_texcoord;
			mutable RectF			_border;
			mutable float2			_dimension;	// dips

		// methods
		public:
			NamedImage () {}
			NamedImage (const ResourceName &name, RGBA8u color) : name{name}, color{color} {}
			
			void Set (const Graphics::ImageInAtlas &val) const;
			void Set (const Graphics::NinePatchImageInAtlas &val) const;

			ND_ bool  HasImage () const;
			ND_ Image Get () const;

			bool Serialize (Serializing::Serializer &) const override;
			bool Deserialize (Serializing::Deserializer const&) override;
		};


		class ColorStyle : public Serializing::ISerializable
		{
			friend class StyleCollection;

		// variables
		public:
			PipelineName		pipeline;
			EBlendMode			blendMode	= Default;
			RGBA8u				disabled;
			RGBA8u				enabled;
			RGBA8u				mouseOver;
			RGBA8u				touchDown;
			RGBA8u				selected;

		// methods
		public:
			ColorStyle () {}
			bool Serialize (Serializing::Serializer &) const override;
			bool Deserialize (Serializing::Deserializer const&) override;
		};


		class ImageStyle final : public Serializing::ISerializable
		{
			friend class StyleCollection;

		// variables
		public:
			PipelineName		pipeline;
			EBlendMode			blendMode	= Default;
			NamedImage			disabled;
			NamedImage			enabled;
			NamedImage			mouseOver;
			NamedImage			touchDown;
			NamedImage			selected;

		// methods
		public:
			ImageStyle () {}
			bool Serialize (Serializing::Serializer &) const override;
			bool Deserialize (Serializing::Deserializer const&) override;
		};


		class FontStyle final : public ColorStyle
		{
			friend class StyleCollection;

		// variables
		public:
			FontPtr			font;

		// methods
		public:
			FontStyle () {}
			bool Serialize (Serializing::Serializer &) const override;
			bool Deserialize (Serializing::Deserializer const&) override;
		};


		struct AnimationSettings
		{
			float				colorAnimSpeed	= 10.0f;		// 1 / seconds
		};


	private:
		using Style_t		= Union< NullUnion, ColorStyle, ImageStyle, FontStyle >;
		using ColorMap_t	= HashMap< StyleID, Style_t >;


	// variables
	private:
		ColorMap_t			_styleMap;
		AnimationSettings	_settings;
		
		DEBUG_ONLY(
			NamedID_HashCollisionCheck	_hcCheck;
		)


	// methods
	public:
		StyleCollection () {}

		bool  Serialize (Serializing::Serializer &) const override;
		bool  Deserialize (Serializing::Deserializer const&) override;

		bool  AddColor (const StyleName &id, const ColorStyle &style);
		bool  AddImage (const StyleName &id, const ImageStyle &style);
		bool  AddFont (const StyleName &id, const FontStyle &style);
		void  Clear ();
		
		bool  GetColorAndPipeline (const StyleID &id, EStyleState state, OUT RGBA8u &outColor,
								   OUT PipelineName &outPipeline, OUT EBlendMode &outBlendMode) const;

		bool  GetImageAndPipeline (const StyleID &id, EStyleState state, OUT Image &outImage,
								   OUT PipelineName &outPipeline, OUT EBlendMode &outBlendMode) const;
		
		bool  GetFontAndPipeline (const StyleID &id, EStyleState state, OUT RGBA8u &outColor, OUT FontPtr &outFont,
								   OUT PipelineName &outPipeline, OUT EBlendMode &outBlendMode) const;

		ND_ RGBA8u	GetColor (const StyleID &id, EStyleState state) const;
		ND_ Image	GetImage (const StyleID &id, EStyleState state) const;

		ND_ AnimationSettings const&	GetSettings ()	const	{ return _settings; }

		ND_ static StyleCollection &	Instance ();


	private:
		ND_ RGBA8u				_GetColorStyle (EStyleState state, const ColorStyle &style) const;
		ND_ NamedImage const&	_GetImageStyle (EStyleState state, const ImageStyle &style) const;
	};


	ND_ forceinline StyleCollection*  UIStyleCollection ()
	{
		return &StyleCollection::Instance();
	}

}	// AE::UI
