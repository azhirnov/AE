// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "ecs/Core/Archetype.h"

namespace AE::ECS::Components
{

	struct ParentID
	{
		struct Tag_Changed {};

		EntityID	id;
	};

	struct ChildIDs
	{
		struct Tag_Changed {};

		Array<EntityID>	ids;
	};

	struct LocalOffset
	{
		struct Tag_Changed {};

		float3	value;
	};

	struct LocalRotation
	{
		struct Tag_Changed {};

		QuatF	value;
	};

	/*struct AxialRotation
	{
		struct Tag_Changed {};

		QuatF	value;
	};*/

	struct GlobalTransformation
	{
		struct Tag_Changed {};

		Transform	value;
	};


}	// AE::ECS::Components

namespace AE::ECS::Systems
{
	//
	// Transform Manager
	//
	class TransformManager
	{
	// types
	private:
		class Node;
		using NodePtr = SharedPtr< Node >;


	// variables
	private:
		NodePtr		_root;


	// methods
	public:

		void OnComponentAdded ();
		void OnComponentRemoved ();
		void OnEntityDestroyed ();
	};


	//
	// Node
	//
	class TransformManager::Node final : public std::enabled_shared_from_this< Node >
	{
	// variables
	public:
		NodePtr			parent;
		Array<NodePtr>	childs;
		Transform		local;
	};


}	// AE::ECS::Systems
