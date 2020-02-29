// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/CompileTime/FunctionInfo.h"

#include "stl/Math/Bytes.h"
#include "stl/Math/BitMath.h"
#include "stl/Math/Vec.h"

#include "stl/Types/HandleTmpl.h"
#include "stl/Types/TypeId.h"
#include "stl/Types/Noncopyable.h"

#include "stl/Containers/FixedArray.h"
#include "stl/Containers/ArrayView.h"

#include "stl/Memory/UntypedAllocator.h"
#include "stl/Algorithms/ArrayUtils.h"

#include "threading/Primitives/DataRaceCheck.h"

#define AE_ECS_VALIDATE_SYSTEM_FN

#ifndef AE_ECS_ENABLE_DEFAULT_MESSAGES
#	define AE_ECS_ENABLE_DEFAULT_MESSAGES	1
#endif


namespace AE::ECS
{
	using namespace AE::STL;

	namespace Components {}
	namespace SingleComponents {}
	namespace Systems {}
	namespace Events {}

	using Threading::DataRaceCheck;
	using Threading::Atomic;

	using EntityID	= HandleTmpl< uint16_t, uint16_t, 1 << 10 >;
	using QueryID	= HandleTmpl< uint16_t, uint16_t, 2 << 10 >;
	

	struct ECS_Config
	{
		static constexpr uint	MaxComponents				= 4 * 64;
		static constexpr uint	MaxComponentsPerArchetype	= 64;
		static constexpr uint	InitialtStorageSize			= 16;
	};

	class Registry;

}	// AE::ECS
