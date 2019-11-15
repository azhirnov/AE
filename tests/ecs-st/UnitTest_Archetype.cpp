// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "ecs-st/Core/Archetype.h"
#include "ecs-st/Core/EntityPool.h"
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

	void Archetype_Test1 ()
	{
		ArchetypeDesc		desc;
		desc.Add<Comp1>();
		desc.Add<Comp2>();

		TEST( desc.components[0].align == alignof(Comp1) );
		TEST( desc.components[0].size  == sizeof(Comp1) );
		TEST( desc.components[1].align == alignof(Comp2) );
		TEST( desc.components[1].size  == sizeof(Comp2) );

		Archetype			arch{ desc };

		ArchetypeStorage	storage{ arch, 1024 };
		EntityPool			pool;

		EntityID	id_1, id_2;
		TEST( pool.Assign( OUT id_1 ));
		TEST( pool.Assign( OUT id_2 ));

		Index_t		index_1, index_2;
		TEST( storage.Add( id_1, OUT index_1 ));
		TEST( storage.Add( id_2, OUT index_2 ));

		TEST( storage.IsValid( id_1, index_1 ));
		TEST( storage.IsValid( id_2, index_2 ));
		TEST( not storage.IsValid( id_2, index_1 ));
		TEST( not storage.IsValid( id_1, index_2 ));

		EntityID	m1, m2;
		TEST( storage.Erase( index_2, OUT m2 ));	// TODO: this is incorrect test
		TEST( storage.Erase( index_1, OUT m1 ));
		
		TEST( pool.Unassign( id_1 ));
		TEST( pool.Unassign( id_2 ));
	}
}


extern void UnitTest_Archetype ()
{
	Archetype_Test1();

	AE_LOGI( "UnitTest_Archetype - passed" );
}
