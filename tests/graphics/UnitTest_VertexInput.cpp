// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#include "graphics/Public/VertexInputState.h"
#include "UnitTest_Common.h"

namespace
{
	using VertexAttrib = VertexInputState::VertexAttrib;


	inline bool TestVertexInput (const VertexInputState &state, const VertexName &id, uint requiredIndex)
	{
		auto	iter = state.Vertices().find( id );

		if ( iter == state.Vertices().end() )
			return false;

		return	iter->second.index	== requiredIndex;
	}


	static void VertexInput_Test1 ()
	{
		struct Vertex1
		{
			float3			position;
			Vec<short,2>	texcoord;
			RGBA8u			color;
		};

		const FixedArray<VertexAttrib, 16>	attribs = {{
			{ VertexName("position"),	2, EVertexType::Float3 },
			{ VertexName("texcoord"),	0, EVertexType::Float2 },
			{ VertexName("color"),	1, EVertexType::Float4 }
		}};

		VertexInputState	vertex_input;
		vertex_input.Bind( VertexBufferName(), SizeOf<Vertex1> );

		vertex_input.Add( VertexName("position"),	&Vertex1::position );
		vertex_input.Add( VertexName("texcoord"),	&Vertex1::texcoord, true );
		vertex_input.Add( VertexName("color"),		&Vertex1::color );

		TEST( vertex_input.ApplyAttribs( attribs ));

		TEST( TestVertexInput( vertex_input, VertexName("position"),	2 ));
		TEST( TestVertexInput( vertex_input, VertexName("texcoord"),	0 ));
		TEST( TestVertexInput( vertex_input, VertexName("color"),		1 ));
	}


	static void VertexInput_Test2 ()
	{
		struct Vertex1
		{
			float3			position;
			Vec<short,2>	texcoord;
			RGBA8u			color;
		};

		const FixedArray<VertexAttrib, 16>	attribs = {{
			{ VertexName("position1"),	2, EVertexType::Float3 },
			{ VertexName("texcoord1"),	0, EVertexType::Float2 },
			{ VertexName("color1"),		1, EVertexType::Float4 },
			{ VertexName("position2"),	5, EVertexType::Float3 },
			{ VertexName("texcoord2"),	4, EVertexType::Float2 },
			{ VertexName("color2"),		3, EVertexType::Float4 }
		}};
	
		VertexInputState	vertex_input;
		vertex_input.Bind( VertexBufferName("frame1"), SizeOf<Vertex1> );
		vertex_input.Bind( VertexBufferName("frame2"), SizeOf<Vertex1> );

		vertex_input.Add( VertexName("position1"),	&Vertex1::position,			VertexBufferName("frame1") );
		vertex_input.Add( VertexName("texcoord1"),	&Vertex1::texcoord, true,	VertexBufferName("frame1") );
		vertex_input.Add( VertexName("color1"),		&Vertex1::color,			VertexBufferName("frame1") );
		vertex_input.Add( VertexName("position2"),	&Vertex1::position,			VertexBufferName("frame2") );
		vertex_input.Add( VertexName("texcoord2"),	&Vertex1::texcoord, true,	VertexBufferName("frame2") );
		vertex_input.Add( VertexName("color2"),		&Vertex1::color,			VertexBufferName("frame2") );
	
		TEST( vertex_input.ApplyAttribs( attribs ));

		TEST( TestVertexInput( vertex_input, VertexName("position1"),	2 ));
		TEST( TestVertexInput( vertex_input, VertexName("texcoord1"),	0 ));
		TEST( TestVertexInput( vertex_input, VertexName("color1"),		1 ));
		TEST( TestVertexInput( vertex_input, VertexName("position2"),	5 ));
		TEST( TestVertexInput( vertex_input, VertexName("texcoord2"),	4 ));
		TEST( TestVertexInput( vertex_input, VertexName("color2"),		3 ));
	}
}

extern void UnitTest_VertexInput ()
{
	VertexInput_Test1();
	VertexInput_Test2();
	AE_LOGI( "UnitTest_VertexInput - passed" );
}
