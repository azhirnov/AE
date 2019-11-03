// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "ecs/Core/EntityPool.h"
#include "ecs/Core/Archetype.h"

namespace AE::ECS
{
	template <typename T>
	struct WriteAccess {
		T&	comp;

		T*  operator -> ()	{ return &comp; }
	};

	template <typename T>
	struct ReadAccess {
		T const&	comp;

		T const*  operator -> ()	{ return &comp; }
	};

	template <typename T>
	struct Subtractive {};


	//
	// Registry
	//

	class Registry final : public std::enable_shared_from_this< Registry >
	{
	// types
	private:
		class ArchetypeGroup;
		using ArchetypeMap_t = HashMap< ArchetypeID, ArchetypeGroup >;

		struct EntityRef
		{
			ArchetypeGroup*	group;
			uint			index;
		};


	// variables
	private:
		EntityPool		_entities;


	// methods
	public:

		// entity
			template <typename ...Components>
		ND_ EntityID	CreateEntity ();
		ND_ EntityID	CreateEntity ();
			void		DestroyEntity (EntityID id);

		ND_ ArchetypeID	GetArchetype (EntityID id);

		// component
			template <typename T>
			T&  AssignComponent (EntityID id);
		
			template <typename T>
			T&  InsertComponent (EntityID id);
		
			template <typename T>
			bool  RemoveComponent (EntityID id);

			template <typename T>
		ND_ T*  GetComponent (EntityID id);

		// system
			template <typename Fn>
			bool Call (Fn&& fn);

		// entity events
			void OnEntityDestroyed (Function<void (EntityID)> fn);

			template <typename Comp, typename Tag>
			void OnComponentChanged (Function<void (EntityID, Comp&)> fn);
	};
	


}	// AE::ECS
