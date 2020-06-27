// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "graphics/Public/ResourceEnums.h"
#include "graphics/Public/EResourceState.h"
#include "graphics/Public/ShaderEnums.h"
#include "graphics/Public/VertexEnums.h"

namespace AE::Graphics
{

/*
=================================================
	EIndex_SizeOf
=================================================
*/
	ND_ inline BytesU  EIndex_SizeOf (EIndex value)
	{
		BEGIN_ENUM_CHECKS();
		switch ( value ) {
			case EIndex::UShort :	return SizeOf<uint16_t>;
			case EIndex::UInt :		return SizeOf<uint32_t>;
			case EIndex::Unknown :	break;
		}
		END_ENUM_CHECKS();
		RETURN_ERR( "unknown index type!" );
	}
//-----------------------------------------------------------------------------

	
/*
=================================================
	EShaderStages_FromShader
=================================================
*/
	ND_ inline EShaderStages  EShaderStages_FromShader (EShader value)
	{
		BEGIN_ENUM_CHECKS();
		switch ( value )
		{
			case EShader::Vertex :			return EShaderStages::Vertex;
			case EShader::TessControl :		return EShaderStages::TessControl;
			case EShader::TessEvaluation :	return EShaderStages::TessEvaluation;
			case EShader::Geometry :		return EShaderStages::Geometry;
			case EShader::Fragment :		return EShaderStages::Fragment;
			case EShader::Compute :			return EShaderStages::Compute;
			case EShader::MeshTask :		return EShaderStages::MeshTask;
			case EShader::Mesh :			return EShaderStages::Mesh;
			case EShader::RayGen :			return EShaderStages::RayGen;
			case EShader::RayAnyHit :		return EShaderStages::RayAnyHit;
			case EShader::RayClosestHit :	return EShaderStages::RayClosestHit;
			case EShader::RayMiss :			return EShaderStages::RayMiss;
			case EShader::RayIntersection :	return EShaderStages::RayIntersection;
			case EShader::RayCallable :		return EShaderStages::RayCallable;
			case EShader::Unknown :
			case EShader::_Count :			break;	// to shutup warnings
		}
		END_ENUM_CHECKS();
		RETURN_ERR( "unsupported shader type!" );
	}
//-----------------------------------------------------------------------------
	
	
/*
=================================================
	EVertexType_SizeOf
=================================================
*/
	ND_ inline BytesU  EVertexType_SizeOf (EVertexType type)
	{
		const EVertexType	scalar_type	= (type & EVertexType::_TypeMask);
		const uint			vec_size	= uint(type & EVertexType::_VecMask) >> uint(EVertexType::_VecOffset);

		switch ( scalar_type )
		{
			case EVertexType::_Byte :
			case EVertexType::_UByte :	return SizeOf<uint8_t> * vec_size;
			case EVertexType::_Short :
			case EVertexType::_UShort :	return SizeOf<uint16_t> * vec_size;
			case EVertexType::_Int :
			case EVertexType::_UInt :	return SizeOf<uint32_t> * vec_size;
			case EVertexType::_Long :
			case EVertexType::_ULong :	return SizeOf<uint64_t> * vec_size;

			case EVertexType::_Half :	return SizeOf<uint16_t> * vec_size;
			case EVertexType::_Float :	return SizeOf<uint32_t> * vec_size;
			case EVertexType::_Double :	return SizeOf<uint64_t> * vec_size;
		}
		RETURN_ERR( "not supported" );
	}
//-----------------------------------------------------------------------------
	


/*
=================================================
	EResourceState_IsReadable
=================================================
*/
	ND_ inline constexpr bool EResourceState_IsReadable (EResourceState value)
	{
		return AllBits( value, EResourceState::_Read );
	}
	
/*
=================================================
	EResourceState_IsWritable
=================================================
*/
	ND_ inline constexpr bool EResourceState_IsWritable (EResourceState value)
	{
		return AllBits( value, EResourceState::_Write );
	}

/*
=================================================
	EResourceState_FromShaders
=================================================
*/
	ND_ inline EResourceState  EResourceState_FromShaders (EShaderStages values)
	{
		EResourceState	result = Zero;
		
		for (EShaderStages t = EShaderStages(1 << 0); t < EShaderStages::_Last; t = EShaderStages(uint(t) << 1)) 
		{
			if ( not AllBits( values, t ))
				continue;
			
			BEGIN_ENUM_CHECKS();
			switch ( t )
			{
				case EShaderStages::Vertex :			result |= EResourceState::_VertexShader;			break;
				case EShaderStages::TessControl :		result |= EResourceState::_TessControlShader;		break;
				case EShaderStages::TessEvaluation :	result |= EResourceState::_TessEvaluationShader;	break;
				case EShaderStages::Geometry :			result |= EResourceState::_GeometryShader;			break;
				case EShaderStages::Fragment :			result |= EResourceState::_FragmentShader;			break;
				case EShaderStages::Compute :			result |= EResourceState::_ComputeShader;			break;
				case EShaderStages::MeshTask :			result |= EResourceState::_MeshTaskShader;			break;
				case EShaderStages::Mesh :				result |= EResourceState::_MeshShader;				break;
				case EShaderStages::RayGen :			result |= EResourceState::_RayGenShader;			break;
				case EShaderStages::RayAnyHit :			result |= EResourceState::_RayAnyHitShader;			break;
				case EShaderStages::RayClosestHit :		result |= EResourceState::_RayClosestHitShader;		break;
				case EShaderStages::RayMiss :			result |= EResourceState::_RayMissShader;			break;
				case EShaderStages::RayIntersection :	result |= EResourceState::_RayIntersectionShader;	break;
				case EShaderStages::RayCallable :		result |= EResourceState::_RayCallableShader;		break;
				case EShaderStages::_Last :
				case EShaderStages::All :
				case EShaderStages::AllGraphics :
				case EShaderStages::AllRayTracing :
				case EShaderStages::Unknown :
				default :								RETURN_ERR( "unknown shader type" );
			}
			END_ENUM_CHECKS();
		}
		return result;
	}
//-----------------------------------------------------------------------------


/*
=================================================
	EPixelFormat_GetInfo
=================================================
*/
	struct PixelFormatInfo
	{
		enum class EType
		{
			SFloat		= 1 << 0,
			UFloat		= 1 << 1,
			UNorm		= 1 << 2,
			SNorm		= 1 << 3,
			Int			= 1 << 4,
			UInt		= 1 << 5,
			Depth		= 1 << 6,
			Stencil		= 1 << 7,
			DepthStencil= Depth | Stencil,
			_ValueMask	= 0xFFFF,

			// flags
			sRGB		= 1 << 16,

			Unknown		= 0
		};

		EPixelFormat		format			= Default;
		uint				bitsPerBlock	= 0;		// for color and depth
		uint				bitsPerBlock2	= 0;		// for stencil
		EImageAspect		aspectMask		= Default;
		EType				valueType		= Default;
		uint2				blockSize		= {1,1};
		uint				channels		= 0;

		PixelFormatInfo () {}

		PixelFormatInfo (EPixelFormat fmt, uint bpp, uint channels, EType type, EImageAspect aspect = EImageAspect::Color) :
			format{fmt}, bitsPerBlock{bpp}, aspectMask{aspect}, valueType{type}, channels{channels} {}

		PixelFormatInfo (EPixelFormat fmt, uint bpp, const uint2 &size, uint channels, EType type, EImageAspect aspect = EImageAspect::Color) :
			format{fmt}, bitsPerBlock{bpp}, aspectMask{aspect}, valueType{type}, blockSize{size}, channels{channels} {}

		PixelFormatInfo (EPixelFormat fmt, uint2 depthStencilBPP, EType type = EType::DepthStencil, EImageAspect aspect = EImageAspect::DepthStencil) :
			format{fmt}, bitsPerBlock{depthStencilBPP.x}, bitsPerBlock2{depthStencilBPP.y}, aspectMask{aspect}, valueType{type} {}
	};
	AE_BIT_OPERATORS( PixelFormatInfo::EType );


	ND_ inline PixelFormatInfo const&  EPixelFormat_GetInfo (EPixelFormat value)
	{
		using EType = PixelFormatInfo::EType;

		static const PixelFormatInfo	fmt_infos[] = {
			{ EPixelFormat::RGBA16_SNorm,			16*4,			4,	EType::SNorm },
			{ EPixelFormat::RGBA8_SNorm,			8*4,			4,	EType::SNorm },
			{ EPixelFormat::RGB16_SNorm,			16*3,			3,	EType::SNorm },
			{ EPixelFormat::RGB8_SNorm,				8*3,			3,	EType::SNorm },
			{ EPixelFormat::RG16_SNorm,				16*2,			2,	EType::SNorm },
			{ EPixelFormat::RG8_SNorm,				8*2,			2,	EType::SNorm },
			{ EPixelFormat::R16_SNorm,				16*1,			1,	EType::SNorm },
			{ EPixelFormat::R8_SNorm,				8*1,			1,	EType::SNorm },
			{ EPixelFormat::RGBA16_UNorm,			16*4,			4,	EType::UNorm },
			{ EPixelFormat::RGBA8_UNorm,			8*4,			4,	EType::UNorm },
			{ EPixelFormat::RGB16_UNorm,			16*3,			3,	EType::UNorm },
			{ EPixelFormat::RGB8_UNorm,				8*3,			3,	EType::UNorm },
			{ EPixelFormat::RG16_UNorm,				16*2,			2,	EType::UNorm },
			{ EPixelFormat::RG8_UNorm,				8*2,			2,	EType::UNorm },
			{ EPixelFormat::R16_UNorm,				16*1,			1,	EType::UNorm },
			{ EPixelFormat::R8_UNorm,				8*1,			1,	EType::UNorm },
			{ EPixelFormat::RGB10_A2_UNorm,			10*3+2,			4,	EType::UNorm },
			{ EPixelFormat::RGBA4_UNorm,			4*4,			4,	EType::UNorm },
			{ EPixelFormat::RGB5_A1_UNorm,			5*3+1,			4,	EType::UNorm },
			{ EPixelFormat::RGB_5_6_5_UNorm,		5+6+5,			3,	EType::UNorm },
			{ EPixelFormat::BGR8_UNorm,				8*3,			3,	EType::UNorm },
			{ EPixelFormat::BGRA8_UNorm,			8*4,			4,	EType::UNorm },
			{ EPixelFormat::sRGB8,					8*3,			3,	EType::UNorm | EType::sRGB },
			{ EPixelFormat::sRGB8_A8,				8*4,			4,	EType::UNorm | EType::sRGB },
			{ EPixelFormat::sBGR8,					8*3,			3,	EType::UNorm | EType::sRGB },
			{ EPixelFormat::sBGR8_A8,				8*4,			4,	EType::UNorm | EType::sRGB },
			{ EPixelFormat::R8I,					8*1,			1,	EType::Int },
			{ EPixelFormat::RG8I,					8*2,			2,	EType::Int },
			{ EPixelFormat::RGB8I,					8*3,			3,	EType::Int },
			{ EPixelFormat::RGBA8I,					8*4,			4,	EType::Int },
			{ EPixelFormat::R16I,					16*1,			1,	EType::Int },
			{ EPixelFormat::RG16I,					16*2,			2,	EType::Int },
			{ EPixelFormat::RGB16I,					16*3,			3,	EType::Int },
			{ EPixelFormat::RGBA16I,				16*4,			4,	EType::Int },
			{ EPixelFormat::R32I,					32*1,			1,	EType::Int },
			{ EPixelFormat::RG32I,					32*2,			2,	EType::Int },
			{ EPixelFormat::RGB32I,					32*3,			3,	EType::Int },
			{ EPixelFormat::RGBA32I,				32*4,			4,	EType::Int },
			{ EPixelFormat::R8U,					8*1,			1,	EType::UInt },
			{ EPixelFormat::RG8U,					8*2,			2,	EType::UInt },
			{ EPixelFormat::RGB8U,					8*3,			3,	EType::UInt },
			{ EPixelFormat::RGBA8U,					8*4,			4,	EType::UInt },
			{ EPixelFormat::R16U,					16*1,			1,	EType::UInt },
			{ EPixelFormat::RG16U,					16*2,			2,	EType::UInt },
			{ EPixelFormat::RGB16U,					16*3,			3,	EType::UInt },
			{ EPixelFormat::RGBA16U,				16*4,			4,	EType::UInt },
			{ EPixelFormat::R32U,					32*1,			1,	EType::UInt },
			{ EPixelFormat::RG32U,					32*2,			2,	EType::UInt },
			{ EPixelFormat::RGB32U,					32*3,			3,	EType::UInt },
			{ EPixelFormat::RGBA32U,				32*4,			4,	EType::UInt },
			{ EPixelFormat::RGB10_A2U,				10*3+2,			4,	EType::UInt },
			{ EPixelFormat::R16F,					16*1,			1,	EType::SFloat },
			{ EPixelFormat::RG16F,					16*2,			2,	EType::SFloat },
			{ EPixelFormat::RGB16F,					16*3,			3,	EType::SFloat },
			{ EPixelFormat::RGBA16F,				16*4,			4,	EType::SFloat },
			{ EPixelFormat::R32F,					32*1,			1,	EType::SFloat },
			{ EPixelFormat::RG32F,					32*2,			2,	EType::SFloat },
			{ EPixelFormat::RGB32F,					32*3,			3,	EType::SFloat },
			{ EPixelFormat::RGBA32F,				32*4,			4,	EType::SFloat },
			{ EPixelFormat::RGB_11_11_10F,			11+11+10,		3,	EType::SFloat },
			{ EPixelFormat::Depth16,				{16, 0},			EType::UNorm  | EType::Depth,			EImageAspect::Depth },
			{ EPixelFormat::Depth24,				{24, 0},			EType::UNorm  | EType::Depth,			EImageAspect::Depth },
			{ EPixelFormat::Depth32F,				{32, 0},			EType::SFloat | EType::Depth,			EImageAspect::Depth },
			{ EPixelFormat::Depth16_Stencil8,		{16, 8},			EType::UNorm  | EType::DepthStencil,	EImageAspect::DepthStencil },
			{ EPixelFormat::Depth24_Stencil8,		{24, 8},			EType::UNorm  | EType::DepthStencil,	EImageAspect::DepthStencil },
			{ EPixelFormat::Depth32F_Stencil8,		{32, 8},			EType::SFloat | EType::DepthStencil,	EImageAspect::DepthStencil },
			{ EPixelFormat::BC1_RGB8_UNorm,			64,		{4,4},	3,	EType::UNorm },
			{ EPixelFormat::BC1_sRGB8_UNorm,		64,		{4,4},	3,	EType::UNorm | EType::sRGB },
			{ EPixelFormat::BC1_RGB8_A1_UNorm,		64,		{4,4},	4,	EType::UNorm },
			{ EPixelFormat::BC1_sRGB8_A1_UNorm,		64,		{4,4},	4,	EType::UNorm | EType::sRGB },
			{ EPixelFormat::BC2_RGBA8_UNorm,		128,	{4,4},	4,	EType::UNorm },
			{ EPixelFormat::BC3_RGBA8_UNorm,		128,	{4,4},	4,	EType::UNorm },
			{ EPixelFormat::BC3_sRGB,				128,	{4,4},	3,	EType::UNorm | EType::sRGB },
			{ EPixelFormat::BC4_R8_SNorm,			64,		{4,4},	1,	EType::SNorm },
			{ EPixelFormat::BC4_R8_UNorm,			64,		{4,4},	1,	EType::UNorm },
			{ EPixelFormat::BC5_RG8_SNorm,			128,	{4,4},	2,	EType::SNorm },
			{ EPixelFormat::BC5_RG8_UNorm,			128,	{4,4},	2,	EType::UNorm },
			{ EPixelFormat::BC7_RGBA8_UNorm,		128,	{4,4},	4,	EType::UNorm },
			{ EPixelFormat::BC7_SRGB8_A8,			128,	{4,4},	4,	EType::UNorm | EType::sRGB },
			{ EPixelFormat::BC6H_RGB16F,			128,	{4,4},	3,	EType::SFloat },
			{ EPixelFormat::BC6H_RGB16UF,			128,	{4,4},	3,	EType::UInt },
			{ EPixelFormat::ETC2_RGB8_UNorm,		64,		{4,4},	3,	EType::UNorm },
			{ EPixelFormat::ECT2_SRGB8,				64,		{4,4},	3,	EType::UNorm | EType::sRGB },
			{ EPixelFormat::ETC2_RGB8_A1_UNorm,		64,		{4,4},	4,	EType::UNorm },
			{ EPixelFormat::ETC2_SRGB8_A1,			64,		{4,4},	4,	EType::UNorm | EType::sRGB },
			{ EPixelFormat::ETC2_RGBA8_UNorm,		128,	{4,4},	4,	EType::UNorm },
			{ EPixelFormat::ETC2_SRGB8_A8,			128,	{4,4},	4,	EType::UNorm | EType::sRGB },
			{ EPixelFormat::EAC_R11_SNorm,			64,		{4,4},	1,	EType::SNorm },
			{ EPixelFormat::EAC_R11_UNorm,			64,		{4,4},	1,	EType::UNorm },
			{ EPixelFormat::EAC_RG11_SNorm,			128,	{4,4},	2,	EType::SNorm },
			{ EPixelFormat::EAC_RG11_UNorm,			128,	{4,4},	2,	EType::UNorm },
			{ EPixelFormat::ASTC_RGBA_4x4,			128,	{4,4},	 4,	EType::UNorm },
			{ EPixelFormat::ASTC_RGBA_5x4,			128,	{5,4},	 4,	EType::UNorm },
			{ EPixelFormat::ASTC_RGBA_5x5,			128,	{5,5},	 4,	EType::UNorm },
			{ EPixelFormat::ASTC_RGBA_6x5,			128,	{6,5},	 4,	EType::UNorm },
			{ EPixelFormat::ASTC_RGBA_6x6,			128,	{6,6},	 4,	EType::UNorm },
			{ EPixelFormat::ASTC_RGBA_8x5,			128,	{8,5},	 4,	EType::UNorm },
			{ EPixelFormat::ASTC_RGBA_8x6,			128,	{8,6},	 4,	EType::UNorm },
			{ EPixelFormat::ASTC_RGBA_8x8,			128,	{8,8},	 4,	EType::UNorm },
			{ EPixelFormat::ASTC_RGBA_10x5,			128,	{10,5},	 4,	EType::UNorm },
			{ EPixelFormat::ASTC_RGBA_10x6,			128,	{10,6},	 4,	EType::UNorm },
			{ EPixelFormat::ASTC_RGBA_10x8,			128,	{10,8},	 4,	EType::UNorm },
			{ EPixelFormat::ASTC_RGBA_10x10,		128,	{10,10}, 4,	EType::UNorm },
			{ EPixelFormat::ASTC_RGBA_12x10,		128,	{12,10}, 4,	EType::UNorm },
			{ EPixelFormat::ASTC_RGBA_12x12,		128,	{12,12}, 4,	EType::UNorm },
			{ EPixelFormat::ASTC_SRGB8_A8_4x4,		128,	{4,4},	 4,	EType::UNorm | EType::sRGB },
			{ EPixelFormat::ASTC_SRGB8_A8_5x4,		128,	{5,4},	 4,	EType::UNorm | EType::sRGB },
			{ EPixelFormat::ASTC_SRGB8_A8_5x5,		128,	{5,5},	 4,	EType::UNorm | EType::sRGB },
			{ EPixelFormat::ASTC_SRGB8_A8_6x5,		128,	{6,5},	 4,	EType::UNorm | EType::sRGB },
			{ EPixelFormat::ASTC_SRGB8_A8_6x6,		128,	{6,6},	 4,	EType::UNorm | EType::sRGB },
			{ EPixelFormat::ASTC_SRGB8_A8_8x5,		128,	{8,5},	 4,	EType::UNorm | EType::sRGB },
			{ EPixelFormat::ASTC_SRGB8_A8_8x6,		128,	{8,6},	 4,	EType::UNorm | EType::sRGB },
			{ EPixelFormat::ASTC_SRGB8_A8_8x8,		128,	{8,8},	 4,	EType::UNorm | EType::sRGB },
			{ EPixelFormat::ASTC_SRGB8_A8_10x5,		128,	{10,5},	 4,	EType::UNorm | EType::sRGB },
			{ EPixelFormat::ASTC_SRGB8_A8_10x6,		128,	{10,6},	 4,	EType::UNorm | EType::sRGB },
			{ EPixelFormat::ASTC_SRGB8_A8_10x8,		128,	{10,8},	 4,	EType::UNorm | EType::sRGB },
			{ EPixelFormat::ASTC_SRGB8_A8_10x10,	128,	{10,10}, 4,	EType::UNorm | EType::sRGB },
			{ EPixelFormat::ASTC_SRGB8_A8_12x10,	128,	{12,10}, 4,	EType::UNorm | EType::sRGB },
			{ EPixelFormat::ASTC_SRGB8_A8_12x12,	128,	{12,12}, 4,	EType::UNorm | EType::sRGB },
			{ EPixelFormat::Unknown,				0,				 0,	EType::Unknown }
		};
		STATIC_ASSERT( CountOf(fmt_infos) == uint(EPixelFormat::_Count)+1 );

		auto&	result = fmt_infos[ value < EPixelFormat::_Count ? uint(value) : uint(EPixelFormat::_Count) ];
		ASSERT( result.format == value );

		return result;
	}
	
/*
=================================================
	EPixelFormat_BitPerPixel
=================================================
*/
	ND_ inline uint  EPixelFormat_BitPerPixel (EPixelFormat value, EImageAspect aspect)
	{
		auto	info = EPixelFormat_GetInfo( value );
		ASSERT( AllBits( info.aspectMask, aspect ) );

		if ( aspect != EImageAspect::Stencil )
			return info.bitsPerBlock / (info.blockSize.x * info.blockSize.y);
		else
			return info.bitsPerBlock2 / (info.blockSize.x * info.blockSize.y);
	}

/*
=================================================
	EPixelFormat_Is***
=================================================
*/
	ND_ inline bool  EPixelFormat_IsDepth (EPixelFormat value)
	{
		return EPixelFormat_GetInfo( value ).valueType == PixelFormatInfo::EType::Depth;
	}
	
	ND_ inline bool  EPixelFormat_IsStencil (EPixelFormat value)
	{
		return EPixelFormat_GetInfo( value ).valueType == PixelFormatInfo::EType::Stencil;
	}
	
	ND_ inline bool  EPixelFormat_IsDepthStencil (EPixelFormat value)
	{
		return EPixelFormat_GetInfo( value ).valueType == PixelFormatInfo::EType::DepthStencil;
	}
	
	ND_ inline bool  EPixelFormat_IsColor (EPixelFormat value)
	{
		return not AnyBits( EPixelFormat_GetInfo( value ).valueType, PixelFormatInfo::EType::DepthStencil );
	}

/*
=================================================
	EPixelFormat_Has***
=================================================
*/
	ND_ inline bool  EPixelFormat_HasDepth (EPixelFormat value)
	{
		return AllBits( EPixelFormat_GetInfo( value ).valueType, PixelFormatInfo::EType::Depth );
	}
	
	ND_ inline bool  EPixelFormat_HasStencil (EPixelFormat value)
	{
		return AllBits( EPixelFormat_GetInfo( value ).valueType, PixelFormatInfo::EType::Stencil );
	}
	
	ND_ inline bool  EPixelFormat_HasDepthOrStencil (EPixelFormat value)
	{
		return AnyBits( EPixelFormat_GetInfo( value ).valueType, PixelFormatInfo::EType::DepthStencil );
	}
	
/*
=================================================
	EPixelFormat_ToImageAspect
=================================================
*/
	ND_ inline EImageAspect  EPixelFormat_ToImageAspect (EPixelFormat format)
	{
		auto&	fmt_info	= EPixelFormat_GetInfo( format );
		auto	depth_bit	= AllBits( fmt_info.valueType, PixelFormatInfo::EType::Depth ) ? EImageAspect::Depth : Default;
		auto	stencil_bit	= AllBits( fmt_info.valueType, PixelFormatInfo::EType::Stencil ) ? EImageAspect::Stencil : Default;
		auto	color_bit	= (not (depth_bit | stencil_bit) ? EImageAspect::Color : Default);

		return depth_bit | stencil_bit | color_bit;
	}


}	// AE::Graphics
