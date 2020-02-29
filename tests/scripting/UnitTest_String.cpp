// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "scripting/Bindings/CoreBindings.h"
#include "UnitTest_Common.h"


namespace
{
	static void  ScriptString_Test1 (const ScriptEnginePtr &se)
	{
		const char	script[] = R"#(
			int main () {
				string	str = "12345678";
				LogDebug( str );
				return str.length();
			}
		)#";

		int	res = 0;
		TEST( Run< int() >( se, script, "main", OUT res ));
		TEST( res == 8 );
	}


	static void  ScriptString_Test2 (const ScriptEnginePtr &se)
	{
		const char	script[] = R"#(
			int main () {
				string	str = "12345678";
				str += "abcd";
				LogDebug( str );
				return str.length();
			}
		)#";

		int	res = 0;
		TEST( Run< int() >( se, script, "main", OUT res ));
		TEST( res == 12 );
	}


	static void  ScriptString_Test3 (const ScriptEnginePtr &se)
	{
		const char	script[] = R"#(
			int main () {
				string	str = "abcd";
				str += 1234;
				LogDebug( str );
				return str.length();
			}
		)#";

		int	res = 0;
		TEST( Run< int() >( se, script, "main", OUT res ));
		TEST( res == 8 );
	}


	static void  ScriptString_Test4 (const ScriptEnginePtr &se)
	{
		const char	script[] = R"#(
			int main () {
				string	str = "a";
				str = str + 0 + "b" + 1 + "c" + 2;
				LogDebug( str );
				return (str == "a0b1c2" ? 1 : 0);
			}
		)#";

		int	res = 0;
		TEST( Run< int() >( se, script, "main", OUT res ));
		TEST( res == 1 );
	}


	static void  ScriptString_Test5 (const ScriptEnginePtr &se)
	{
		const char	script[] = R"#(
			int main (string str) {
				str = str + 0 + "b" + 1 + "c" + 2;
				LogDebug( str );
				return (str == "a0b1c2" ? 1 : 0);
			}
		)#";

		String	arg = "a";
		int		res = 0;
		TEST( Run< int(String) >( se, script, "main", OUT res, arg ));
		TEST( res == 1 );
	}


	static void  ScriptString_Test6 (const ScriptEnginePtr &se)
	{
		const char	script[] = R"#(
			string main () {
				string str = "a" + 0 + "b" + 1 + "c" + 2;
				return str;
			}
		)#";

		String	res;
		TEST( Run< String() >( se, script, "main", OUT res ));
		TEST( res == "a0b1c2" );
	}
}


extern void UnitTest_String ()
{
	auto	se = MakeShared<ScriptEngine>();
	TEST( se->Create() );

	CoreBindings::BindString( se );
	CoreBindings::BindLog( se );

	ScriptString_Test1( se );
	ScriptString_Test2( se );
	ScriptString_Test3( se );
	ScriptString_Test4( se );
	ScriptString_Test5( se );
	ScriptString_Test6( se );

	AE_LOGI( "UnitTest_String - passed" );
}
