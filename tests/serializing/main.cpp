// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Common.h"

extern void UnitTest_Serialization ();


#ifdef PLATFORM_ANDROID
extern int Test_Serializing ()
#else
int main ()
#endif
{
	std::atexit( [] () { CHECK_FATAL( AE_DUMP_MEMLEAKS() ); });

	UnitTest_Serialization();

	AE_LOGI( "Tests.Serializing finished" );
	return 0;
}
