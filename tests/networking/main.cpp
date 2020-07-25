// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Common.h"

extern void UnitTest_Http ();


#ifdef PLATFORM_ANDROID
extern int Test_Networking ()
#else
int main ()
#endif
{
	UnitTest_Http();

	CHECK_FATAL( AE_DUMP_MEMLEAKS() );

	AE_LOGI( "Tests.Network finished" );
	return 0;
}
