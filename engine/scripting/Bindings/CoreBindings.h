// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#pragma once

#include "scripting/Impl/ScriptEngine.inl.h"

#include "stl/Math/Vec.h"
#include "stl/Math/Color.h"

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


	AE_DECL_SCRIPT_TYPE( AE::Math::bool2,		"bool2" );
	AE_DECL_SCRIPT_TYPE( AE::Math::int2,		"int2" );
	AE_DECL_SCRIPT_TYPE( AE::Math::uint2,		"uint2" );
	AE_DECL_SCRIPT_TYPE( AE::Math::float2,		"float2"  );
	AE_DECL_SCRIPT_TYPE( AE::Math::double2,		"double2" );
	
	AE_DECL_SCRIPT_TYPE( AE::Math::bool3,		"bool3" );
	AE_DECL_SCRIPT_TYPE( AE::Math::int3,		"int3" );
	AE_DECL_SCRIPT_TYPE( AE::Math::uint3,		"uint3" );
	AE_DECL_SCRIPT_TYPE( AE::Math::float3,		"float3"  );
	AE_DECL_SCRIPT_TYPE( AE::Math::double3,		"double3" );
	
	AE_DECL_SCRIPT_TYPE( AE::Math::bool4,		"bool4" );
	AE_DECL_SCRIPT_TYPE( AE::Math::int4,		"int4" );
	AE_DECL_SCRIPT_TYPE( AE::Math::uint4,		"uint4" );
	AE_DECL_SCRIPT_TYPE( AE::Math::float4,		"float4"  );
	AE_DECL_SCRIPT_TYPE( AE::Math::double4,		"double4" );
	
	AE_DECL_SCRIPT_TYPE( AE::Math::RGBA32f,		"RGBA32f" );
	AE_DECL_SCRIPT_TYPE( AE::Math::RGBA32u,		"RGBA32u" );
	AE_DECL_SCRIPT_TYPE( AE::Math::RGBA32i,		"RGBA32i" );
	AE_DECL_SCRIPT_TYPE( AE::Math::RGBA8u,		"RGBA8u" );
	AE_DECL_SCRIPT_TYPE( AE::Math::DepthStencil,"DepthStencil" );
	AE_DECL_SCRIPT_TYPE( AE::Math::HSVColor,	"HSVColor" );


}	// AE::Scripting
