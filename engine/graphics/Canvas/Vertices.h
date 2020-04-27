// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "graphics/Canvas/Canvas.h"
#include "graphics/Canvas/VertexAttributes.h"

namespace AE::Graphics
{

	//
	// Vertex 2D
	//
	struct Vertex2D
	{
	// types
		using Self	= Vertex2D;

	// variables
		float2		position;
		float2		texcoord;
		RGBA8u		color;
		
	// methods
		Vertex2D () {}
		
		Vertex2D (const float2 &pos, const float2 &texc, const RGBA8u &color) :
			position(pos), texcoord(texc), color(color)
		{}

		ND_ static VertexInputState  GetAttribs (BytesU stride)
		{
			return VertexInputState()
				.Bind( Default, stride )
				.Add( EVertexAttribute::Position,		&Self::position )
				.Add( EVertexAttribute::TextureUVs[0],	&Self::texcoord )
				.Add( EVertexAttribute::Color,			&Self::color,	true );
		}

		ND_ static std::type_index  GetTypeId ()
		{
			return typeid(Self);
		}
	};



	//
	// SDF Vertex 2D
	//
	struct SdfVertex2D
	{
	// types
		using Self	= SdfVertex2D;

	// variables
		float2		position;
		float2		texcoord;		// TODO: use uint16_t
		RGBA8u		color;
		
	// methods
		SdfVertex2D () {}
		
		SdfVertex2D (const float2 &pos, const float2 &texc, const RGBA8u &color) :
			position(pos), texcoord(texc), color(color)
		{}

		ND_ static VertexInputState  GetAttribs (BytesU stride)
		{
			return VertexInputState()
				.Bind( Default, stride )
				.Add( EVertexAttribute::Position,		&Self::position )
				.Add( EVertexAttribute::TextureUVs[0],	&Self::texcoord )
				.Add( EVertexAttribute::Color,			&Self::color,	true );
		}

		ND_ static std::type_index  GetTypeId ()
		{
			return typeid(Self);
		}
	};



	//
	// Vertex 3D
	//
	struct Vertex3D
	{
	// types
		using Self	= Vertex3D;

	// variables
		float3		position;
		float2		texcoord;
		RGBA8u		color;
		
	// methods
		Vertex3D () {}
		
		Vertex3D (const float3 &pos, const float2 &texc, const RGBA8u &color) :
			position(pos), texcoord(texc), color(color)
		{}

		ND_ static VertexInputState  GetAttribs (BytesU stride)
		{
			return VertexInputState()
				.Bind( Default, stride )
				.Add( EVertexAttribute::Position,		&Self::position )
				.Add( EVertexAttribute::TextureUVs[0],	&Self::texcoord )
				.Add( EVertexAttribute::Color,			&Self::color,	true );
		}
		
		ND_ static std::type_index  GetTypeId ()
		{
			return typeid(Self);
		}
	};
	

}	// AE::Graphics
