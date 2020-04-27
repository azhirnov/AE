// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "graphics/Canvas/Vertices.h"
#include "stl/Math/Radians.h"

namespace AE::Graphics
{

	//
	// Rectangle Batch
	//
	template <typename VertexType>
	struct RectangleBatch
	{
	// types
		using Vertex_t	= VertexType;

	// variables
		RectF		position;
		RectF		texcoord;
		RGBA8u		color;

	// methods
		constexpr RectangleBatch () {}
		constexpr RectangleBatch (const RectF &pos, RGBA8u color) : RectangleBatch{pos, RectF{}, color} {}
		constexpr RectangleBatch (const RectF &pos, const RectF &texc, RGBA8u color = HtmlColor::White) :	position{pos}, texcoord{texc}, color{color} {}
		
		ND_ static constexpr EPrimitive	Topology ()			{ return EPrimitive::TriangleList; }
		ND_ static constexpr uint		IndexCount ()		{ return 6; }
		ND_ static constexpr uint		VertexCount ()		{ return 4; }

		static constexpr void GetIndices (OUT BatchIndex_t *idx, BatchIndex_t offset)
		{
			*(idx++) = 0 + offset;
			*(idx++) = 1 + offset;
			*(idx++) = 2 + offset;
			*(idx++) = 1 + offset;
			*(idx++) = 2 + offset;
			*(idx++) = 3 + offset;
		}
		
		constexpr void GetVertices (OUT void *ptr, BytesU stride) const
		{
			*Cast<Vertex_t>( ptr + stride*0 ) = Vertex_t{ float2(position.left,  position.top),		float2(texcoord.left,  texcoord.top),	 color };
			*Cast<Vertex_t>( ptr + stride*1 ) = Vertex_t{ float2(position.left,  position.bottom),	float2(texcoord.left,  texcoord.bottom), color };
			*Cast<Vertex_t>( ptr + stride*2 ) = Vertex_t{ float2(position.right, position.top),		float2(texcoord.right, texcoord.top),	 color };
			*Cast<Vertex_t>( ptr + stride*3 ) = Vertex_t{ float2(position.right, position.bottom),	float2(texcoord.right, texcoord.bottom), color };
		}
	};
	

	
	//
	// Nine Patch Batch
	//
	template <typename VertexType>
	struct NinePatchBatch
	{
	// types
		using Vertex_t	= VertexType;

	// variables
		RectF		position;
		RectF		posOffsets;
		RectF		texcoord;
		RectF		texcOffsets;
		RGBA8u		color;
		//bool4		enabledPatches;

	// methods
		constexpr NinePatchBatch () {}
		constexpr NinePatchBatch (const RectF &pos, const RectF &posOffset, const RectF &texc, const RectF &texcOffset, RGBA8u color = HtmlColor::White) :
			position{pos}, posOffsets{posOffset}, texcoord{texc}, texcOffsets{texcOffset}, color{color}
			//enabledPatches{ posOffsets.left > 0.0f, posOffsets.top > 0.0f, posOffsets.right > 0.0f, posOffsets.bottom < 0.0f }
		{}
		
		ND_ static constexpr EPrimitive	Topology ()			{ return EPrimitive::TriangleList; }

		ND_ constexpr uint  IndexCount () const
		{
			return 6*9;
			//return uint(enabledPatches[0]) + uint(enabledPatches[1]) + uint(enabledPatches[2]) + uint(enabledPatches[3]);
		}

		ND_ constexpr uint  VertexCount () const
		{
			return 4 + 4*3;
			//return 4 + uint(enabledPatches[0]) + uint(enabledPatches[1]) + uint(enabledPatches[2]) + uint(enabledPatches[3]) * 3;
		}

		constexpr void GetIndices (OUT BatchIndex_t *idx, BatchIndex_t offset) const
		{
			// left top
			*(idx++) = 0 + offset;	*(idx++) = 4 + offset;	*(idx++) = 6 + offset;
			*(idx++) = 6 + offset;	*(idx++) = 4 + offset;	*(idx++) = 5 + offset;

			// center top
			*(idx++) = 4 + offset;	*(idx++) = 7 + offset;	*(idx++) = 5 + offset;
			*(idx++) = 5 + offset;	*(idx++) = 7 + offset;	*(idx++) = 8 + offset;

			// right top
			*(idx++) = 7 + offset;	*(idx++) = 2 + offset;	*(idx++) = 8 + offset;
			*(idx++) = 8 + offset;	*(idx++) = 2 + offset;	*(idx++) = 9 + offset;
			
			// left center
			*(idx++) = 6 + offset;	*(idx++) = 5 + offset;	*(idx++) = 12 + offset;
			*(idx++) = 12 + offset;	*(idx++) = 5 + offset;	*(idx++) = 10 + offset;
			
			// center
			*(idx++) = 5 + offset;	*(idx++) = 8 + offset;	*(idx++) = 10 + offset;
			*(idx++) = 10 + offset;	*(idx++) = 8 + offset;	*(idx++) = 13 + offset;
			
			// right center
			*(idx++) = 8 + offset;	*(idx++) = 9 + offset;	*(idx++) = 13 + offset;
			*(idx++) = 13 + offset;	*(idx++) = 9 + offset;	*(idx++) = 15 + offset;
			
			// left bottom
			*(idx++) = 12 + offset;	*(idx++) = 10 + offset;	*(idx++) = 1 + offset;
			*(idx++) = 1 + offset;	*(idx++) = 10 + offset;	*(idx++) = 11 + offset;
			
			// center bottom
			*(idx++) = 10 + offset;	*(idx++) = 13 + offset;	*(idx++) = 11 + offset;
			*(idx++) = 11 + offset;	*(idx++) = 13 + offset;	*(idx++) = 14 + offset;
			
			// right bottom
			*(idx++) = 13 + offset;	*(idx++) = 15 + offset;	*(idx++) = 14 + offset;
			*(idx++) = 14 + offset;	*(idx++) = 15 + offset;	*(idx++) = 3 + offset;
		}
		
		constexpr void GetVertices (OUT void *ptr, BytesU stride) const
		{
			const float	posx[] = { position.left, position.left + posOffsets.left,  position.right  - posOffsets.right,   position.right  };
			const float	posy[] = { position.top,  position.top  + posOffsets.top,   position.bottom - posOffsets.bottom,  position.bottom };
			const float tcx[]  = { texcoord.left, texcoord.left + texcOffsets.left, texcoord.right  - texcOffsets.right,  texcoord.right  };
			const float tcy[]  = { texcoord.top,  texcoord.top  + texcOffsets.top,  texcoord.bottom - texcOffsets.bottom, texcoord.bottom };
			
			uint	i = 0;

			// (0,1,2,3)
			*Cast<Vertex_t>( ptr + stride*(i++) ) = Vertex_t{ float2(posx[0], posy[0]), float2(tcx[0], tcy[0]),	color };
			*Cast<Vertex_t>( ptr + stride*(i++) ) = Vertex_t{ float2(posx[0], posy[3]), float2(tcx[0], tcy[3]), color };
			*Cast<Vertex_t>( ptr + stride*(i++) ) = Vertex_t{ float2(posx[3], posy[0]), float2(tcx[3], tcy[0]),	color };
			*Cast<Vertex_t>( ptr + stride*(i++) ) = Vertex_t{ float2(posx[3], posy[3]), float2(tcx[3], tcy[3]), color };

			// left top (4,5,6)
			//if ( enabledPatches[0] | enabledPatches[1] )
			{
				*Cast<Vertex_t>( ptr + stride*(i++) ) = Vertex_t{ float2(posx[1], posy[0]), float2(tcx[1], tcy[0]), color };
				*Cast<Vertex_t>( ptr + stride*(i++) ) = Vertex_t{ float2(posx[1], posy[1]), float2(tcx[1], tcy[1]), color };
				*Cast<Vertex_t>( ptr + stride*(i++) ) = Vertex_t{ float2(posx[0], posy[1]), float2(tcx[0], tcy[1]), color };
			}

			// right top (7,8,9)
			//if ( enabledPatches[2] | enabledPatches[1] )
			{
				*Cast<Vertex_t>( ptr + stride*(i++) ) = Vertex_t{ float2(posx[2], posy[0]), float2(tcx[2], tcy[0]), color };
				*Cast<Vertex_t>( ptr + stride*(i++) ) = Vertex_t{ float2(posx[2], posy[1]), float2(tcx[2], tcy[1]), color };
				*Cast<Vertex_t>( ptr + stride*(i++) ) = Vertex_t{ float2(posx[3], posy[1]), float2(tcx[3], tcy[1]), color };
			}

			// left bottom (10,11,12)
			//if ( enabledPatches[0] | enabledPatches[3] )
			{
				*Cast<Vertex_t>( ptr + stride*(i++) ) = Vertex_t{ float2(posx[1], posy[2]), float2(tcx[1], tcy[2]), color };
				*Cast<Vertex_t>( ptr + stride*(i++) ) = Vertex_t{ float2(posx[1], posy[3]), float2(tcx[1], tcy[3]), color };
				*Cast<Vertex_t>( ptr + stride*(i++) ) = Vertex_t{ float2(posx[0], posy[2]), float2(tcx[0], tcy[2]), color };
			}

			// right bottom (13,14,15)
			//if ( enabledPatches[2] | enabledPatches[3] )
			{
				*Cast<Vertex_t>( ptr + stride*(i++) ) = Vertex_t{ float2(posx[2], posy[2]), float2(tcx[2], tcy[2]), color };
				*Cast<Vertex_t>( ptr + stride*(i++) ) = Vertex_t{ float2(posx[2], posy[3]), float2(tcx[2], tcy[3]), color };
				*Cast<Vertex_t>( ptr + stride*(i++) ) = Vertex_t{ float2(posx[3], posy[2]), float2(tcx[3], tcy[2]), color };
			}
		}
	};



	//
	// Circle Batch
	//
	template <typename VertexType, bool Fill>
	struct CircleBatch
	{
	// types
		using Vertex_t	= VertexType;

	// variables
		RectF		position;
		RGBA8u		color;
		uint		segments	= 0;

	// methods
		constexpr CircleBatch () {}
		constexpr CircleBatch (uint segments, const RectF &pos, RGBA8u color) : position{pos}, color{color}, segments{segments} {}

		ND_ static constexpr EPrimitive	Topology ()				{ return EPrimitive::LineList; }
		ND_ constexpr uint				IndexCount ()	const	{ return segments * 2; }
		ND_ constexpr uint				VertexCount ()	const	{ return segments; }
		
		constexpr void GetIndices (OUT BatchIndex_t *idx, BatchIndex_t offset) const
		{
			for (uint i = 0; i < segments-1; ++i)
			{
				*(idx++) = BatchIndex_t(i) + offset;
				*(idx++) = BatchIndex_t(i+1) + offset;
			}

			*(idx++) = BatchIndex_t(segments-1) + offset;
			*(idx++) = offset;
		}
		
		constexpr void GetVertices (OUT void *ptr, BytesU stride) const
		{
			float2	center	= position.Center();
			float2	scale	= position.Size() * 0.5f;

			for (uint i = 0; i < segments; ++i)
			{
				float	a = 2.0f * float(Pi) * float(i) / segments;
				float2	p = center + scale * float2{cos(a), sin(a)};
				
				*Cast<Vertex_t>( ptr + stride*i ) = Vertex_t{ p, float2{}, color };
			}
		}
	};
	


	//
	// Circle Batch
	//
	template <typename VertexType>
	struct CircleBatch< VertexType, true >
	{
	// types
		using Vertex_t	= VertexType;

	// variables
		RectF		position;
		RectF		texcoord;
		RGBA8u		color;
		uint		segments	= 0;

	// methods
		constexpr CircleBatch () {}
		constexpr CircleBatch (uint segments, const RectF &pos, RGBA8u color) : CircleBatch{segments, pos, RectF{}, color} {}

		constexpr CircleBatch (uint segments, const RectF &pos, const RectF &texc, RGBA8u color) :
			position{pos}, texcoord{texc}, color{color}, segments{segments}
		{
			ASSERT( segments >= 4 );
		}

		ND_ static constexpr EPrimitive	Topology ()				{ return EPrimitive::TriangleList; }
		ND_ constexpr uint				IndexCount ()	const	{ return segments * 3; }
		ND_ constexpr uint				VertexCount ()	const	{ return segments + 1; }
		
		constexpr void GetIndices (OUT BatchIndex_t *idx, BatchIndex_t offset) const
		{
			for (uint i = 1; i < segments; ++i)
			{
				*(idx++) = offset;
				*(idx++) = BatchIndex_t(i) + offset;
				*(idx++) = BatchIndex_t(i+1) + offset;
			}
			
			*(idx++) = offset;
			*(idx++) = BatchIndex_t(segments) + offset;
			*(idx++) = 1 + offset;
		}
		
		constexpr void GetVertices (OUT void *ptr, BytesU stride) const
		{
			float2	center	 = position.Center();
			float2	scale	 = position.Size() * 0.5f;
			float2	tc_bias	 = texcoord.Center();
			float2	tc_scale = texcoord.Size();
			
			*Cast<Vertex_t>( ptr ) = Vertex_t{ center, tc_bias, color };

			for (uint i = 0; i < segments; ++i)
			{
				float	angle	= 2.0f * float(Pi) * float(i) / segments;
				float2	factor	= float2{cos(angle), sin(angle)};
				float2	pos		= center + scale * factor;
				float2	texc	= tc_bias + tc_scale * factor;
				
				*Cast<Vertex_t>( ptr + stride*(i+1) ) = Vertex_t{ pos, texc, color };
			}
		}
	};


	using Rectangle2D		= RectangleBatch< Vertex2D >;
	using Circle2D			= CircleBatch< Vertex2D, false >;
	using FilledCircle2D	= CircleBatch< Vertex2D, true >;
	using NinePatch2D		= NinePatchBatch< Vertex2D >;


}	// AE::Graphics
