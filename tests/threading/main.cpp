// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Common.h"

extern void UnitTest_Promise ();
extern void UnitTest_TaskDeps ();
extern void PerfTest_Threading ();
extern void UnitTest_LfLinearAllocator ();
extern void UnitTest_LfIndexedPool ();
extern void UnitTest_IndexedPool ();


#ifdef PLATFORM_ANDROID
extern int Test_Threading ()
#else
int main ()
#endif
{
	UnitTest_LfLinearAllocator();
	UnitTest_LfIndexedPool();
	UnitTest_IndexedPool();

	UnitTest_TaskDeps();
	UnitTest_Promise();

#if (not defined(AE_CI_BUILD)) and (not defined(PLATFORM_ANDROID))
	PerfTest_Threading();
#endif

	AE_LOGI( "Tests.Threading finished" );
	return 0;
}
