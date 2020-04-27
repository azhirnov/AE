// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Common.h"

extern void UnitTest_Layouts ();


#ifdef PLATFORM_ANDROID
extern int Test_UI ()
#else
int main ()
#endif
{
	UnitTest_Layouts();

	AE_LOGI( "Tests.UI finished" );
	return 0;
}
