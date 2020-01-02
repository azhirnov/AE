// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "ecs-st/Hierarchy/TransformationGraph.h"
#include "ecs-st/Core/Registry.h"
#include "UnitTest_Common.h"

namespace
{
	struct UpdateTransform {};
	

	void InitRegistry (Registry &reg)
	{
		using namespace ECS::Components;
		reg.RegisterComponents< ParentID, LocalOffset, LocalRotation, LocalScale, GlobalTransformation >();
	}


	EntityID  CreateObj (Registry &reg, EntityID parent, const float3 &pos)
	{
		EntityID	id  = reg.CreateEntity< Components::ParentID,
											Components::LocalOffset,
											Components::GlobalTransformation >();
		TEST( id );
		
		auto		c_off = reg.GetComponent< Components::LocalOffset >( id );
		TEST( c_off );
		c_off->value = pos;

		auto		c_par = reg.GetComponent< Components::ParentID >( id );
		TEST( c_par );
		c_par->value = parent;
		reg.AddMessage<MsgTag_ComponentChanged>( id, *c_par );

		return id;
	}
	

	EntityID  CreateObj (Registry &reg, EntityID parent, const float3 &pos, const QuatF &rot)
	{
		EntityID	id  = reg.CreateEntity< Components::ParentID,
											Components::LocalOffset,
											Components::LocalRotation,
											Components::GlobalTransformation >();
		TEST( id );
		
		auto		c_off = reg.GetComponent< Components::LocalOffset >( id );
		TEST( c_off );
		c_off->value = pos;

		auto		c_par = reg.GetComponent< Components::ParentID >( id );
		TEST( c_par );
		c_par->value = parent;
		reg.AddMessage<MsgTag_ComponentChanged>( id, *c_par );

		auto		c_rot = reg.GetComponent< Components::LocalRotation >( id );
		TEST( c_rot );
		c_rot->value = rot;

		return id;
	}
	

	EntityID  CreateObj (Registry &reg, EntityID parent, const float3 &pos, const QuatF &rot, float scale)
	{
		EntityID	id  = reg.CreateEntity< Components::ParentID,
											Components::LocalOffset,
											Components::LocalRotation,
											Components::LocalScale,
											Components::GlobalTransformation >();
		TEST( id );
		
		auto		c_off = reg.GetComponent< Components::LocalOffset >( id );
		TEST( c_off );
		c_off->value = pos;

		auto		c_par = reg.GetComponent< Components::ParentID >( id );
		TEST( c_par );
		c_par->value = parent;
		reg.AddMessage<MsgTag_ComponentChanged>( id, *c_par );

		auto		c_rot = reg.GetComponent< Components::LocalRotation >( id );
		TEST( c_rot );
		c_rot->value = rot;

		auto		c_scale = reg.GetComponent< Components::LocalScale >( id );
		TEST( c_scale );
		c_scale->value = scale;

		return id;
	}


	void TransformationGraph_Test1 ()
	{
		Registry						reg;
		Systems::TransformationGraph	sg{ reg };

		InitRegistry( reg );

		reg.AddEventListener<UpdateTransform>( [&sg] (Registry &) { sg.Update(); });

		// initialize
		EntityID	e1 = CreateObj( reg, EntityID{}, float3{1.0f, 0.0f, 0.0f} );
		EntityID	e2 = CreateObj( reg, e1, float3{0.0f, 2.0f, 0.0f} );
		EntityID	e3 = CreateObj( reg, e2, float3{0.0f, 0.0f, 1.0f} );
		EntityID	e4 = CreateObj( reg, e1, float3{1.0f, 0.0f, 0.0f} );

		reg.EnqueEvent< UpdateTransform >();
		reg.Process();

		// test
		{
			auto	c_pid = reg.GetComponent< Components::ParentID >( e1 );
			TEST( c_pid );
			TEST( c_pid->value == EntityID{} );
			auto	c_gt = reg.GetComponent< Components::GlobalTransformation >( e1 );
			TEST( c_gt );
			TEST( c_gt->value == Transform{ float3{1.0f, 0.0f, 0.0f}, {} });
		}{
			auto	c_pid = reg.GetComponent< Components::ParentID >( e2 );
			TEST( c_pid );
			TEST( c_pid->value == e1 );
			auto	c_gt = reg.GetComponent< Components::GlobalTransformation >( e2 );
			TEST( c_gt );
			TEST( c_gt->value == Transform{ float3{1.0f, 2.0f, 0.0f}, {} });
		}{
			auto	c_pid = reg.GetComponent< Components::ParentID >( e3 );
			TEST( c_pid );
			TEST( c_pid->value == e2 );
			auto	c_gt = reg.GetComponent< Components::GlobalTransformation >( e3 );
			TEST( c_gt );
			TEST( c_gt->value == Transform{ float3{1.0f, 2.0f, 1.0f}, {} });
		}{
			auto	c_pid = reg.GetComponent< Components::ParentID >( e4 );
			TEST( c_pid );
			TEST( c_pid->value == e1 );
			auto	c_gt = reg.GetComponent< Components::GlobalTransformation >( e4 );
			TEST( c_gt );
			TEST( c_gt->value == Transform{ float3{2.0f, 0.0f, 0.0f}, {} });
		}

		reg.DestroyAllEntities();
	}
}


extern void UnitTest_Transformation ()
{
	TransformationGraph_Test1();

	AE_LOGI( "UnitTest_Transformation - passed" );
}
