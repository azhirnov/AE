// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "script_binding/Bindings/CoreBindings.h"
#include "UnitTest_Common.h"


static void ScriptMath_Test1 (ScriptEngine &se)
{
	int script_res;
	bool res = se.Run( R"#(
		int main() {
			return Max( 2, -4 );
		})#", "main", OUT script_res );
	TEST( res );
	TEST( script_res == 2 );
}


static void ScriptMath_Test2 (ScriptEngine &se)
{
	int script_res;
	bool res = se.Run( R"#(
		int main() {
			uint2 a( 1, 2 );
			uint2 b = a + 4;
			uint2 c = b & 1;
			return c.x == 1 ? 1 : c.y == 1 ? 2 : 0;
		})#", "main", OUT script_res );
	TEST( res );
	TEST( script_res == 1 );
}


extern void UnitTest_MathFunc ()
{
	auto	se = MakeShared<ScriptEngine>();
	TEST( se->Create() );

	CoreBindings::BindScalarMath( se );
	CoreBindings::BindVectorMath( se );

	ScriptMath_Test1( *se );
	ScriptMath_Test2( *se );

	FG_LOGI( "UnitTest_MathFunc - passed" );
}
