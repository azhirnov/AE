// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#pragma once

#include "script_binding/Impl/ScriptEngine.h"
#include "script_binding/Impl/ScriptModule.h"
#include "script_binding/Impl/ScriptTypes.h"

#include "stl/math/Vec.h"
#include "stl/math/Color.h"

namespace FGScript
{

	//
	// Core Bindings
	//

	struct CoreBindings final
	{
		static void BindScalarMath (const ScriptEnginePtr &se);
		static void BindVectorMath (const ScriptEnginePtr &se);
		static void BindColor (const ScriptEnginePtr &se);
		static void BindString (const ScriptEnginePtr &se);
		static void BindArray (const ScriptEnginePtr &se);
		static void BindLog (const ScriptEnginePtr &se);
	};


	FG_DECL_SCRIPT_TYPE( FGC::bool2,		"bool2" );
	FG_DECL_SCRIPT_TYPE( FGC::int2,			"int2" );
	FG_DECL_SCRIPT_TYPE( FGC::uint2,		"uint2" );
	FG_DECL_SCRIPT_TYPE( FGC::float2,		"float2"  );
	FG_DECL_SCRIPT_TYPE( FGC::double2,		"double2" );
	
	FG_DECL_SCRIPT_TYPE( FGC::bool3,		"bool3" );
	FG_DECL_SCRIPT_TYPE( FGC::int3,			"int3" );
	FG_DECL_SCRIPT_TYPE( FGC::uint3,		"uint3" );
	FG_DECL_SCRIPT_TYPE( FGC::float3,		"float3"  );
	FG_DECL_SCRIPT_TYPE( FGC::double3,		"double3" );
	
	FG_DECL_SCRIPT_TYPE( FGC::bool4,		"bool4" );
	FG_DECL_SCRIPT_TYPE( FGC::int4,			"int4" );
	FG_DECL_SCRIPT_TYPE( FGC::uint4,		"uint4" );
	FG_DECL_SCRIPT_TYPE( FGC::float4,		"float4"  );
	FG_DECL_SCRIPT_TYPE( FGC::double4,		"double4" );
	
	FG_DECL_SCRIPT_TYPE( FGC::RGBA32f,		"RGBA32f" );
	FG_DECL_SCRIPT_TYPE( FGC::RGBA32u,		"RGBA32u" );
	FG_DECL_SCRIPT_TYPE( FGC::RGBA32i,		"RGBA32i" );
	FG_DECL_SCRIPT_TYPE( FGC::RGBA8u,		"RGBA8u" );
	FG_DECL_SCRIPT_TYPE( FGC::DepthStencil,	"DepthStencil" );
	FG_DECL_SCRIPT_TYPE( FGC::HSVColor,		"HSVColor" );


}	// FGScript
