// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "ecs-st/Hierarchy/TransformationGraph.h"
#include "ecs-st/Core/Registry.h"

namespace AE::ECS::Systems
{
	
/*
=================================================
	AddChild
=================================================
*/
	void TransformationGraph::Node::AddChild (const NodePtr &n)
	{
		childs.push_back( n );
		n->parent = GetRC();
	}

/*
=================================================
	RemoveSelf
=================================================
*/
	void TransformationGraph::Node::RemoveSelf ()
	{
		if ( not parent )
			return;

		for (auto iter = parent->childs.begin(); iter != parent->childs.end(); ++iter)
		{
			if ( iter->get() == this )
			{
				parent->childs.erase( iter );
				parent = null;
				return;
			}
		}

		CHECK( !"something goes wrong!" );
	}
	
/*
=================================================
	RemoveFromChilds
=================================================
*/
	void TransformationGraph::Node::RemoveFromChilds ()
	{
		for (auto& child : childs)
		{
			child->parent = null;
		}
		childs.clear();
	}
//-----------------------------------------------------------------------------


/*
=================================================
	constructor
=================================================
*/
	TransformationGraph::TransformationGraph (Registry &reg) :
		_root{ new Node{} },
		_owner{ &reg }
	{
		reg.AddMessageListener< Components::GlobalTransformation, MsgTag_AddedComponent >(
			[this] (ArrayView<EntityID> entities)
			{
				for (auto& id : entities)
				{
					NodePtr&	node = _entityToNode[id];
					if ( not node )
						node.reset( new Node{ id });

					if ( not node->parent )
						_root->AddChild( node );
				}
			});
		
		reg.AddMessageListener< Components::ParentID, MsgTag_ComponentChanged >(
			[this] (ArrayView<EntityID> entities, ArrayView<Components::ParentID> components)
			{
				CHECK_ERRV( entities.size() == components.size() );

				for (usize i = 0; i < entities.size(); ++i)
				{
					EntityID	id		= entities[i];
					NodePtr&	node	= _entityToNode[id];
					if ( not node )
						node.reset( new Node{ id });

					node->RemoveSelf();

					auto	iter = _entityToNode.find( components[i].value );

					if ( iter != _entityToNode.end() )
						iter->second->AddChild( node );
					else
						_root->AddChild( node );
				}
			});

		reg.AddMessageListener< Components::GlobalTransformation, MsgTag_RemovedComponent >(
			[this] (ArrayView<EntityID> entities)
			{
				for (auto& id : entities)
				{
					auto	iter = _entityToNode.find( id );
					if ( iter != _entityToNode.end() )
					{
						iter->second->RemoveSelf();
						_entityToNode.erase( iter );
					}
				}
			});
		
		reg.AddMessageListener< Components::ParentID, MsgTag_RemovedComponent >(
			[this] (ArrayView<EntityID> entities)
			{
				for (auto& id : entities)
				{
					auto	iter = _entityToNode.find( id );
					if ( iter != _entityToNode.end() )
					{
						iter->second->RemoveSelf();
					}
				}
			});
	}
	
/*
=================================================
	destructor
=================================================
*/
	TransformationGraph::~TransformationGraph ()
	{}
	
/*
=================================================
	Update
=================================================
*/
	void TransformationGraph::Update ()
	{
		_temp.push_back( _root.get() );

		for (; _temp.size();)
		{
			Node*	n = _temp.back();
			_temp.pop_back();

			auto[off, rot, scale, global]	= _owner->GetComponenets< Components::LocalOffset,
																	  Components::LocalRotation,
																	  Components::LocalScale,
																	  Components::GlobalTransformation >( n->entity );
			const Transform	local{
				off ? off->value : float3{},
				rot ? rot->value : QuatF{},
				scale ? scale->value : 1.0f };

			if ( n->parent )
				n->global = n->parent->global + local;
			else
				n->global = local;

			if ( global )
				global->value = n->global;

			for (auto& c : n->childs) {
				_temp.push_back( c.get() );
			}
		}
	}

}	// AE::ECS::Systems
