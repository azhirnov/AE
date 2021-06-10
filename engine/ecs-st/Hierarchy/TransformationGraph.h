// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "ecs-st/Hierarchy/Components.h"
#include "ecs-st/Core/Archetype.h"

namespace AE::ECS::Systems
{

	//
	// Transformation Graph
	//

	class TransformationGraph final
	{
	// types
	private:

		class Node final : public EnableRC< Node >
		{
		public:
			using NodePtr = RC< Node >;

		public:
			NodePtr			parent;
			Array<NodePtr>	childs;
			Transform		global;
			EntityID		entity;

		public:
			Node () {}
			explicit Node (EntityID id) : entity{id} {}

			void AddChild (const NodePtr &n);
			void RemoveSelf ();
			void RemoveFromChilds ();
		};

		using NodePtr			= RC< Node >;
		using EntityToNode_t	= HashMap< EntityID, NodePtr >;


	// variables
	private:
		NodePtr				_root;
		EntityToNode_t		_entityToNode;
		Registry *			_owner;
		Array<Node*>		_temp;


	// methods
	public:
		explicit TransformationGraph (Registry &);
		~TransformationGraph ();

		void Update ();
	};

}	// AE::ECS::Systems
