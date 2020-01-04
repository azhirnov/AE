// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "graphics/Public/Common.h"

namespace AE::Graphics
{

	enum class EShader : uint
	{
		Vertex,
		TessControl,
		TessEvaluation,
		Geometry,
		Fragment,
		Compute,

		MeshTask,
		Mesh,
		
		RayGen,
		RayAnyHit,
		RayClosestHit,
		RayMiss,
		RayIntersection,
		RayCallable,

		_Count,
		Unknown		= ~0u,
	};

	
	enum class EShaderStages : uint
	{
		Vertex			= 1 << uint(EShader::Vertex),
		TessControl		= 1 << uint(EShader::TessControl),
		TessEvaluation	= 1 << uint(EShader::TessEvaluation),
		Geometry		= 1 << uint(EShader::Geometry),
		Fragment		= 1 << uint(EShader::Fragment),
		Compute			= 1 << uint(EShader::Compute),
		MeshTask		= 1 << uint(EShader::MeshTask),
		Mesh			= 1 << uint(EShader::Mesh),
		RayGen			= 1 << uint(EShader::RayGen),
		RayAnyHit		= 1 << uint(EShader::RayAnyHit),
		RayClosestHit	= 1 << uint(EShader::RayClosestHit),
		RayMiss			= 1 << uint(EShader::RayMiss),
		RayIntersection	= 1 << uint(EShader::RayIntersection),
		RayCallable		= 1 << uint(EShader::RayCallable),
		_Last,

		All				= ((_Last-1) << 1) - 1,
		AllGraphics		= Vertex | TessControl | TessEvaluation | Geometry | MeshTask | Mesh | Fragment,
		AllRayTracing	= RayGen | RayAnyHit | RayClosestHit | RayMiss | RayIntersection | RayCallable,
		Unknown			= 0,
	};
	AE_BIT_OPERATORS( EShaderStages );

	ND_ forceinline EShaderStages  operator |  (EShaderStages lhs, EShader rhs)		{ return lhs | EShaderStages(1 << uint(rhs)); }
		forceinline EShaderStages  operator |= (EShaderStages &lhs, EShader rhs)	{ return (lhs |= EShaderStages(1 << uint(rhs))); }
	


}	// AE::Graphics
