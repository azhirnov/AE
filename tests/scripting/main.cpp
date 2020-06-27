// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "scripting/Bindings/CoreBindings.h"

extern void UnitTest_Array ();
extern void UnitTest_Class ();
extern void UnitTest_MathFunc ();
extern void UnitTest_String ();


#ifdef PLATFORM_ANDROID
extern int Test_Scripting ()
#else
int main ()
#endif
{
	std::atexit( [] () { CHECK_FATAL( AE_DUMP_MEMLEAKS() ); });

	UnitTest_Array();
	UnitTest_Class();
	UnitTest_MathFunc();
	UnitTest_String();

	AE_LOGI( "Tests.ScriptBinding finished" );
	return 0;
}
