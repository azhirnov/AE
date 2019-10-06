// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "script_binding/Impl/ScriptEngine.h"
#include "script_binding/Impl/ScriptTypes.h"
#include "stl/Algorithms/StringUtils.h"

namespace FGScript
{

	//
	// Script Module
	//

	class ScriptModule final : public std::enable_shared_from_this<ScriptModule>
	{
	// variables
	private:
		ScriptEnginePtr							_engine;
		Ptr< AngelScript::asIScriptModule >		_module;


	// methods
	public:
		explicit ScriptModule (const ScriptEnginePtr &eng);
		~ScriptModule ();

		bool Create (NtStringView name);
		void Destroy ();
		
		template <typename Ret, typename ...Args>
		bool Run (StringView script, StringView entry, OUT Ret &result, Args ...args);
		bool Run (StringView script, StringView entry);

	private:
		template <typename T> using CtxResult	= _fgscript_hidden_::ContextResult<T>;

		template <typename Ret, typename ...Args>
		bool _Run (StringView script, StringView entry, CtxResult<Ret> result, Args ...args);
	};
	

/*
=================================================
	Run
=================================================
*/
	template <typename Ret, typename ...Args>
	inline bool ScriptModule::Run (StringView script, StringView entry, OUT Ret &result, Args ...args)
	{
		return _Run< Ret, Args... >( script, entry, CtxResult<Ret>{result}, args... );
	}
	
/*
=================================================
	_Run
=================================================
*/
	template <typename Ret, typename ...Args>
	inline bool ScriptModule::_Run (StringView script, StringView entry, CtxResult<Ret> result, Args ...args)
	{
		using namespace AngelScript;
		
		CHECK_ERR( _module );
		CHECK_ERR( not script.empty() );
		CHECK_ERR( not entry.empty() );

		// prepare script
		String	signature;
		GlobalFunction< Ret (*) (Args...) >::GetDescriptor( OUT signature, entry );
		
		_module->AddScriptSection( "def_script", script.data(), script.length() );
		
		AS_CALL_R( _module->Build() );
		
		asIScriptContext *	ctx		= _engine->Get()->CreateContext();
		asIScriptFunction * func	= _module->GetFunctionByDecl( signature.c_str() );
		CHECK_ERR( func != null );

		AS_CALL_R( ctx->Prepare( func ));
		

		// execute
		_fgscript_hidden_::SetContextArgs< Args... >::Set( ctx, 0, args... );

		const int exec_res = ctx->Execute();


		// check result
		if ( exec_res == asEXECUTION_FINISHED )
		{
			if constexpr( not IsSameTypes< void, Ret > )
			{
				result.value = _fgscript_hidden_::ContextSetterGetter<Ret>::Get( ctx );
			}
			else
				FG_UNUSED( result );
		}
		else
		if ( exec_res == asEXECUTION_EXCEPTION )
		{
			String&	err = signature;

			err.clear();
			err	<< "Exception in function: "
				<< ctx->GetExceptionFunction()->GetName();

			const char *section = 0;
			int column = 0;
			int line = ctx->GetExceptionLineNumber( OUT &column, OUT &section );

			err << "(" << ToString( line ) << ", " << ToString( column ) << "):\n";
			err << section << "\n";
			err << ctx->GetExceptionString();

			RETURN_ERR( err );
		}
		else
		{
			RETURN_ERR( "AngelScript execution failed" );
		}

		AS_CALL( ctx->Release() );
		return true;
	}
//-----------------------------------------------------------------------------
	
	
/*
=================================================
	Run
=================================================
*/
	template <typename Ret, typename ...Args>
	inline bool ScriptEngine::Run (StringView script, StringView entry, OUT Ret &result, Args ...args)
	{
		return _defModule->Run( script, entry, OUT result, args... );
	}
	
	inline bool ScriptEngine::Run (StringView script, StringView entry)
	{
		return _defModule->Run( script, entry );
	}
	

}	// FGScript
