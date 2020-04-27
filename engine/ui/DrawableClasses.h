// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "ui/Drawable.h"
#include "ui/Layout.h"
#include "ui/StyleCollection.h"

namespace AE::UI
{

	//
	// Rectangle Shape Drawable
	//
	class RectangleDrawable final : public IDrawable
	{
	// variables
	private:
		PipelineName		_pipeline;
		EBlendMode			_blendMode		= Default;
		EStyleState			_state			= Default;
		RGBA8u				_prevColor;
		RGBA8u				_currColor;
		float				_factor			= 2.0f;
		StyleID				_styleId;


	// methods
	public:
		RectangleDrawable () {}
		explicit RectangleDrawable (const StyleID &styleId);
		
		bool Serialize (Serializing::Serializer &) const override;
		bool Deserialize (Serializing::Deserializer const&) override;

		void Draw (const UIRendererData &, const LayoutState &, float dt) override;
		bool GetPreferredSize (const RectF &, OUT RectF &) const override	{ return false; }
	};



	//
	// Image Drawable
	//
	class ImageDrawable final : public IDrawable
	{
	// variables
	private:
		PipelineName			_pipeline;
		EBlendMode				_blendMode		= Default;
		EStyleState				_state			= Default;
		StyleCollection::Image	_currImage;
		RGBA8u					_prevColor;
		float					_factor			= 2.0f;
		StyleID					_styleId;


	// methods
	public:
		ImageDrawable () {}
		explicit ImageDrawable (const StyleID &styleId);
		
		bool Serialize (Serializing::Serializer &) const override;
		bool Deserialize (Serializing::Deserializer const&) override;

		void Draw (const UIRendererData &, const LayoutState &, float dt) override;
		bool GetPreferredSize (const RectF &maxRect, OUT RectF &result) const override;
	};



	//
	// Nine Patch Drawable
	//
	class NinePatchDrawable final : public IDrawable
	{
	// variables
	private:
		PipelineName			_pipeline;
		EBlendMode				_blendMode		= Default;
		EStyleState				_state			= Default;
		StyleCollection::Image	_currImage;
		RectF					_border;		// dips
		RGBA8u					_prevColor;
		float					_factor			= 2.0f;
		StyleID					_styleId;


	// methods
	public:
		NinePatchDrawable () {}
		explicit NinePatchDrawable (const StyleID &styleId, const RectF &border);
		
		bool Serialize (Serializing::Serializer &) const override;
		bool Deserialize (Serializing::Deserializer const&) override;

		void Draw (const UIRendererData &, const LayoutState &, float dt) override;
		bool GetPreferredSize (const RectF &maxRect, OUT RectF &result) const override;
	};



	//
	// Text Drawable
	//
	class TextDrawable final : public IDrawable
	{
		using Text_t		= Graphics::VectorFont::PrecalculatedText;
		using FormattedText	= Graphics::FormattedText;

	// variables
	private:
		Text_t				_text;
		PipelineName		_pipeline;
		FontPtr				_font;
		EBlendMode			_blendMode		= Default;
		EStyleState			_state			= Default;
		RGBA8u				_prevColor;
		RGBA8u				_currColor;
		float				_factor			= 2.0f;
		StyleID				_styleId;


	// methods
	public:
		TextDrawable () {}
		explicit TextDrawable (const FormattedText &text, float spacing, bool wordWrap, const StyleID &styleId);
		
		void SetText (const FormattedText &text, float spacing, bool wordWrap);

		bool Serialize (Serializing::Serializer &) const override;
		bool Deserialize (Serializing::Deserializer const&) override;

		void Draw (const UIRendererData &, const LayoutState &, float dt) override;
		bool GetPreferredSize (const RectF &, OUT RectF &) const override	{ return false; }
	};


}	// AE::UI
