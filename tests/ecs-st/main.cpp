// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Common.h"

extern void UnitTest_Archetype ();
extern void UnitTest_EntityPool ();
extern void UnitTest_Registry ();


int main ()
{
	UnitTest_Archetype();
	UnitTest_EntityPool();
	UnitTest_Registry();

	AE_LOGI( "Tests.ECS finished" );
	return 0;
}
