// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#pragma once

#include "scripting/Impl/ScriptEngine.inl.h"

#include "stl/math/Vec.h"
#include "stl/math/Color.h"

namespace AE::Scripting
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


	AE_DECL_SCRIPT_TYPE( AE::STL::bool2,		"bool2" );
	AE_DECL_SCRIPT_TYPE( AE::STL::int2,			"int2" );
	AE_DECL_SCRIPT_TYPE( AE::STL::uint2,		"uint2" );
	AE_DECL_SCRIPT_TYPE( AE::STL::float2,		"float2"  );
	AE_DECL_SCRIPT_TYPE( AE::STL::double2,		"double2" );
	
	AE_DECL_SCRIPT_TYPE( AE::STL::bool3,		"bool3" );
	AE_DECL_SCRIPT_TYPE( AE::STL::int3,			"int3" );
	AE_DECL_SCRIPT_TYPE( AE::STL::uint3,		"uint3" );
	AE_DECL_SCRIPT_TYPE( AE::STL::float3,		"float3"  );
	AE_DECL_SCRIPT_TYPE( AE::STL::double3,		"double3" );
	
	AE_DECL_SCRIPT_TYPE( AE::STL::bool4,		"bool4" );
	AE_DECL_SCRIPT_TYPE( AE::STL::int4,			"int4" );
	AE_DECL_SCRIPT_TYPE( AE::STL::uint4,		"uint4" );
	AE_DECL_SCRIPT_TYPE( AE::STL::float4,		"float4"  );
	AE_DECL_SCRIPT_TYPE( AE::STL::double4,		"double4" );
	
	AE_DECL_SCRIPT_TYPE( AE::STL::RGBA32f,		"RGBA32f" );
	AE_DECL_SCRIPT_TYPE( AE::STL::RGBA32u,		"RGBA32u" );
	AE_DECL_SCRIPT_TYPE( AE::STL::RGBA32i,		"RGBA32i" );
	AE_DECL_SCRIPT_TYPE( AE::STL::RGBA8u,		"RGBA8u" );
	AE_DECL_SCRIPT_TYPE( AE::STL::DepthStencil,	"DepthStencil" );
	AE_DECL_SCRIPT_TYPE( AE::STL::HSVColor,		"HSVColor" );


}	// AE::Scripting
