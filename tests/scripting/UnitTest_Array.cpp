// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "scripting/Bindings/CoreBindings.h"
#include "UnitTest_Common.h"

namespace
{
	static void  ScriptArray_Test1 (const ScriptEnginePtr &se)
	{
		const char	script[] = R"#(
			int main () {
				array<int>	arr;
				arr.push_back( 1 );
				arr.push_back( 2 );
				arr.insert( /*pos*/1, 3 );
				arr.pop_back();
				return arr[1];
			}
		)#";

		int	res = 0;
		TEST( Run< int() >( se, script, "main", OUT res ));
		TEST( res == 3 );
	}
}


extern void UnitTest_Array ()
{
	auto	se = MakeShared<ScriptEngine>();
	TEST( se->Create() );

	CoreBindings::BindArray( se );

	ScriptArray_Test1( se );

	AE_LOGI( "UnitTest_Array - passed" );
}
