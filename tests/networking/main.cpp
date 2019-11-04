// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Common.h"

extern void UnitTest_Http ();


int main ()
{
	UnitTest_Http();

	AE_LOGI( "Tests.Network finished" );
	return 0;
}
