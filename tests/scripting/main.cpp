// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "scripting/Bindings/CoreBindings.h"

extern void UnitTest_Array ();
extern void UnitTest_Class ();
extern void UnitTest_MathFunc ();
extern void UnitTest_String ();


int main ()
{
	UnitTest_Array();
	UnitTest_Class();
	UnitTest_MathFunc();
	UnitTest_String();

	AE_LOGI( "Tests.ScriptBinding finished" );
	return 0;
}
