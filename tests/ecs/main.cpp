// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Common.h"

extern void UnitTest_Temp ();


int main ()
{
	UnitTest_Temp();

	AE_LOGI( "Tests.ECS finished" );
	return 0;
}
