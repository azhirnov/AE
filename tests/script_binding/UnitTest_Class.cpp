// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "script_binding/Impl/ClassBinder.h"
#include "script_binding/Impl/EnumBinder.h"
#include "UnitTest_Common.h"


// class
struct Test1_CL : AngelScriptHelper::SimpleRefCounter
{
	int i;

	Test1_CL() : i(3)
	{}

	Test1_CL(int i): i(i)
	{}

	int F () {
		return i;
	}
};
AE_DECL_SCRIPT_OBJ_RC( Test1_CL, "Test1_CL" );

static void ScriptClass_Test1 (const ScriptEnginePtr &se)
{
	ClassBinder<Test1_CL> binder( se );

	TEST( binder.CreateRef() );
	TEST( binder.AddMethod( &Test1_CL::F, "F" ));

	const char	script[] = R"#(
		int main (int i) {
			Test1_CL@ c = Test1_CL();
			return c.F() + i;
		}
	)#";

	int	res = 0;
	TEST( Run< int (int) >( se, script, "main", OUT res, 1 ));
	TEST( res == 3+1 );
}
//-----------------------------------------------------------------------------


// value class
struct Test2_Value
{
	int i;

	Test2_Value () : i(4) {}
	Test2_Value (int i) : i(i) {}

	void operator = (const Test2_Value &) {}

	int F () const {
		return i;
	}

	static int Append (Test2_Value &self, int x) {
		self.i += x;
		return self.i;
	}
};

AE_DECL_SCRIPT_OBJ( Test2_Value, "Test2_Value" );

static void ScriptClass_Test2 (const ScriptEnginePtr &se)
{
	ClassBinder<Test2_Value>	binder{ se };

	TEST( binder.CreateClassValue() );
	TEST( binder.AddMethod( &Test2_Value::F, "F" ));
	TEST( binder.AddMethodFromGlobal( &Test2_Value::Append, "Append" ));

	const char	script[] = R"#(
		int main (int i) {
			Test2_Value p = Test2_Value();
			p.Append( i );
			return p.F();
		}
	)#";

	int	res = 0;
	TEST( Run< int (int) >( se, script, "main", OUT res, 2 ));
	TEST( res == 4+2 );
}
//-----------------------------------------------------------------------------


// enum
enum class EEnum : uint
{
	Value1 = 1,
	Value2,
	Value3,
	Value4,
	_Count
};

AE_DECL_SCRIPT_TYPE( EEnum,	"EEnum" );

static void ScriptClass_Test3 (const ScriptEnginePtr &se)
{
	EnumBinder<EEnum>	binder{ se };

	TEST( binder.Create() );
	TEST( binder.AddValue( "Value1", EEnum::Value1 ));
	TEST( binder.AddValue( "Value2", EEnum::Value2 ));
	TEST( binder.AddValue( "Value3", EEnum::Value3 ));
	TEST( binder.AddValue( "Value4", EEnum::Value4 ));

	static const char script[] = R"#(
		int main () {
			EEnum e = EEnum_Value1;
			return e + EEnum_Value2;
		}
	)#";
	
	int	res = 0;
	TEST( Run< int () >( se, script, "main", OUT res ));
	TEST( res == 3 );
}
//-----------------------------------------------------------------------------


// script in script
class ScriptCl
{
public:
	static ScriptEnginePtr	engine;

	int Run (int value)
	{
		const char	script[] = R"#(
			int main (int x) {
				return 10 + x;
			}
		)#";

		int	res = 0;
		TEST( ::Run< int (int) >( engine, script, "main", OUT res, value ));

		return res;
	}
};

ScriptEnginePtr	ScriptCl::engine;

static void ScriptClass_Test4 (const ScriptEnginePtr &se)
{
	const char	script[] = R"#(
		int main ()
		{
			Script sc;
			return sc.Run( 1 );
		}
	)#";

	ScriptCl::engine = se;

	ClassBinder<ScriptCl>		binder{ se, "Script" };

	TEST( binder.CreateClassValue() );
	TEST( binder.AddMethod( &ScriptCl::Run, "Run" ));


	int	res = 0;
	TEST( Run< int() >( se, script, "main", OUT res ));
	TEST( res == 11 );
}
//-----------------------------------------------------------------------------


struct Test5_CL : AngelScriptHelper::SimpleRefCounter
{
	int i;

	Test5_CL() : i(3)
	{}

	Test5_CL(int i): i(i)
	{}

	int F () {
		return i;
	}
};
AE_DECL_SCRIPT_OBJ_RC( Test5_CL, "Test5_CL" );

static void ScriptClass_Test5 (const ScriptEnginePtr &se)
{
	ClassBinder<Test5_CL> binder( se );

	TEST( binder.CreateRef() );
	TEST( binder.AddMethod( &Test5_CL::F, "F" ));

	const char	script[] = R"#(
		int main (Test5_CL@ c) {
			return c.F() + 22;
		}
	)#";

	Test5_CL	arg{ 11 };
	int			res = 0;
	TEST( Run< int(Test5_CL*) >( se, script, "main", OUT res, &arg ));
	TEST( res == 11+22 );
	TEST( arg.__Counter() == 1 );
}
//-----------------------------------------------------------------------------


struct Test6_CL : AngelScriptHelper::SimpleRefCounter
{
	int i;

	Test6_CL() : i(0)
	{}

	void Set (int v) {
		this->i = v;
	}
};
AE_DECL_SCRIPT_OBJ_RC( Test6_CL, "Test6_CL" );

static void ScriptClass_Test6 (const ScriptEnginePtr &se)
{
	ClassBinder<Test6_CL> binder( se );

	TEST( binder.CreateRef() );
	TEST( binder.AddMethod( &Test6_CL::Set, "Set" ));

	const char	script[] = R"#(
		Test6_CL@ main () {
			Test6_CL@ c = Test6_CL();
			c.Set( 11 );
			return c;
		}
	)#";

	Test6_CL*	res = null;
	TEST( Run< Test6_CL*() >( se, script, "main", OUT res ));
	TEST( res );
	TEST( res->__Counter() == 1 );
	TEST( res->i == 11 );
}
//-----------------------------------------------------------------------------


// value class
struct Test7_Value
{
	int i;

	Test7_Value () : i(4) {}
	Test7_Value (int i) : i(i) {}

	int F () const {
		return i;
	}

	static int Append (Test7_Value &self, int x) {
		self.i += x;
		return self.i;
	}
};

AE_DECL_SCRIPT_OBJ( Test7_Value, "Test7_Value" );

static void ScriptClass_Test7 (const ScriptEnginePtr &se)
{
	ClassBinder<Test7_Value>	binder{ se };

	TEST( binder.CreateRef( &AngelScriptHelper::FactoryCreate<Test7_Value>, null, null, 0 ));
	TEST( binder.AddMethod( &Test7_Value::F, "F" ));
	TEST( binder.AddMethodFromGlobal( &Test7_Value::Append, "Append" ));

	const char	script[] = R"#(
		int main (Test7_Value& p) {
			p.Append( 5 );
			return p.F();
		}
	)#";

	Test7_Value	val{3};
	int			res = 0;
	TEST( Run< int (Test7_Value*) >( se, script, "main", OUT res, &val ));
	TEST( res == 3+5 );
}
//-----------------------------------------------------------------------------


struct Test8_CL : AngelScriptHelper::SimpleRefCounter
{
	int i;

	Test8_CL() : i(0)
	{}

	void Set (int v) {
		this->i = v;
	}
};
AE_DECL_SCRIPT_OBJ_RC( Test8_CL, "Test8_CL" );

using Test8_Ptr = AngelScriptHelper::SharedPtr<Test8_CL>;

static void ScriptClass_Test8 (const ScriptEnginePtr &se)
{
	ClassBinder<Test8_CL> binder( se );

	TEST( binder.CreateRef() );
	TEST( binder.AddMethod( &Test8_CL::Set, "Set" ));

	const char	script[] = R"#(
		Test8_CL@ main (Test8_CL@ c) {
			c.Set( 11 );
			Test8_CL@ a = Test8_CL();
			a.Set( 22 );
			return a;
		}
	)#";

	Test8_Ptr	arg{ new Test8_CL(), false };
	Test8_Ptr	res;

	TEST( Run< Test8_Ptr (Test8_Ptr) >( se, script, "main", OUT res, arg ));
	TEST( res );
	TEST( arg->__Counter() == 1 );
	TEST( arg->i == 11 );
	TEST( res->__Counter() == 1 );
	TEST( res->i == 22 );
}
//-----------------------------------------------------------------------------


extern void UnitTest_Class ()
{
	auto	se = MakeShared<ScriptEngine>();
	TEST( se->Create() );

	ScriptClass_Test1( se );
	ScriptClass_Test2( se );
	ScriptClass_Test3( se );
	ScriptClass_Test4( se );
	ScriptClass_Test5( se );
	ScriptClass_Test6( se );
	ScriptClass_Test7( se );
	ScriptClass_Test8( se );
	
	AE_LOGI( "UnitTest_Class - passed" );
}
