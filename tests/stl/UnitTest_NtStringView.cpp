// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Containers/NtStringView.h"
#include "UnitTest_Common.h"


namespace
{
	void NtStringView_Test1 ()
	{
		const auto	Func = [] (NtStringView str, const char* expected) -> bool {
			return strcmp( str.data(), expected ) == 0;
		};

		TEST( Func( "test", "test" ));
		TEST( not Func( Default, "test" ));
		TEST( Func( StringView{"test"}, "test" ));
		TEST( Func( StringView{"test, test"}.substr(0,4), "test" ));
		TEST( Func( String{"test"}, "test" ));
		TEST( Func( String{"test, test"}.substr(0,4), "test" ));
	}
}


extern void UnitTest_NtStringView ()
{
	NtStringView_Test1();
	
	AE_LOGI( "UnitTest_NtStringView - passed" );
}
