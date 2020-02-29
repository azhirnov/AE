// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "scripting/Impl/ScriptEngine.h"
#include "stl/Algorithms/StringUtils.h"

namespace AE::Scripting
{
	
/*
=================================================
	constructor
=================================================
*/
	ScriptModule::ScriptModule (AngelScript::asIScriptModule* mod) :
		_module{ mod }
	{
		ASSERT( _module );
	}
	
/*
=================================================
	destructor
=================================================
*/
	ScriptModule::~ScriptModule ()
	{
		if ( _module ) {
			_module->Discard();
		}
	}
//-----------------------------------------------------------------------------


/*
=================================================
	constructor
=================================================
*/
	ScriptEngine::ScriptEngine ()
	{
	}

/*
=================================================
	destructor
=================================================
*/
	ScriptEngine::~ScriptEngine ()
	{
		if ( _engine ) {
			_engine->ShutDownAndRelease();
		}
	}
	
/*
=================================================
	Create
=================================================
*/
	bool ScriptEngine::Create ()
	{
		CHECK_ERR( not _engine );

		using namespace AngelScript;

		_engine = asCreateScriptEngine( ANGELSCRIPT_VERSION );
		_engine->SetMessageCallback( asFUNCTION( _MessageCallback ), 0, asCALL_CDECL );

		return true;
	}
	
/*
=================================================
	Create
=================================================
*/
	bool ScriptEngine::Create (AngelScript::asIScriptEngine *se)
	{
		CHECK_ERR( not _engine );
		CHECK_ERR( se != null );

		_engine = se;
		_engine->AddRef();
		
		return true;
	}
	
/*
=================================================
	CreateModule
=================================================
*/
	ScriptModulePtr  ScriptEngine::CreateModule (ArrayView<ModuleSource> sources)
	{
		using namespace AngelScript;
		using ModulePtr = std::unique_ptr<asIScriptModule, void (*)(asIScriptModule*)>;

		const String	name = "module_" + ToString(++_moduleIndex);

		ASSERT( not _engine->GetModule( name.c_str(), asGM_ONLY_IF_EXISTS ));

		ModulePtr	module{
			_engine->GetModule( name.c_str(), asGM_ALWAYS_CREATE ),
			[](asIScriptModule* m) { m->Discard(); }
		};

		for (auto& src : sources) {
			AS_CALL_R( module->AddScriptSection( src.name.c_str(), src.script.data(), src.script.length() ));
		}
		AS_CALL_R( module->Build() );

		return ScriptModulePtr{ new ScriptModule{ module.release() }};
	}
	
/*
=================================================
	_CreateContext
=================================================
*/
	bool  ScriptEngine::_CreateContext (const String &signature, const ScriptModulePtr &module, OUT AngelScript::asIScriptContext* &ctx)
	{
		using namespace AngelScript;
		
		ctx = _engine->CreateContext();
		CHECK_ERR( ctx );

		asIScriptFunction* func = module->_module->GetFunctionByDecl( signature.c_str() );
		CHECK_ERR( func );

		AS_CALL_R( ctx->Prepare( func ));
		return true;
	}

/*
=================================================
	SetNamespace
=================================================
*/
	void ScriptEngine::SetNamespace (NtStringView name)
	{
		AS_CALL( _engine->SetDefaultNamespace( name.c_str() ));
	}
	
/*
=================================================
	SetDefaultNamespace
=================================================
*/
	void ScriptEngine::SetDefaultNamespace ()
	{
		SetNamespace( "" );
	}
	
/*
=================================================
	_MessageCallback
=================================================
*/
	void ScriptEngine::_MessageCallback (const AngelScript::asSMessageInfo *msg, void *)
	{
		using namespace AngelScript;

		String	str("AngelScript message: ");

		str << msg->section << " (" << ToString( msg->row ) << ", " << ToString( msg->col ) << ") ";

		switch ( msg->type )
		{
			case asMSGTYPE_WARNING :		str << "WARN  ";	break;
			case asMSGTYPE_INFORMATION :	str << "INFO  ";	break;
			case asMSGTYPE_ERROR :			str << "ERROR ";	break;
		}
			
		str << msg->message;

		StringView	msg_str		= msg->message;
		StringView	code_mark	= "(Code: ";

		if ( size_t pos = msg_str.find( code_mark ); pos != StringView::npos )
		{
			const size_t	begin	= pos + code_mark.length();
			StringView		code	= msg_str.substr( begin );
			
			//if ( code.Find( ')', OUT pos ) )
			//	code = code.SubString( 0, pos );

			const int	int_code = std::strtol( NtStringView{code}.c_str(), null, 0 );

			str << ", code name: ";

			switch ( int_code )
			{
				case asERetCodes::asERROR :									str << "asERROR";						break;
				case asERetCodes::asCONTEXT_ACTIVE :						str << "asCONTEXT_ACTIVE";				break;
				case asERetCodes::asCONTEXT_NOT_FINISHED :					str << "asCONTEXT_NOT_FINISHED";		break;
				case asERetCodes::asCONTEXT_NOT_PREPARED :					str << "asCONTEXT_NOT_PREPARED";		break;
				case asERetCodes::asINVALID_ARG :							str << "asINVALID_ARG";					break;
				case asERetCodes::asNO_FUNCTION :							str << "asNO_FUNCTION";					break;
				case asERetCodes::asNOT_SUPPORTED :							str << "asNOT_SUPPORTED";				break;
				case asERetCodes::asINVALID_NAME :							str << "asINVALID_NAME";				break;
				case asERetCodes::asNAME_TAKEN :							str << "asNAME_TAKEN";					break;
				case asERetCodes::asINVALID_DECLARATION :					str << "asINVALID_DECLARATION";			break;
				case asERetCodes::asINVALID_OBJECT :						str << "asINVALID_OBJECT";				break;
				case asERetCodes::asINVALID_TYPE :							str << "asINVALID_TYPE";				break;
				case asERetCodes::asALREADY_REGISTERED :					str << "asALREADY_REGISTERED";			break;
				case asERetCodes::asMULTIPLE_FUNCTIONS :					str << "asMULTIPLE_FUNCTIONS";			break;
				case asERetCodes::asNO_MODULE :								str << "asNO_MODULE";					break;
				case asERetCodes::asNO_GLOBAL_VAR :							str << "asNO_GLOBAL_VAR";				break;
				case asERetCodes::asINVALID_CONFIGURATION :					str << "asINVALID_CONFIGURATION";		break;
				case asERetCodes::asINVALID_INTERFACE :						str << "asINVALID_INTERFACE";			break;
				case asERetCodes::asCANT_BIND_ALL_FUNCTIONS :				str << "asCANT_BIND_ALL_FUNCTIONS";		break;
				case asERetCodes::asLOWER_ARRAY_DIMENSION_NOT_REGISTERED :	str << "asLOWER_ARRAY_DIMENSION_NOT_REGISTERED"; break;
				case asERetCodes::asWRONG_CONFIG_GROUP :					str << "asWRONG_CONFIG_GROUP";			break;
				case asERetCodes::asCONFIG_GROUP_IS_IN_USE :				str << "asCONFIG_GROUP_IS_IN_USE";		break;
				case asERetCodes::asILLEGAL_BEHAVIOUR_FOR_TYPE :			str << "asILLEGAL_BEHAVIOUR_FOR_TYPE";	break;
				case asERetCodes::asWRONG_CALLING_CONV :					str << "asWRONG_CALLING_CONV";			break;
				case asERetCodes::asBUILD_IN_PROGRESS :						str << "asBUILD_IN_PROGRESS";			break;
				case asERetCodes::asINIT_GLOBAL_VARS_FAILED :				str << "asINIT_GLOBAL_VARS_FAILED";		break;
				case asERetCodes::asOUT_OF_MEMORY :							str << "asOUT_OF_MEMORY";				break;
				case asERetCodes::asMODULE_IS_IN_USE :						str << "asMODULE_IS_IN_USE";			break;
				default :													str << "unknown";
			}
		}

		AE_LOGI( str );
	}
	
/*
=================================================
	_CheckError
=================================================
*/
	bool ScriptEngine::_CheckError (int err, StringView asFunc, StringView func, StringView file, int line)
	{
		using namespace AngelScript;

		if ( err >= 0 )
			return true;

		String	str("AngelScript error: ");

#		define __AS_CASE_ERR( _val_ ) \
			case _val_ : str << AE_PRIVATE_TOSTRING( _val_ ); break;

		switch ( err )
		{
			__AS_CASE_ERR( asERROR );
			__AS_CASE_ERR( asCONTEXT_ACTIVE );
			__AS_CASE_ERR( asCONTEXT_NOT_FINISHED );
			__AS_CASE_ERR( asCONTEXT_NOT_PREPARED );
			__AS_CASE_ERR( asINVALID_ARG );
			__AS_CASE_ERR( asNO_FUNCTION  );
			__AS_CASE_ERR( asNOT_SUPPORTED );
			__AS_CASE_ERR( asINVALID_NAME );
			__AS_CASE_ERR( asNAME_TAKEN );
			__AS_CASE_ERR( asINVALID_DECLARATION );
			__AS_CASE_ERR( asINVALID_OBJECT );
			__AS_CASE_ERR( asINVALID_TYPE );
			__AS_CASE_ERR( asALREADY_REGISTERED );
			__AS_CASE_ERR( asMULTIPLE_FUNCTIONS );
			__AS_CASE_ERR( asNO_MODULE );
			__AS_CASE_ERR( asNO_GLOBAL_VAR );
			__AS_CASE_ERR( asINVALID_CONFIGURATION );
			__AS_CASE_ERR( asINVALID_INTERFACE );
			__AS_CASE_ERR( asCANT_BIND_ALL_FUNCTIONS );
			__AS_CASE_ERR( asLOWER_ARRAY_DIMENSION_NOT_REGISTERED );
			__AS_CASE_ERR( asWRONG_CONFIG_GROUP );
			__AS_CASE_ERR( asCONFIG_GROUP_IS_IN_USE );
			__AS_CASE_ERR( asILLEGAL_BEHAVIOUR_FOR_TYPE );
			__AS_CASE_ERR( asWRONG_CALLING_CONV );
			__AS_CASE_ERR( asBUILD_IN_PROGRESS );
			__AS_CASE_ERR( asINIT_GLOBAL_VARS_FAILED );
			__AS_CASE_ERR( asOUT_OF_MEMORY );

			default :
				str << "code: 0x" << ToString<16>( err );
				break;
		}

#			undef __AS_CASE_ERR
			
		str << ", in " << asFunc;
		str << ", function: " << func;

		AE_LOGE( str, file, line );
		return false;
	}

}	// AE::Scripting
