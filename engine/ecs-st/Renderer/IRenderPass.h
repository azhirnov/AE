// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "ecs-st/Common.h"

namespace AE::ECS::Systems
{

	//
	// Render Pass interface
	//

	class IRenderPass
	{
	// interface
	public:
		virtual void Update () = 0;
		//virtual void Render () = 0;
	};


}	// AE::ECS::Systems
