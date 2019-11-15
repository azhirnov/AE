// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/CompileTime/FunctionInfo.h"

#include "stl/Math/Bytes.h"
#include "stl/Math/Vec.h"
//#include "stl/Math/Transformation.h"

#include "stl/Types/HandleTmpl.h"
#include "stl/Types/TypeId.h"
#include "stl/Types/Noncopyable.h"

#include "stl/Containers/FixedArray.h"
#include "stl/Containers/ArrayView.h"

#include "stl/Memory/UntypedAllocator.h"
#include "stl/Algorithms/ArrayUtils.h"

#include "threading/Primitives/DataRaceCheck.h"

#define AE_ECS_VALIDATE_SYSTEM_FN


namespace AE::ECS
{
	using namespace AE::STL;

	namespace Components {}
	namespace SingleComponent {}
	namespace Systems {}

	using DataRaceCheck	= Threading::DataRaceCheck;

	using EntityID		= HandleTmpl< uint16_t, uint16_t, 0 >;
	using ArchetypeID	= HandleTmpl< uint16_t, uint16_t, 1 >;
	

	struct ECS_Config
	{
		static constexpr uint	MaxComponents		= 64;
		static constexpr uint	MaxTagComponents	= 16;
		static constexpr uint	DefaultStorageSize	= 1024;
	};

}	// AE::ECS
