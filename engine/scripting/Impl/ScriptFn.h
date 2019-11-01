// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "scripting/Impl/ScriptEngine.h"
#include "scripting/Impl/ScriptTypes.h"
#include "stl/Algorithms/StringUtils.h"

namespace AE::Scripting
{
	template <typename F>
	class ScriptFn;


	//
	// Script Function
	//

	template <typename R, typename ...Args>
	class ScriptFn< R (Args...) > final : public std::enable_shared_from_this< ScriptFn<R (Args...)> >
	{
		friend class ScriptEngine;

	// types
	private:
		using Result_t = Conditional< IsSameTypes<R, void>, bool, Optional<R> >;


	// variables
	private:
		ScriptModulePtr					_module;
		AngelScript::asIScriptContext*	_ctx;


	// methods
	private:
		ScriptFn (const ScriptModulePtr &mod, AngelScript::asIScriptContext* ctx) :
			_module{ mod }, _ctx{ ctx }
		{}

	public:
		~ScriptFn ()
		{
			if ( _ctx )
				_ctx->Release();
		}

		ND_ Result_t  Run (Args ...args)
		{
			using namespace AngelScript;

			_ae_scripting_hidden_::SetContextArgs<Args...>::Set( _ctx, 0, std::forward<Args>(args)... );

			const int exec_res = _ctx->Execute();
			
			if constexpr( IsSameTypes<R, void> )
			{
				if ( exec_res == asEXECUTION_FINISHED ) {
					return true;
				}
				return _CheckError( exec_res );
			}
			else
			{
				if ( exec_res == asEXECUTION_FINISHED ) {
					return {_ae_scripting_hidden_::ContextSetterGetter<R>::Get( _ctx )};
				}
				_CheckError( exec_res );
				return {};
			}
		}

	private:
		bool _CheckError (int exec_res)
		{
			using namespace AngelScript;

			if ( exec_res == asEXECUTION_EXCEPTION )
			{
				String	err;
				err	<< "Exception in function: "
					<< _ctx->GetExceptionFunction()->GetName();

				const char *section = 0;
				int column = 0;
				int line = _ctx->GetExceptionLineNumber( OUT &column, OUT &section );

				err << "(" << ToString( line ) << ", " << ToString( column ) << "):\n";
				err << section << "\n";
				err << _ctx->GetExceptionString();

				RETURN_ERR( err );
			}
			else
			{
				RETURN_ERR( "AngelScript execution failed" );
			}
		}
	};

}	// AE::Scripting
