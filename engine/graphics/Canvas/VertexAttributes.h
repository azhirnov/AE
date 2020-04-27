// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "graphics/Public/VertexInputState.h"

namespace AE::Graphics
{

	struct VertexAttributeName
	{
		using Name_t = FixedString<32>;

		static constexpr Name_t		Position	{"at_Position"};
		static constexpr Name_t		Position_1	{"at_Position_1"};
		
		static constexpr Name_t		Normal		{"at_Normal"};
		static constexpr Name_t		BiNormal	{"at_BiNormal"};
		static constexpr Name_t		Tangent		{"at_Tangent"};
		
		static constexpr Name_t		ObjectID	{"at_ObjectID"};
		static constexpr Name_t		MaterialID	{"at_MaterialID"};

		static constexpr Name_t		BoneWeights	{"at_BoneWeights"};

		static constexpr Name_t		LightmapUV	{"at_LightmapUV"};
		static constexpr Name_t		TextureUVs[] = { "at_TextureUV", "at_TextureUV_1", "at_TextureUV_2", "at_TextureUV_3" };
		
		static constexpr Name_t		Color		{"at_Color"};

		static Name_t  GetName (const VertexName &);
	};


	struct EVertexAttribute
	{
		static constexpr VertexName		Position	{VertexAttributeName::Position};
		static constexpr VertexName		Position_1	{VertexAttributeName::Position_1};	// for morphing animation
		
		static constexpr VertexName		Normal		{VertexAttributeName::Normal};
		static constexpr VertexName		BiNormal	{VertexAttributeName::BiNormal};
		static constexpr VertexName		Tangent		{VertexAttributeName::Tangent};
		
		static constexpr VertexName		ObjectID	{VertexAttributeName::ObjectID};
		static constexpr VertexName		MaterialID	{VertexAttributeName::MaterialID};

		static constexpr VertexName		BoneWeights	{VertexAttributeName::BoneWeights};	// for sceletal animation
		
		static constexpr VertexName		Color		{VertexAttributeName::Color};

		static constexpr VertexName		LightmapUV	{VertexAttributeName::LightmapUV};
		static constexpr VertexName		TextureUVs[] = { VertexName{VertexAttributeName::TextureUVs[0]},
														 VertexName{VertexAttributeName::TextureUVs[1]},
														 VertexName{VertexAttributeName::TextureUVs[2]},
														 VertexName{VertexAttributeName::TextureUVs[3]} };
	};


}	// AE::Graphics
