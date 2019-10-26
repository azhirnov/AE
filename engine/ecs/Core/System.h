// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "ecs/Common.h"

namespace AE::ECS
{

	template <typename T>
	struct SubtractiveComp {};

	template <typename T>
	struct RequiredComp {};


	//
	// System
	//

	class BaseSystem : public std::enable_shared_from_this<BaseSystem>
	{
	};

}	// AE::ECS
