// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "script_binding/Bindings/CoreBindings.h"

extern void UnitTest_FunctionInfo ();
extern void UnitTest_MathFunc ();
extern void UnitTest_String ();
extern void UnitTest_Array ();
extern void UnitTest_Class ();


int main ()
{
	UnitTest_FunctionInfo();
	UnitTest_MathFunc();
	UnitTest_String();
	UnitTest_Array();
	UnitTest_Class();

	FG_LOGI( "Tests.ScriptBinding finished" );
	return 0;
}
