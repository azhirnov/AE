// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "ecs-st/Core/Registry.h"
#include "UnitTest_Common.h"

namespace
{
	using Index_t = ArchetypeStorage::Index_t;

	struct Comp1
	{
		int		value;
	};

	struct Comp2
	{
		float	value;
	};

	struct Tag1 {};

	struct Event1 {};
	struct Event2 {};
	struct Event3 {};
	
	STATIC_ASSERT( not IsEmpty<Comp1> );
	STATIC_ASSERT( not IsEmpty<Comp2> );
	STATIC_ASSERT( IsEmpty<Tag1> );

	
	void ComponentValidator_Test1 ()
	{
	#ifdef AE_ECS_VALIDATE_SYSTEM_FN
		{
			using Types = TypeList< WriteAccess<Comp1>, ReadAccess<Comp2> >;
			STATIC_ASSERT( _reg_detail_::CheckForDuplicateComponents< Types::Get<0> >::Test< 0, Types >() );
			STATIC_ASSERT( _reg_detail_::CheckForDuplicateComponents< Types::Get<1> >::Test< 1, Types >() );
		}{
			using Types = TypeList< WriteAccess<Comp1>, WriteAccess<Comp2>, ReadAccess<Comp2> >;
			STATIC_ASSERT( _reg_detail_::CheckForDuplicateComponents< Types::Get<0> >::Test< 0, Types >() );
			STATIC_ASSERT( not _reg_detail_::CheckForDuplicateComponents< Types::Get<1> >::Test< 1, Types >() );
			STATIC_ASSERT( not _reg_detail_::CheckForDuplicateComponents< Types::Get<2> >::Test< 2, Types >() );
		}{
			using Types = TypeList< WriteAccess<Comp1>, OptionalReadAccess<Comp2>, ReadAccess<Comp2> >;
			STATIC_ASSERT( _reg_detail_::CheckForDuplicateComponents< Types::Get<0> >::Test< 0, Types >() );
			STATIC_ASSERT( not _reg_detail_::CheckForDuplicateComponents< Types::Get<1> >::Test< 1, Types >() );
			STATIC_ASSERT( not _reg_detail_::CheckForDuplicateComponents< Types::Get<2> >::Test< 2, Types >() );
		}{
			using Types = TypeList< WriteAccess<Comp1>, Require<Comp2, Tag1> >;
			STATIC_ASSERT( _reg_detail_::CheckForDuplicateComponents< Types::Get<0> >::Test< 0, Types >() );
			STATIC_ASSERT( _reg_detail_::CheckForDuplicateComponents< Types::Get<1> >::Test< 1, Types >() );
		}{
			using Types = TypeList< WriteAccess<Comp1>, Require<Comp1, Comp2, Tag1> >;
			STATIC_ASSERT( not _reg_detail_::CheckForDuplicateComponents< Types::Get<0> >::Test< 0, Types >() );
			STATIC_ASSERT( _reg_detail_::CheckForDuplicateComponents< Types::Get<1> >::Test< 1, Types >() );
		}{
			using Types = TypeList< Comp1*, Comp2* >;
			STATIC_ASSERT( _reg_detail_::SC_CheckForDuplicateComponents< Types::Get<0> >::Test< Types, 0 >() );
			STATIC_ASSERT( _reg_detail_::SC_CheckForDuplicateComponents< Types::Get<1> >::Test< Types, 1 >() );
		}{
			using Types = TypeList< Comp1*, Comp2& >;
			STATIC_ASSERT( _reg_detail_::SC_CheckForDuplicateComponents< Types::Get<0> >::Test< Types, 0 >() );
			STATIC_ASSERT( _reg_detail_::SC_CheckForDuplicateComponents< Types::Get<1> >::Test< Types, 1 >() );
		}{
			using Types = TypeList< Comp1 const*, Comp2* >;
			STATIC_ASSERT( _reg_detail_::SC_CheckForDuplicateComponents< Types::Get<0> >::Test< Types, 0 >() );
			STATIC_ASSERT( _reg_detail_::SC_CheckForDuplicateComponents< Types::Get<1> >::Test< Types, 1 >() );
		}{
			using Types = TypeList< Comp1*, Comp1& >;
			STATIC_ASSERT( not _reg_detail_::SC_CheckForDuplicateComponents< Types::Get<0> >::Test< Types, 0 >() );
			STATIC_ASSERT( not _reg_detail_::SC_CheckForDuplicateComponents< Types::Get<1> >::Test< Types, 1 >() );
		}
	#endif	// AE_ECS_VALIDATE_SYSTEM_FN
	}


	void Entity_Test1 ()
	{
		Registry	reg;

		// create
		EntityID	e1 = reg.CreateEntity<Comp1, Comp2>();
		TEST( e1 );
		{
			auto*	c11 = reg.GetComponent<Comp1>( e1 );
			auto*	c12 = reg.GetComponent<Comp2>( e1 );
			TEST( c11 );
			TEST( c12 );

			*c11 = Comp1{ 0x1234 };
			*c12 = Comp2{ 1.23f };
		}
		
		EntityID	t0 = reg.CreateEntity<Comp1, Comp2>();
		TEST( t0 );

		EntityID	t1 = reg.CreateEntity<Comp1, Comp2>();
		TEST( t1 );

		TEST( reg.DestroyEntity( t0 ));

		EntityID	e2 = reg.CreateEntity<Comp1, Comp2>();
		TEST( e2 );
		{
			auto*	c21 = reg.GetComponent<Comp1>( e2 );
			auto*	c22 = reg.GetComponent<Comp2>( e2 );
			TEST( c21 );
			TEST( c22 );

			*c21 = Comp1{ 0x111 };
			*c22 = Comp2{ 9.4f };

			reg.AssignComponent<Comp1>( e2 );
		}
		
		EntityID	t2 = reg.CreateEntity<Comp1>();
		TEST( t2 );

		EntityID	e3 = reg.CreateEntity();
		TEST( e3 );
		{
			auto&	c31 = reg.AssignComponent<Comp1>( e3 );
			c31.value = 0x777;
		}

		EntityID	e4 = reg.CreateEntity();
		TEST( e4 );
		{
			auto&	c42 = reg.AssignComponent<Comp2>( e4 );
			c42.value = 0.88f;
			
			TEST( not reg.RemoveComponent<Comp1>( e4 ));
		}
		
		EntityID	e5 = reg.CreateEntity<Comp1, Comp2>();
		TEST( e5 );
		{
			auto*	c51 = reg.GetComponent<Comp1>( e5 );
			auto*	c52 = reg.GetComponent<Comp2>( e5 );
			TEST( c51 );
			TEST( c52 );

			*c51 = Comp1{ 0x222 };
			*c52 = Comp2{ 5.3f };

			TEST( reg.RemoveComponent<Comp2>( e5 ));
		}

		TEST( reg.DestroyEntity( t1 ));
		TEST( reg.DestroyEntity( t2 ));


		// check
		{
			TEST( reg.GetArchetype( e1 ) == reg.GetArchetype( e2 ));
			TEST( reg.GetArchetype( e1 ) != reg.GetArchetype( e3 ));
			TEST( reg.GetArchetype( e1 ) != reg.GetArchetype( e4 ));
			TEST( reg.GetArchetype( e3 ) != reg.GetArchetype( e4 ));
			TEST( reg.GetArchetype( e3 ) == reg.GetArchetype( e5 ));
		}
		{
			auto*	c11 = reg.GetComponent<Comp1>( e1 );
			auto*	c12 = reg.GetComponent<Comp2>( e1 );
			TEST( c11 );
			TEST( c12 );
			TEST( c11->value == 0x1234 );
			TEST( c12->value == 1.23f );
		}
		{
			auto*	c21 = reg.GetComponent<Comp1>( e2 );
			auto*	c22 = reg.GetComponent<Comp2>( e2 );
			TEST( c21 );
			TEST( c22 );
			TEST( c21->value == 0x111 );
			TEST( c22->value == 9.4f );
		}
		{
			auto*	c31 = reg.GetComponent<Comp1>( e3 );
			TEST( c31 );
			TEST( c31->value == 0x777 );
			TEST( reg.GetComponent<Comp2>( e3 ) == null );
		}
		{
			auto*	c42 = reg.GetComponent<Comp2>( e4 );
			TEST( c42 );
			TEST( c42->value == 0.88f );
			TEST( reg.GetComponent<Comp1>( e4 ) == null );
		}
		{
			auto*	c51 = reg.GetComponent<Comp1>( e5 );
			TEST( c51 );
			TEST( c51->value == 0x222 );
			TEST( reg.GetComponent<Comp2>( e5 ) == null );
		}


		// destroy
		TEST( reg.DestroyEntity( e1 ));
		TEST( reg.DestroyEntity( e2 ));
		TEST( reg.DestroyEntity( e3 ));
		TEST( reg.DestroyEntity( e4 ));
		TEST( reg.DestroyEntity( e5 ));
	}


	void SingleComponent_Test1 ()
	{
		Registry	reg;

		TEST( not reg.RemoveSingleComponent<Comp1>() );
		TEST( reg.GetSingleComponent<Comp2>() == null );

		auto&	s1 = reg.AssignSingleComponent<Comp1>();
		s1.value = 0x8899;
		
		auto&	s2 = reg.AssignSingleComponent<Comp2>();
		s2.value = 6.32f;

		auto&	s3 = reg.AssignSingleComponent<Comp1>();
		TEST( s3.value == 0x8899 );

		auto*	s4 = reg.GetSingleComponent<Comp2>();
		TEST( s4 );
		TEST( s4->value == 6.32f );

		TEST( reg.RemoveSingleComponent<Comp2>() );
		TEST( reg.RemoveSingleComponent<Comp1>() );
	}


	void System_Test1 ()
	{
		Registry		reg;
		const size_t	count = 100;

		for (size_t i = 0; i < count; ++i)
		{
			EntityID	e1 = reg.CreateEntity<Comp1, Comp2>();
			TEST( e1 );
		}

		size_t	cnt = 0;
		reg.Enque(	[&cnt] (ArrayView<Tuple< size_t, WriteAccess<Comp1>, ReadAccess<Comp2> >> chunks)
					{
						for (auto& chunk : chunks)
						{
							std::apply(
								[&cnt] (size_t count, WriteAccess<Comp1> comp1, ReadAccess<Comp2> comp2)
								{
									for (size_t i = 0; i < count; ++i) {
										comp1[i].value = int(comp2[i].value);
										++cnt;
									}
								},
								chunk );
						}
					});
		reg.Process();

		TEST( cnt == count );

		reg.DestroyAllEntities();
	}
	
	
	void System_Test2 ()
	{
		struct SingleComp1
		{
			size_t	sum;
		};

		Registry		reg;
		const size_t	count = 100;

		// create archetypes
		for (size_t i = 0; i < count; ++i)
		{
			EntityID	e1 = reg.CreateEntity<Comp1, Comp2, Tag1>();
			EntityID	e2 = reg.CreateEntity<Comp1>();
			EntityID	e3 = reg.CreateEntity<Comp2, Tag1>();
			EntityID	e4 = reg.CreateEntity<Comp2, Comp1>();
			EntityID	e5 = reg.CreateEntity<Comp1, Tag1>();

			TEST( e1 and e2 and e3 and e4 and e5 );

			TEST( reg.GetArchetype( e1 ) != reg.GetArchetype( e2 ));
			TEST( reg.GetArchetype( e1 ) != reg.GetArchetype( e3 ));
			TEST( reg.GetArchetype( e1 ) != reg.GetArchetype( e4 ));
			TEST( reg.GetArchetype( e1 ) != reg.GetArchetype( e5 ));
			
			TEST( reg.GetArchetype( e2 ) != reg.GetArchetype( e3 ));
			TEST( reg.GetArchetype( e2 ) != reg.GetArchetype( e4 ));
			TEST( reg.GetArchetype( e2 ) != reg.GetArchetype( e5 ));
			
			TEST( reg.GetArchetype( e3 ) != reg.GetArchetype( e4 ));
			TEST( reg.GetArchetype( e3 ) != reg.GetArchetype( e5 ));
			
			TEST( reg.GetArchetype( e4 ) != reg.GetArchetype( e5 ));
		}

		// init single component
		{
			auto&	sc1 = reg.AssignSingleComponent<SingleComp1>();
			sc1.sum = 0;
		}

		size_t	cnt1 = 0;
		reg.Enque(	[&cnt1] (ArrayView<Tuple< size_t, ReadAccess<Comp1>, Subtractive<Tag1> >> chunks,
							 Tuple< SingleComp1& > single)
					{
						size_t&	sum = std::get<0>( single ).sum;

						for (auto& chunk : chunks)
						{
							for (size_t i = 0; i < std::get<0>(chunk); ++i) {
								sum += std::get<1>(chunk)[i].value;
								++cnt1;
							}
						}
					});

		size_t	cnt2 = 0;
		reg.Enque(	[&cnt2] (ArrayView<Tuple< size_t, ReadAccess<Comp2>, Require<Tag1, Comp1> >> chunks)
					{
						for (auto& chunk : chunks)
						{
							for (size_t i = 0; i < std::get<0>(chunk); ++i) {
								void( std::get<1>(chunk)[i].value );
								++cnt2;
							}
						}
					});

		reg.Process();
		
		TEST( cnt1 == count*2 );
		TEST( cnt2 == count );

		reg.DestroyAllEntities();
		reg.DestroyAllSingleComponents();
	}
	
	
	void System_Test3 ()
	{
		Registry	reg;

		// create archetypes
		{
			EntityID	e1 = reg.CreateEntity<Comp1, Comp2, Tag1>();
			EntityID	e2 = reg.CreateEntity<Comp1>();
			EntityID	e3 = reg.CreateEntity<Comp2, Tag1>();
			EntityID	e4 = reg.CreateEntity<Comp2, Comp1>();
			EntityID	e5 = reg.CreateEntity<Comp1, Tag1>();
			TEST( e1 and e2 and e3 and e4 and e5 );
		}
		
		Array<uint>	arr;
		reg.Enque(	[&reg, &arr] (ArrayView<Tuple< size_t, ReadAccess<Comp1> >>)
					{
						arr.push_back( 1 );
						
						reg.Enque(	[&arr] (ArrayView<Tuple< size_t, WriteAccess<Comp1>, ReadAccess<Comp2> >>)
									{
										arr.push_back( 2 );
									});
					});

		reg.Enque(	[&arr] (ArrayView<Tuple< size_t, ReadAccess<Comp2> >>)
					{
						arr.push_back( 3 );
					});

		reg.Process();
		
		TEST(( arr == Array<uint>{1, 2, 3} ));

		reg.DestroyAllEntities();
	}


	void Events_Test1 ()
	{
		Registry	reg;
		Array<uint>	arr;
		
		reg.AddEventListener<BeforeEvent<Event1>>(
			[&arr] ()
			{
				arr.push_back( 1 );
			});
		reg.AddEventListener<Event1>(
			[&reg, &arr] ()
			{
				arr.push_back( 2 );
				reg.EnqueEvent<Event2>();
			});
		reg.AddEventListener<AfterEvent<Event1>>(
			[&arr] ()
			{
				arr.push_back( 3 );
			});
		reg.AddEventListener<BeforeEvent<Event2>>(
			[&reg, &arr] ()
			{
				arr.push_back( 4 );
				reg.EnqueEvent<Event3>();
			});
		reg.AddEventListener<Event2>(
			[&arr] ()
			{
				arr.push_back( 5 );
			});
		reg.AddEventListener<Event3>(
			[&arr] ()
			{
				arr.push_back( 6 );
			});
		
		reg.EnqueEvent<Event1>();
		reg.Process();
		
		TEST(( arr == Array<uint>{1, 2, 3, 4, 5, 6} ));
	}


	void Messages_Test1 ()
	{
		Registry		reg;
		const size_t	count = 100;

		size_t	cnt1 = 0;
		reg.AddMessageListener<Comp1, MsgTag_RemovedComponent>(
			[&cnt1] (ArrayView<EntityID> entities, ArrayView<Comp1> components)
			{
				TEST( entities.size() == components.size() );
				cnt1 += entities.size();
			});

		Array<EntityID>	entities;

		for (size_t i = 0; i < count; ++i)
		{
			EntityID	e1 = reg.CreateEntity<Comp1>();
			TEST( e1 );
			entities.push_back( e1 );
		}

		reg.Process();
		TEST( cnt1 == 0 );
		
		for (size_t i = 0; i < entities.size(); ++i)
		{
			reg.DestroyEntity( entities[i] );
		}
		entities.clear();
		
		reg.Process();
		TEST( cnt1 == count );
	}


	void Messages_Test2 ()
	{
		Registry		reg;
		const size_t	count = 100;
		
		size_t	cnt1 = 0;
		reg.AddMessageListener<Comp1, MsgTag_ComponentChanged>(
			[&cnt1] (ArrayView<EntityID> entities)
			{
				cnt1 += entities.size();
			});
		
		for (size_t i = 0; i < count; ++i)
		{
			EntityID	e1 = reg.CreateEntity<Comp1>();
			TEST( e1 );
		}

		reg.Enque(	[&reg] (ArrayView<Tuple< size_t, ReadAccess<Comp1>, ReadAccess<EntityID> >> chunks)
					{
						for (auto& chunk : chunks)
						{
							std::apply(
								[&reg] (size_t count, ReadAccess<Comp1>, ReadAccess<EntityID> entities)
								{
									for (size_t i = 0; i < count; ++i)
									{
										reg.AddMessage<MsgTag_ComponentChanged>( entities[i], ComponentTypeInfo<Comp1>::id );
									}
								},
								chunk );
						}
					});
		reg.Process();

		TEST( cnt1 == count );

		reg.DestroyAllEntities();
	}
}


extern void UnitTest_Registry ()
{
	ComponentValidator_Test1();
	Entity_Test1();
	SingleComponent_Test1();
	System_Test1();
	System_Test2();
	System_Test3();
	Events_Test1();
	Messages_Test1();
	Messages_Test2();

	AE_LOGI( "UnitTest_Registry - passed" );
}
