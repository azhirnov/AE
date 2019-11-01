// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Common.h"

extern void UnitTest_TaskDeps ();


int main ()
{
	UnitTest_TaskDeps();

	AE_LOGI( "Tests.Threading finished" );
	return 0;
}
