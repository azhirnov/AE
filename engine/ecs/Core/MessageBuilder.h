// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "ecs/Core/Archetype.h"

namespace AE::ECS
{

	//
	// Message Builder
	//

	class MessageBuilder
	{
	// variables
	private:


	// methods
	public:

		template <typename Tag, typename Comp>
		void Add (EntityID id, Comp& comp);
	};


}	// AE::ECS
