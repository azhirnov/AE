// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

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
	
	struct GlobalTransformation
	{
		Transform	value;
	};

}	// AE::ECS::Components
