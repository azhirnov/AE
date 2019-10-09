// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'
//
// see http://www.angelcode.com/angelscript/
//
// online reference http://www.angelcode.com/angelscript/sdk/docs/manual/index.html
//

#pragma once

#include "stl/Containers/Ptr.h"
#include "stl/Containers/StringView.h"
#include "stl/Containers/NtStringView.h"
#include "stl/CompileTime/TypeList.h"

// AngelScript + Addons //
#define AS_USE_NAMESPACE
#include "angelscript.h"

namespace FGScript
{
	using namespace FGC;

	using ScriptModulePtr = SharedPtr< class ScriptModule >;
	using ScriptEnginePtr = SharedPtr< class ScriptEngine >;



	//
	// Script Engine
	//

	class ScriptEngine final : public std::enable_shared_from_this<ScriptEngine>
	{
	// types
	public:
		class ScriptSharedObj : public std::enable_shared_from_this<ScriptSharedObj>
		{};

		using ScriptSharedObjPtr = SharedPtr< ScriptSharedObj >;


	// variables
	private:
		Ptr< AngelScript::asIScriptEngine >		_engine;

		ScriptModulePtr							_defModule;

		HashSet< ScriptSharedObjPtr >			_objects;


	// methods
	private:
		ScriptEngine (const ScriptEngine &) {}
		void operator = (const ScriptEngine &) {}

	public:
		ScriptEngine ();
		~ScriptEngine ();

		ND_ AngelScript::asIScriptEngine *			Get ()					{ return _engine.operator->(); }
		ND_ AngelScript::asIScriptEngine const *	Get ()	const			{ return _engine.operator->(); }

		ND_ AngelScript::asIScriptEngine *			operator -> ()			{ return _engine.operator->(); }
		ND_ AngelScript::asIScriptEngine const *	operator -> () const	{ return _engine.operator->(); }

		bool Create ();
		bool Create (AngelScript::asIScriptEngine *se);

		template <typename T>
		void AddFunction (T func, StringView name);

		//template <typename T>
		//void AddFunctionTemplate (T func, StringView name);

		template <typename T>
		void AddProperty (INOUT T &var, StringView name);
		
		template <typename T>
		void AddConstProperty (const T &var, StringView name);

		void SetNamespace (NtStringView name);
		void SetDefaultNamespace ();

		void AddSharedObject (const ScriptSharedObjPtr &obj);
		
		template <typename Ret, typename ...Args>
		bool Run (StringView script, StringView entry, OUT Ret &result, Args ...args);
		bool Run (StringView script, StringView entry);

		ND_ static bool _CheckError (int err, StringView asFunc, StringView func, StringView file, int line);


	private:
		static void _MessageCallback (const AngelScript::asSMessageInfo *msg, void *param);
	};
	

#	define AS_CALL( ... ) \
	{ \
		int __as_result = ( __VA_ARGS__ ); \
		::FGScript::ScriptEngine::_CheckError( __as_result, FG_PRIVATE_TOSTRING( __VA_ARGS__ ), FG_FUNCTION_NAME, __FILE__, __LINE__ ); \
	}
	
#	define AS_CALL_R( ... ) \
	{ \
		int __as_result = ( __VA_ARGS__ ); \
		if ( not ::FGScript::ScriptEngine::_CheckError( __as_result, FG_PRIVATE_TOSTRING( __VA_ARGS__ ), FG_FUNCTION_NAME, __FILE__, __LINE__ ) ) \
			return false; \
	}
	

}	// FGScript
