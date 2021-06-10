// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

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
	UnitTest_Array();
	UnitTest_Class();
	UnitTest_MathFunc();
	UnitTest_String();
	
	CHECK_FATAL( AE_DUMP_MEMLEAKS() );

	AE_LOGI( "Tests.ScriptBinding finished" );
	return 0;
}
