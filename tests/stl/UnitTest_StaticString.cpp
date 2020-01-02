// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Containers/StaticString.h"
#include "stl/CompileTime/Hash.h"
#include "UnitTest_Common.h"


namespace
{
	void StaticString_Test1 ()
	{
		String				str2 = "12345678";
		StaticString<64>	str1 = StringView{str2};

		TEST( str1.length() == str2.length() );
		TEST( str1.size() == str2.size() );
		TEST( str1 == str2 );
	}

	void StaticString_Test2 ()
	{
		String				str2 = "12345678";
		StaticString<64>	str1 = str2.data();

		TEST( str1.length() == str2.length() );
		TEST( str1.size() == str2.size() );
		TEST( str1 == str2 );
	}
}


extern void UnitTest_StaticString ()
{
	StaticString_Test1();
	StaticString_Test2();
	AE_LOGI( "UnitTest_StaticString - passed" );
}
