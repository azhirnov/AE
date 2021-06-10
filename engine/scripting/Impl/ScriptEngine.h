// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'
//
// see http://www.angelcode.com/angelscript/
//
// online reference http://www.angelcode.com/angelscript/sdk/docs/manual/index.html
//

#pragma once

#include "stl/Containers/Ptr.h"
#include "stl/Containers/ArrayView.h"
#include "stl/Containers/NtStringView.h"
#include "stl/CompileTime/TypeList.h"
#include "stl/CompileTime/FunctionInfo.h"
#include "stl/Math/BitMath.h"
#include "stl/Utils/RefCounter.h"

// AngelScript + Addons //
#include "angelscript.h"

namespace AE::Scripting
{
	using namespace AE::STL;

	using ScriptModulePtr = RC< class ScriptModule >;
	using ScriptEnginePtr = RC< class ScriptEngine >;
	
	template <typename Fn>
	class ScriptFn;

	template <typename Fn>
	using ScriptFnPtr = RC< ScriptFn<Fn> >;
	


	//
	// Script Module
	//

	class ScriptModule final : public EnableRC<ScriptModule>
	{
		friend class ScriptEngine;
	// variables
	private:
		AngelScript::asIScriptModule*	_module;
		
	// methods
	private:
		explicit ScriptModule (AngelScript::asIScriptModule* mod);
	public:
		~ScriptModule ();
	};



	//
	// Script Engine
	//

	class ScriptEngine final : public EnableRC<ScriptEngine>
	{
	// types
	public:
		struct ModuleSource
		{
			String	name;
			String	script;

			ModuleSource () {}
			ModuleSource (StringView name, StringView script) : name{name}, script{script} {}
			ModuleSource (String&& name, String&& script) : name{RVRef(name)}, script{RVRef(script)} {}
		};


	// variables
	private:
		Ptr< AngelScript::asIScriptEngine >		_engine;
		usize									_moduleIndex	= 0;


	// methods
	public:
		ScriptEngine ();
		~ScriptEngine ();
		
		ScriptEngine (const ScriptEngine &) = delete;
		ScriptEngine (ScriptEngine &&) = delete;
		ScriptEngine& operator = (const ScriptEngine &) = delete;
		ScriptEngine& operator = (ScriptEngine &&) = delete;

		ND_ AngelScript::asIScriptEngine *			Get ()					{ return _engine.operator->(); }
		ND_ AngelScript::asIScriptEngine const *	Get ()	const			{ return _engine.operator->(); }

		ND_ AngelScript::asIScriptEngine *			operator -> ()			{ return _engine.operator->(); }
		ND_ AngelScript::asIScriptEngine const *	operator -> () const	{ return _engine.operator->(); }

		bool Create ();
		bool Create (AngelScript::asIScriptEngine *se);

		ND_ ScriptModulePtr  CreateModule (ArrayView<ModuleSource> src);
		
		template <typename Fn>
		ND_ ScriptFnPtr<Fn>  CreateScript (StringView entry, const ScriptModulePtr &module);

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

		static bool _CheckError (int err, StringView asFunc, StringView func, StringView file, int line);

	private:
		bool _CreateContext (const String &signature, const ScriptModulePtr &module, OUT AngelScript::asIScriptContext* &ctx);

		static void _MessageCallback (const AngelScript::asSMessageInfo *msg, void *param);
	};



	//
	// Script Engine Multithreading Scope
	//
	struct ScriptEngineMultithreadingScope
	{
		ScriptEngineMultithreadingScope ();
		~ScriptEngineMultithreadingScope ();
	};
	

	//
	// Script Thread Scope
	//
	struct ScriptThreadScope
	{
		ScriptThreadScope () {}
		~ScriptThreadScope ();
	};


#	define AS_CALL( ... ) \
	{ \
		int __as_result = ( __VA_ARGS__ ); \
		::AE::Scripting::ScriptEngine::_CheckError( __as_result, AE_PRIVATE_TOSTRING( __VA_ARGS__ ), AE_FUNCTION_NAME, __FILE__, __LINE__ ); \
	}
	
#	define AS_CALL_R( ... ) \
	{ \
		int __as_result = ( __VA_ARGS__ ); \
		if ( not ::AE::Scripting::ScriptEngine::_CheckError( __as_result, AE_PRIVATE_TOSTRING( __VA_ARGS__ ), AE_FUNCTION_NAME, __FILE__, __LINE__ )) \
			return Default; \
	}
	

}	// AE::Scripting
