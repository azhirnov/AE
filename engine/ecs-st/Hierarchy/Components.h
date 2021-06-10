// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "ecs-st/Common.h"
#include "stl/Math/Transformation.h"

namespace AE::ECS::Components
{
	struct ParentID
	{
		EntityID	value;
	};

	struct LocalOffset
	{
		float3		value;
	};

	struct LocalRotation
	{
		QuatF		value;
	};

	struct LocalScale
	{
		float		value;
	};

	struct LocalTransformation
	{
		Transform	value;
	};
	
	struct GlobalTransformation
	{
		Transform	value;
	};

	struct TransformationLevel
	{
		uint		level : 8;
		uint		index : 24;

		TransformationLevel () : level{(1u << 8) - 1}, index{(1u << 24) - 1} {}
	};

}	// AE::ECS::Components
