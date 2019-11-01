// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#include "scripting/Bindings/CoreBindings.h"
#include "scripting/Impl/ScriptEngine.inl.h"

#include "scriptstdstring.h"
#include "scriptarray.h"

namespace AE::Scripting
{

/*
=================================================
	BindString
=================================================
*/
	void CoreBindings::BindString (const ScriptEnginePtr &se)
	{
		AngelScript::RegisterStdString( se->Get() );
	}
	
/*
=================================================
	BindArray
=================================================
*/
	void CoreBindings::BindArray (const ScriptEnginePtr &se)
	{
		AngelScript::RegisterScriptArray( se->Get(), false );
	}

/*
=================================================
	LogFunc
=================================================
*/
	struct LogFunc
	{
		static void Err (const String &msg) {
			AE_LOGE( msg );
		}

		static void Info (const String &msg) {
			AE_LOGI( msg );
		}

		static void Dbg (const String &msg) {
			AE_LOGD( msg );
		}
	};
	
/*
=================================================
	BindLog
=================================================
*/
	void CoreBindings::BindLog (const ScriptEnginePtr &se)
	{
		se->AddFunction( &LogFunc::Err,		"LogError" );
		se->AddFunction( &LogFunc::Info,	"LogInfo" );
		se->AddFunction( &LogFunc::Dbg,		"LogDebug" );
	}

}	// AE::Scripting
