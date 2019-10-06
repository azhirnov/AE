// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "script_binding/Impl/FunctionInfo.h"
#include "UnitTest_Common.h"

using namespace FGC;
using namespace FGScript::_fgscript_hidden_;


static void Fn1 ();
static int  Fn2 ();
static int  Fn3 (int, float);

struct Cl {
	void Fn1 ();
	int  Fn2 ();
	int  Fn3 (int, float) const;
	int  Fn4 (double) const volatile;
	//int  Fn5 () noexcept;
};


extern void UnitTest_FunctionInfo ()
{
	STATIC_ASSERT(( IsSameTypes< FunctionInfo< decltype(Fn1) >::clazz, void > ));
	STATIC_ASSERT(( IsSameTypes< FunctionInfo< decltype(Fn1) >::result, void > ));
	STATIC_ASSERT(( not FunctionInfo< decltype(Fn1) >::is_const ));
	STATIC_ASSERT(( not FunctionInfo< decltype(Fn1) >::is_volatile ));
	STATIC_ASSERT( FunctionInfo< decltype(Fn1) >::args::Count == 0 );

	STATIC_ASSERT(( IsSameTypes< FunctionInfo< decltype(Fn2) >::clazz, void > ));
	STATIC_ASSERT(( IsSameTypes< FunctionInfo< decltype(Fn2) >::result, int > ));
	STATIC_ASSERT(( not FunctionInfo< decltype(Fn2) >::is_const ));
	STATIC_ASSERT(( not FunctionInfo< decltype(Fn2) >::is_volatile ));
	STATIC_ASSERT( FunctionInfo< decltype(Fn2) >::args::Count == 0 );
	
	STATIC_ASSERT(( IsSameTypes< FunctionInfo< decltype(Fn3) >::clazz, void > ));
	STATIC_ASSERT(( IsSameTypes< FunctionInfo< decltype(Fn3) >::result, int > ));
	STATIC_ASSERT( FunctionInfo< decltype(Fn3) >::args::Count == 2 );
	STATIC_ASSERT(( IsSameTypes< FunctionInfo< decltype(Fn3) >::args::Get<0>, int > ));
	STATIC_ASSERT(( IsSameTypes< FunctionInfo< decltype(Fn3) >::args::Get<1>, float > ));
	
	STATIC_ASSERT(( IsSameTypes< FunctionInfo< decltype(&Cl::Fn1) >::clazz, Cl > ));
	STATIC_ASSERT(( IsSameTypes< FunctionInfo< decltype(&Cl::Fn1) >::result, void > ));
	STATIC_ASSERT(( not FunctionInfo< decltype(&Cl::Fn1) >::is_const ));
	STATIC_ASSERT(( not FunctionInfo< decltype(&Cl::Fn1) >::is_volatile ));
	STATIC_ASSERT( FunctionInfo< decltype(&Cl::Fn1) >::args::Count == 0 );
	
	STATIC_ASSERT(( IsSameTypes< FunctionInfo< decltype(&Cl::Fn2) >::clazz, Cl > ));
	STATIC_ASSERT(( IsSameTypes< FunctionInfo< decltype(&Cl::Fn2) >::result, int > ));
	STATIC_ASSERT(( not FunctionInfo< decltype(&Cl::Fn2) >::is_const ));
	STATIC_ASSERT(( not FunctionInfo< decltype(&Cl::Fn2) >::is_volatile ));
	STATIC_ASSERT( FunctionInfo< decltype(&Cl::Fn2) >::args::Count == 0 );

	STATIC_ASSERT(( IsSameTypes< FunctionInfo< decltype(&Cl::Fn3) >::clazz, Cl > ));
	STATIC_ASSERT(( IsSameTypes< FunctionInfo< decltype(&Cl::Fn3) >::result, int > ));
	STATIC_ASSERT(( FunctionInfo< decltype(&Cl::Fn3) >::is_const ));
	STATIC_ASSERT(( not FunctionInfo< decltype(&Cl::Fn3) >::is_volatile ));
	STATIC_ASSERT( FunctionInfo< decltype(&Cl::Fn3) >::args::Count == 2 );
	STATIC_ASSERT(( IsSameTypes< FunctionInfo< decltype(&Cl::Fn3) >::args::Get<0>, int > ));
	STATIC_ASSERT(( IsSameTypes< FunctionInfo< decltype(&Cl::Fn3) >::args::Get<1>, float > ));
	
	STATIC_ASSERT(( IsSameTypes< FunctionInfo< decltype(&Cl::Fn4) >::clazz, Cl > ));
	STATIC_ASSERT(( IsSameTypes< FunctionInfo< decltype(&Cl::Fn4) >::result, int > ));
	STATIC_ASSERT(( FunctionInfo< decltype(&Cl::Fn4) >::is_const ));
	STATIC_ASSERT(( FunctionInfo< decltype(&Cl::Fn4) >::is_volatile ));
	STATIC_ASSERT( FunctionInfo< decltype(&Cl::Fn4) >::args::Count == 1 );
	STATIC_ASSERT(( IsSameTypes< FunctionInfo< decltype(&Cl::Fn4) >::args::Get<0>, double > ));
	
	/*
	STATIC_ASSERT(( IsSameTypes< FunctionInfo< decltype(&Cl::Fn5) >::clazz, Cl > ))
	STATIC_ASSERT(( IsSameTypes< FunctionInfo< decltype(&Cl::Fn5) >::result, int > ));
	STATIC_ASSERT(( not FunctionInfo< decltype(&Cl::Fn5) >::is_const ));
	STATIC_ASSERT(( not FunctionInfo< decltype(&Cl::Fn5) >::is_volatile ));
	*/
	FG_LOGI( "UnitTest_FunctionInfo - passed" );
}
