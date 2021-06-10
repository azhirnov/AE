// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "ui/StyleCollection.h"
#include "serializing/ObjectFactory.h"
#include "graphics/Canvas/Vertices.h"

namespace AE::UI
{

/*
=================================================
	ColorStyle::Serialize / Deserialize
=================================================
*/
	bool  StyleCollection::ColorStyle::Serialize (Serializing::Serializer &ser) const
	{
		return ser( blendMode, disabled, enabled, mouseOver, touchDown, selected );
	}

	bool  StyleCollection::ColorStyle::Deserialize (Serializing::Deserializer const &des)
	{
		return des( blendMode, disabled, enabled, mouseOver, touchDown, selected );
	}
//-----------------------------------------------------------------------------
	

/*
=================================================
	ImageStyle::Serialize / Deserialize
=================================================
*/
	bool  StyleCollection::ImageStyle::Serialize (Serializing::Serializer &ser) const
	{
		return ser( blendMode, disabled, enabled, mouseOver, touchDown, selected );
	}

	bool  StyleCollection::ImageStyle::Deserialize (Serializing::Deserializer const &des)
	{
		return des( blendMode, disabled, enabled, mouseOver, touchDown, selected );
	}
//-----------------------------------------------------------------------------
	

/*
=================================================
	FontStyle::Serialize / Deserialize
=================================================
*/
	bool  StyleCollection::FontStyle::Serialize (Serializing::Serializer &ser) const
	{
		return ser( blendMode, disabled, enabled, mouseOver, touchDown, selected );
	}

	bool  StyleCollection::FontStyle::Deserialize (Serializing::Deserializer const &des)
	{
		return des( blendMode, disabled, enabled, mouseOver, touchDown, selected );
	}
//-----------------------------------------------------------------------------
	

/*
=================================================
	NamedImage::Serialize / Deserialize
=================================================
*/
	bool  StyleCollection::NamedImage::Serialize (Serializing::Serializer &ser) const
	{
		return ser( name, color );
	}

	bool  StyleCollection::NamedImage::Deserialize (Serializing::Deserializer const &des)
	{
		return des( name, color );
	}
	
/*
=================================================
	NamedImage::Set
=================================================
*/
	inline void  StyleCollection::NamedImage::Set (const Graphics::ImageInAtlas &val) const
	{
		_imageId	= val.imageId;
		_texcoord	= val.texcoord;
		_dimension	= val.dimension;
	}

	inline void  StyleCollection::NamedImage::Set (const Graphics::NinePatchImageInAtlas &val) const
	{
		_imageId	= val.imageId;
		_texcoord	= val.texcoord;
		_border		= val.border;
		_dimension	= val.dimension;
	}
	
/*
=================================================
	NamedImage::Get
=================================================
*/
	inline StyleCollection::Image  StyleCollection::NamedImage::Get () const
	{
		return { _imageId, _texcoord, _border, _dimension, color };
	}
	
/*
=================================================
	NamedImage::HasImage
=================================================
*/
	inline bool  StyleCollection::NamedImage::HasImage () const
	{
		return bool(_imageId);
	}
//-----------------------------------------------------------------------------


/*
=================================================
	Serialize / Deserialize
=================================================
*/
	bool  StyleCollection::Serialize (Serializing::Serializer &ser) const
	{
		return ser( _styleMap );
	}
	
	bool  StyleCollection::Deserialize (Serializing::Deserializer const& des)
	{
		return des( _styleMap );
	}

/*
=================================================
	AddColor
=================================================
*/
	bool  StyleCollection::AddColor (const StyleName &id, const ColorStyle &style)
	{
		DEBUG_ONLY( _hcCheck.Add( id ));

		CHECK( _styleMap.insert_or_assign( id, style ).second );
		return true;
	}
	
/*
=================================================
	AddImage
=================================================
*/
	bool  StyleCollection::AddImage (const StyleName &id, const ImageStyle &style)
	{
		DEBUG_ONLY( _hcCheck.Add( id ));

		CHECK( _styleMap.insert_or_assign( id, style ).second );
		return true;
	}
	
/*
=================================================
	AddFont
=================================================
*/
	bool  StyleCollection::AddFont (const StyleName &id, const FontStyle &style)
	{
		DEBUG_ONLY( _hcCheck.Add( id ));
		
		CHECK( _styleMap.insert_or_assign( id, style ).second );
		return true;
	}

/*
=================================================
	Clear
=================================================
*/
	void  StyleCollection::Clear ()
	{
		_styleMap.clear();
	}
	
/*
=================================================
	_GetColorStyle
=================================================
*/
	RGBA8u  StyleCollection::_GetColorStyle (EStyleState state, const ColorStyle &style) const
	{
		switch ( state )
		{
			case EStyleState::Selected :							return style.selected;
			case EStyleState::Selected | EStyleState::MouseOver :	return style.selected;
			case EStyleState::Selected | EStyleState::TouchDown :	return style.selected;
			case EStyleState::MouseOver :							return style.mouseOver;
			case EStyleState::TouchDown :							return style.touchDown;
			case EStyleState::Disabled :							return style.disabled;
		}
		return style.enabled;
	}
	
/*
=================================================
	_GetImageStyle
=================================================
*/
	StyleCollection::NamedImage const&  StyleCollection::_GetImageStyle (EStyleState state, const ImageStyle &style) const
	{
		NamedImage const*	result = &style.enabled;
		switch ( state )
		{
			case EStyleState::Selected :							result = &style.selected;	break;
			case EStyleState::Selected | EStyleState::MouseOver :	result = &style.selected;	break;
			case EStyleState::Selected | EStyleState::TouchDown :	result = &style.selected;	break;
			case EStyleState::MouseOver :							result = &style.mouseOver;	break;
			case EStyleState::TouchDown :							result = &style.touchDown;	break;
			case EStyleState::Disabled :							result = &style.disabled;	break;
		}

		/*if ( not result->HasImage() )
		{
			_ctx->resourceCache->Find( result->name,
									[&] (const auto* resource)
									{
										if ( auto* np_image = UnionGetIf< NinePatchImageInAtlas >( resource ))
											result->Set( *np_image );
										else
										if ( auto* image = UnionGetIf< ImageInAtlas >( resource ))
											result->Set( *image );
									});
		}*/
		return *result;
	}

/*
=================================================
	GetColor
=================================================
*/
	RGBA8u  StyleCollection::GetColor (const StyleID &id, EStyleState state) const
	{
		using StyleTypes_t = TypeList< Style_t >;

		auto	iter = _styleMap.find( id );
		CHECK_ERR( iter != _styleMap.end() );

		switch ( iter->second.index() )
		{
			case StyleTypes_t::Index<ColorStyle> :	return _GetColorStyle( state, UnionGet<ColorStyle>( iter->second ));
			case StyleTypes_t::Index<FontStyle> :	return _GetColorStyle( state, UnionGet<FontStyle>( iter->second ));
		}
		RETURN_ERR( "not found" );
	}
	
/*
=================================================
	GetImage
=================================================
*/
	StyleCollection::Image  StyleCollection::GetImage (const StyleID &id, EStyleState state) const
	{
		auto	iter = _styleMap.find( id );
		CHECK_ERR( iter != _styleMap.end() );
		
		ImageStyle const*	style = UnionGetIf<ImageStyle>( &iter->second );
		CHECK_ERR( style );
		
		return _GetImageStyle( state, *style ).Get();
	}

/*
=================================================
	GetColorAndPipeline
=================================================
*/
	bool  StyleCollection::GetColorAndPipeline (const StyleID &id, EStyleState state, OUT RGBA8u &outColor,
												OUT PipelineName &outPipeline, OUT EBlendMode &outBlendMode) const
	{
		auto	iter = _styleMap.find( id );
		CHECK_ERR( iter != _styleMap.end() );

		ColorStyle const*	style = UnionGetIf<ColorStyle>( &iter->second );
		CHECK_ERR( style );

		outColor	 = _GetColorStyle( state, *style );
		outPipeline	 = style->pipeline;
		outBlendMode = style->blendMode;
		return true;
	}
	
/*
=================================================
	GetImageAndPipeline
=================================================
*/
	bool  StyleCollection::GetImageAndPipeline (const StyleID &id, EStyleState state, OUT Image &outImage,
												OUT PipelineName &outPipeline, OUT EBlendMode &outBlendMode) const
	{
		auto	iter = _styleMap.find( id );
		CHECK_ERR( iter != _styleMap.end() );
		
		ImageStyle const*	style = UnionGetIf<ImageStyle>( &iter->second );
		CHECK_ERR( style );
		
		outImage	 = _GetImageStyle( state, *style ).Get();
		outPipeline	 = style->pipeline;
		outBlendMode = style->blendMode;
		return true;
	}
	
/*
=================================================
	GetFontAndPipeline
=================================================
*/
	bool  StyleCollection::GetFontAndPipeline (const StyleID &id, EStyleState state, OUT RGBA8u &outColor, OUT FontPtr &outFont,
												OUT PipelineName &outPipeline, OUT EBlendMode &outBlendMode) const
	{
		auto	iter = _styleMap.find( id );
		CHECK_ERR( iter != _styleMap.end() );
		
		FontStyle const*	style = UnionGetIf<FontStyle>( &iter->second );
		CHECK_ERR( style );

		outColor	 = _GetColorStyle( state, *style );
		outPipeline	 = style->pipeline;
		outBlendMode = style->blendMode;
		outFont		 = style->font;
		return true;
	}
	
/*
=================================================
	Instance
=================================================
*/
	StyleCollection&  StyleCollection::Instance ()
	{
		static StyleCollection	inst;
		return inst;
	}

}	// AE::UI
