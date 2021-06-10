// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "ui/DrawableClasses.h"
#include "ui/UISystem.h"

#include "graphics/Canvas/Primitives.h"
#include "graphics/Public/ResourceManager.h"

#include "serializing/ObjectFactory.h"

namespace AE::UI
{
	using AE::Graphics::UniformName;
	using AE::Graphics::DescriptorSetName;
	using AE::Graphics::SamplerName;
	using AE::Graphics::DescriptorSetID;
	
/*
=================================================
	constructor
=================================================
*/
	RectangleDrawable::RectangleDrawable (const StyleID &styleId) : _styleId{styleId}
	{
		UIStyleCollection()->GetColorAndPipeline( _styleId, _state, OUT _currColor, OUT _pipeline, OUT _blendMode );
		_prevColor = _currColor;
	}

/*
=================================================
	Draw
=================================================
*/
	void RectangleDrawable::Draw (const UIRendererData &renderer, const LayoutState &layout, float dt)
	{
		// change state
		if_unlikely( _state != layout.StyleFlags() )
		{
			_state		= layout.StyleFlags();
			_prevColor	= (_factor < 1.0f ? Lerp( _prevColor, _currColor, _factor ) : _currColor);
			_currColor	= UIStyleCollection()->GetColor( _styleId, _state );
			_factor		= 0.0f;
		}

		RGBA8u	color = _currColor;

		// color animation
		if_unlikely( _factor < 1.0f )
		{
			_factor	= Min( 1.0f, _factor + dt * UIStyleCollection()->GetSettings().colorAnimSpeed );
			color	= Lerp( _prevColor, _currColor, _factor );
		}

		Canvas::Material	mtr;
		mtr.SetPipeline( _pipeline );
		mtr.SetBlending( _blendMode );

		renderer.canvas.AddBatch( mtr, Rectangle2D{ renderer.canvas.DipsToViewport( layout.ClippedRect() ), RectF{}, color });
	}
	
/*
=================================================
	Serialize / Deserialize
=================================================
*/
	bool RectangleDrawable::Serialize (Serializing::Serializer &ser) const
	{
		return ser( _styleId );
	}
	
	bool RectangleDrawable::Deserialize (Serializing::Deserializer const& des)
	{
		CHECK_ERR( des( _styleId ));

		UIStyleCollection()->GetColorAndPipeline( _styleId, _state, OUT _currColor, OUT _pipeline, OUT _blendMode );
		_prevColor = _currColor;

		return true;
	}
//-----------------------------------------------------------------------------

	
	
/*
=================================================
	constructor
=================================================
*/
	ImageDrawable::ImageDrawable (const StyleID &styleId) : _styleId{styleId}
	{
		UIStyleCollection()->GetImageAndPipeline( _styleId, _state, OUT _currImage, OUT _pipeline, OUT _blendMode );
		_prevColor = _currImage.color;
	}

/*
=================================================
	Draw
=================================================
*/
	void ImageDrawable::Draw (const UIRendererData &renderer, const LayoutState &layout, float dt)
	{
		// change state
		if_unlikely( _state != layout.StyleFlags() )
		{
			_state		= layout.StyleFlags();
			_prevColor	= (_factor < 1.0f ? Lerp( _prevColor, _currImage.color, _factor ) : _currImage.color);
			_currImage	= UIStyleCollection()->GetImage( _styleId, _state );
			_factor		= 0.0f;
		}
		
		RGBA8u	color = _currImage.color;

		// color animation
		if_unlikely( _factor < 1.0f )
		{
			_factor	= Min( 1.0f, _factor + dt * UIStyleCollection()->GetSettings().colorAnimSpeed );
			color	= Lerp( _prevColor, _currImage.color, _factor );
		}
		
		// TODO: optimize ?
		Graphics::DescriptorSet		desc_set;
		CHECK_ERR( renderer.resourceMngr.InitializeDescriptorSet( _pipeline, DescriptorSetName{"0"}, OUT desc_set ), void());

		desc_set.BindTexture( UniformName{"un_Texture"}, _currImage.imageId, SamplerName{"LinearClamp"} );

		DescriptorSetID	ds = renderer.resourceMngr.CreateDescriptorSet( desc_set );
		CHECK_ERR( ds, void());

		Canvas::Material	mtr;
		mtr.SetPipeline( _pipeline );
		mtr.SetDescriptorSet( ds );
		mtr.SetBlending( _blendMode );

		renderer.canvas.AddBatch( mtr, Rectangle2D{ renderer.canvas.DipsToViewport( layout.ClippedRect() ), _currImage.texcoord, color });
	}
	
/*
=================================================
	GetPreferredSize
=================================================
*/
	bool ImageDrawable::GetPreferredSize (const RectF &, OUT RectF &result) const
	{
		result = RectF{ 0.0f, 0.0f, _currImage.dimension.x, _currImage.dimension.y };
		return true;
	}

/*
=================================================
	Serialize / Deserialize
=================================================
*/
	bool ImageDrawable::Serialize (Serializing::Serializer &ser) const
	{
		return ser( _styleId );
	}

	bool ImageDrawable::Deserialize (Serializing::Deserializer const& des)
	{
		CHECK_ERR( des( _styleId ));

		UIStyleCollection()->GetImageAndPipeline( _styleId, _state, OUT _currImage, OUT _pipeline, OUT _blendMode );
		_prevColor = _currImage.color;

		return true;
	}
//-----------------------------------------------------------------------------

	
	
/*
=================================================
	constructor
=================================================
*/
	NinePatchDrawable::NinePatchDrawable (const StyleID &styleId, const RectF &border) :
		_border{border}, _styleId{styleId}
	{
		UIStyleCollection()->GetImageAndPipeline( _styleId, _state, OUT _currImage, OUT _pipeline, OUT _blendMode );
		_prevColor = _currImage.color;
	}

/*
=================================================
	Draw
=================================================
*/
	void NinePatchDrawable::Draw (const UIRendererData &renderer, const LayoutState &layout, float dt)
	{
		// change state
		if_unlikely( _state != layout.StyleFlags() )
		{
			_state		= layout.StyleFlags();
			_prevColor	= (_factor < 1.0f ? Lerp( _prevColor, _currImage.color, _factor ) : _currImage.color);
			_currImage	= UIStyleCollection()->GetImage( _styleId, _state );
			_factor		= 0.0f;
		}
		
		RGBA8u	color;// = _currImage.color;

		// color animation
		if_unlikely( _factor < 1.0f )
		{
			_factor	= Min( 1.0f, _factor + dt * UIStyleCollection()->GetSettings().colorAnimSpeed );
			color	= Lerp( _prevColor, _currImage.color, _factor );
		}
		
		// TODO: optimize ?
		Graphics::DescriptorSet		desc_set;
		CHECK_ERR( renderer.resourceMngr.InitializeDescriptorSet( _pipeline, DescriptorSetName{"0"}, OUT desc_set ), void());

		desc_set.BindTexture( UniformName{"un_Texture"}, _currImage.imageId, SamplerName{"LinearClamp"} );

		DescriptorSetID	ds = renderer.resourceMngr.CreateDescriptorSet( desc_set );
		CHECK_ERR( ds, void());

		Canvas::Material	mtr;
		mtr.SetPipeline( _pipeline );
		mtr.SetDescriptorSet( ds );
		mtr.SetBlending( _blendMode );

		renderer.canvas.AddBatch( mtr, NinePatch2D{ renderer.canvas.DipsToViewport( layout.ClippedRect() ),
													renderer.canvas.DipsSizeToViewport( _border ),
													_currImage.texcoord, _currImage.border, color });
	}
	
/*
=================================================
	GetPreferredSize
=================================================
*/
	bool NinePatchDrawable::GetPreferredSize (const RectF &, OUT RectF &result) const
	{
		result = RectF{ 0.0f, 0.0f, _currImage.dimension.x, _currImage.dimension.y };
		return true;
	}

/*
=================================================
	Serialize / Deserialize
=================================================
*/
	bool NinePatchDrawable::Serialize (Serializing::Serializer &ser) const
	{
		return ser( _styleId, _border );
	}

	bool NinePatchDrawable::Deserialize (Serializing::Deserializer const& des)
	{
		CHECK_ERR( des( _styleId, _border ));

		UIStyleCollection()->GetImageAndPipeline( _styleId, _state, OUT _currImage, OUT _pipeline, OUT _blendMode );
		_prevColor = _currImage.color;

		return true;
	}
//-----------------------------------------------------------------------------


	
/*
=================================================
	constructor
=================================================
*/
	TextDrawable::TextDrawable (const FormattedText &text, float spacing, bool wordWrap, const StyleID &styleId) :
		_styleId{styleId}
	{
		UIStyleCollection()->GetFontAndPipeline( _styleId, _state, OUT _currColor, OUT _font, OUT _pipeline, OUT _blendMode );
		_prevColor = _currColor;

		SetText( text, spacing, wordWrap );
	}

/*
=================================================
	SetText
=================================================
*/
	void  TextDrawable::SetText (const FormattedText &text, float spacing, bool wordWrap)
	{
		_text.SetText( text ).SetSpacing( spacing ).SetWordWrap( wordWrap );
		_font->CalculateDimensions( float2{}, INOUT _text );	// TODO
	}

/*
=================================================
	Draw
=================================================
*/
	void TextDrawable::Draw (const UIRendererData &renderer, const LayoutState &layout, float dt)
	{
		// change state
		if_unlikely( _state != layout.StyleFlags() )
		{
			_state		= layout.StyleFlags();
			_prevColor	= (_factor < 1.0f ? Lerp( _prevColor, _currColor, _factor ) : _currColor);
			_currColor	= UIStyleCollection()->GetColor( _styleId, _state );
			_factor		= 0.0f;
		}

		RGBA8u	color = _currColor;

		// color animation
		if_unlikely( _factor < 1.0f )
		{
			_factor	= Min( 1.0f, _factor + dt * UIStyleCollection()->GetSettings().colorAnimSpeed );
			color	= Lerp( _prevColor, _currColor, _factor );
		}
		
		Canvas::Material	mtr;
		mtr.SetPipeline( _pipeline );
		mtr.SetBlending( _blendMode );

		// TODO: optimize ?
		Graphics::DescriptorSet		desc_set;
		CHECK_ERR( renderer.resourceMngr.InitializeDescriptorSet( _pipeline, DescriptorSetName{"0"}, OUT desc_set ), void());

		renderer.canvas.DrawText( _text, _font, renderer.canvas.DipsToPixels( layout.GlobalRect() ), color, mtr, desc_set, UniformName{"un_Texture"} );
	}
	
/*
=================================================
	Serialize / Deserialize
=================================================
*/
	bool TextDrawable::Serialize (Serializing::Serializer &ser) const
	{
		return ser( _text, _styleId );
	}
	
	bool TextDrawable::Deserialize (Serializing::Deserializer const& des)
	{
		CHECK_ERR( des( _text, _styleId ));

		UIStyleCollection()->GetFontAndPipeline( _styleId, _state, OUT _currColor, OUT _font, OUT _pipeline, OUT _blendMode );
		_prevColor = _currColor;

		return true;
	}
//-----------------------------------------------------------------------------


}	// AE::UI
