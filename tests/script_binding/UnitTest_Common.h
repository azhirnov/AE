// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Algorithms/Cast.h"
#include "stl/CompileTime/FunctionInfo.h"
#include "script_binding/Impl/ScriptFn.h"
#include "script_binding/Impl/ScriptEngine.inl.h"

using namespace AE::Script;

#define TEST	CHECK_FATAL

template <typename Fn, typename R, typename ...Args>
ND_ inline EnableIf< not IsSameTypes<typename FunctionInfo<Fn>::result, void>, bool >
Run (const ScriptEnginePtr &se, StringView script, const String &entry, OUT R &result, Args&& ...args)
{
	auto mod = se->CreateModule({ ScriptEngine::ModuleSource{"def", script} });
	auto scr = se->CreateScript<Fn>( entry, mod );
	auto res = scr->Run( std::forward<Args>(args)... );
	if ( not res.has_value() )
		return false;
	result = std::move(res.value());
	return true;
}

template <typename Fn, typename R, typename ...Args>
ND_ inline EnableIf< IsSameTypes<typename FunctionInfo<Fn>::result, void>, bool >
Run (const ScriptEnginePtr &se, StringView script, const String &entry, Args&& ...args)
{
	auto mod = se->CreateModule({ {"def", script} });
	auto scr = se->CreateScript<Fn>( entry, mod );
	return scr->Run( std::forward<Args>(args)... );
}
