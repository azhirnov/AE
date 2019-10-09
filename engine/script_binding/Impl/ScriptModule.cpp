// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "script_binding/Impl/ScriptModule.h"
#include "stl/Stream/FileStream.h"
#include "stl/Algorithms/StringUtils.h"

namespace FGScript
{

/*
=================================================
	constructor
=================================================
*/
	ScriptModule::ScriptModule (const ScriptEnginePtr &eng) : _engine{ eng }
	{
	}
		
/*
=================================================
	destructor
=================================================
*/
	ScriptModule::~ScriptModule ()
	{
		Destroy();
	}

/*
=================================================
	Create
=================================================
*/
	bool ScriptModule::Create (NtStringView name)
	{
		using namespace AngelScript;

		CHECK_ERR( _engine and _engine->Get() );

		_module = _engine->Get()->GetModule( name.c_str(), asGM_ALWAYS_CREATE );
		CHECK_ERR( _module );

		return true;
	}
	
/*
=================================================
	Destroy
=================================================
*/
	void ScriptModule::Destroy ()
	{
		if ( _module )
		{
			_module->Discard();
			_module = null;
		}

		_engine = null;
	}
	
/*
=================================================
	Run
=================================================
*/
	bool ScriptModule::Run (StringView script, StringView entry)
	{
		return _Run<void>( script, entry, CtxResult<void>{} );
	}

}	// FGScript
