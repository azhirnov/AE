// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "script_binding/Impl/ScriptEngine.h"
#include "script_binding/Impl/ScriptTypes.h"

namespace FGScript
{

/*
=================================================
	AddFunction
=================================================
*/
	template <typename T>
	inline void  ScriptEngine::AddFunction (T func, StringView name)
	{
		using namespace AngelScript;

		String	signature;
		GlobalFunction<T>::GetDescriptor( OUT signature, name );

		AS_CALL( _engine->RegisterGlobalFunction( signature.c_str(), asFUNCTION( *func ), asCALL_CDECL ) );
	}
	
/*
=================================================
	AddProperty
=================================================
*/
	template <typename T>
	inline void  ScriptEngine::AddProperty (INOUT T &var, StringView name)
	{
		String	signature;
		ScriptTypeInfo<T>::Name( OUT signature );
		signature << ' ' << name;

		AS_CALL( _engine->RegisterGlobalProperty( signature.c_str(), Cast<void *>(&var) ) );
	}
	
/*
=================================================
	AddConstProperty
=================================================
*/
	template <typename T>
	inline void  ScriptEngine::AddConstProperty (const T &var, StringView name)
	{
		String	signature( "const " );
		ScriptTypeInfo<T>::Name( OUT signature );
		signature << ' ' << name;

		AS_CALL( _engine->RegisterGlobalProperty( signature.c_str(), Cast<void *>(const_cast<T*>(&var)) ) );
	}
	
/*
=================================================
	CreateScript
=================================================
*/
	template <typename Fn>
	inline ScriptFnPtr<Fn>  ScriptEngine::CreateScript (StringView entry, const ScriptModulePtr &module)
	{
		String	signature;
		GlobalFunction<Fn>::GetDescriptor( OUT signature, entry );

		AngelScript::asIScriptContext*	ctx = null;
		if ( not _CreateContext( signature, module, ctx ) )
		{
			if ( ctx )
				ctx->Release();
		}

		return ScriptFnPtr<Fn>{ new ScriptFn<Fn>{ module, ctx }};
	}

}	// FGScript
