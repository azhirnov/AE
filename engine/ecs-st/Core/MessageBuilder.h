// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "ecs-st/Common.h"

namespace AE::ECS
{
	struct Tag_AddedComponent {};
	struct Tag_RemovedComponent {};


	//
	// Message Builder
	//

	class MessageBuilder
	{
	// variables
	private:


	// methods
	public:

		template <typename Tag>
		void Add (EntityID id, ComponentID compId);
		
		template <typename Tag, typename Comp>
		void Add (EntityID id, Comp&& comp);
	};


}	// AE::ECS
