// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Common.h"

extern void UnitTest_Archetype ();
extern void UnitTest_EntityPool ();
extern void UnitTest_Registry ();
extern void UnitTest_Transformation ();


int main ()
{
	UnitTest_Archetype();
	UnitTest_EntityPool();
	UnitTest_Registry();
	UnitTest_Transformation();

	AE_LOGI( "Tests.ECS finished" );
	return 0;
}
