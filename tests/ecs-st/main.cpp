// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Common.h"

extern void UnitTest_Archetype ();
extern void UnitTest_EntityPool ();
extern void UnitTest_Registry ();
extern void UnitTest_Transformation ();
extern void UnitTest_RendererVk ();


#ifdef PLATFORM_ANDROID
extern int Test_ECSst ()
#else
int main ()
#endif
{
	std::atexit( [] () { CHECK_FATAL( AE_DUMP_MEMLEAKS() ); });

	UnitTest_Archetype();
	UnitTest_EntityPool();
	UnitTest_Registry();
	UnitTest_Transformation();

#if !defined(AE_CI_BUILD) or (defined(AE_CI_TYPE) and (AE_CI_TYPE == 2))
	UnitTest_RendererVk();
#endif

	AE_LOGI( "Tests.ECS finished" );
	return 0;
}
