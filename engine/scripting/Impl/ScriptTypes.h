// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "scripting/Impl/ScriptEngine.h"

namespace AE::Scripting
{

	//
	// Script Type Info
	//

	template <typename T>
	struct ScriptTypeInfo
	{
		using type = T;

		static constexpr bool is_object = false;
		static constexpr bool is_ref_counted = false;

		static void Name (INOUT String &);
		static void ArgName (INOUT String &s);
		static uint SizeOf ();
	};

	template <>
	struct ScriptTypeInfo <void>
	{
		using type = void;

		static constexpr bool is_object = false;
		static constexpr bool is_ref_counted = false;

		static void Name (INOUT String &s)		{ s += "void"; }
		static void ArgName (INOUT String &s)	{ s += "void"; }
		static uint SizeOf ()					{ return 0; }
	};

#	define AE_DECL_SCRIPT_TYPE( _type_, _name_ ) \
		template <> \
		struct ScriptTypeInfo < _type_ > \
		{ \
			using type = _type_; \
			\
			static constexpr bool is_object = false; \
			static constexpr bool is_ref_counted = false; \
			\
			static void Name (INOUT String &s)		{ s += (_name_); } \
			static void ArgName (INOUT String &s)	{ s += (_name_); } \
			static uint SizeOf ()					{ return sizeof(type); } \
		}
	
#	define AE_DECL_SCRIPT_OBJ( _type_, _name_ ) \
		template <> \
		struct ScriptTypeInfo < _type_ > \
		{ \
			using type = _type_; \
			\
			static constexpr bool is_object = true; \
			static constexpr bool is_ref_counted = false; \
			\
			static void Name (INOUT String &s)		{ s += (_name_); } \
			static void ArgName (INOUT String &s)	{ s += (_name_); } \
			static uint SizeOf ()					{ return sizeof(type); } \
		}

#	define AE_DECL_SCRIPT_OBJ_RC( _type_, _name_ ) \
		template <> \
		struct ScriptTypeInfo < _type_ > \
		{ \
			using type = _type_; \
			\
			static constexpr bool is_object = true; \
			static constexpr bool is_ref_counted = false; \
			\
			static void Name (INOUT String &s)		{ s += (_name_); } \
			static void ArgName (INOUT String &s)	{ s += (_name_); } \
			static uint SizeOf ()					{ return sizeof(type); } \
		}; \
		\
		template <> \
		struct ScriptTypeInfo < _type_* > \
		{ \
			using type   = _type_ *; \
			using Base_t = ScriptTypeInfo< _type_ >; \
			\
			static constexpr bool is_object = true; \
			static constexpr bool is_ref_counted = true; \
			\
			static void Name (INOUT String &s)		{ s += _name_; s += '@'; } \
			static void ArgName (INOUT String &s)	{ s += _name_; s += '@'; } \
			static uint SizeOf ()					{ return sizeof(type); } \
		}; \
		\
		template <> \
		struct ScriptTypeInfo < AngelScriptHelper::SharedPtr< _type_ > > : \
			ScriptTypeInfo< _type_* > \
		{}; \
		\
		template <> struct ScriptTypeInfo < const _type_* > {}; \
		template <> struct ScriptTypeInfo < _type_& > {}; \
		template <> struct ScriptTypeInfo < const _type_& > {}


#	define DECL_SCRIPT_TYPE( _type_ )	AE_DECL_SCRIPT_TYPE( _type_, AE_PRIVATE_TOSTRING( _type_ ) )
	DECL_SCRIPT_TYPE( bool );
	DECL_SCRIPT_TYPE( float );
	DECL_SCRIPT_TYPE( double );
	DECL_SCRIPT_TYPE( int );
	DECL_SCRIPT_TYPE( uint );
	AE_DECL_SCRIPT_TYPE( int8_t,		"int8" );
	AE_DECL_SCRIPT_TYPE( uint8_t,		"uint8" );
	AE_DECL_SCRIPT_TYPE( int16_t,		"int16" );
	AE_DECL_SCRIPT_TYPE( uint16_t,		"uint16" );
	AE_DECL_SCRIPT_TYPE( int64_t,		"int64" );
	AE_DECL_SCRIPT_TYPE( uint64_t,		"uint64" );
	AE_DECL_SCRIPT_OBJ( std::string,	"string" );
#	undef DECL_SCRIPT_TYPE


	// only 'in' and 'out' are supported
#	define AE_DECL_SCRIPT_WRAP( _templ_, _buildName_, _buildArg_ ) \
		template <typename T> \
		struct ScriptTypeInfo < _templ_ > \
		{ \
			using type   = _templ_; \
			using Base_t = ScriptTypeInfo<T>; \
			\
			static constexpr bool is_object = false; \
			static constexpr bool is_ref_counted = false; \
			\
			static void Name (INOUT String &s)		{ _buildName_; } \
			static void ArgName (INOUT String &s)	{ _buildArg_; } \
			static uint SizeOf ()					{ return sizeof(type); } \
		}

#	define MULTILINE_ARG( ... )  __VA_ARGS__
	AE_DECL_SCRIPT_WRAP( const T,
						 MULTILINE_ARG(
							s += "const ";
							Base_t::Name( s );
						 ),
						 MULTILINE_ARG(
							s += "const ";
							Base_t::Name( s );
						 ));

	AE_DECL_SCRIPT_WRAP( const T &,
						 MULTILINE_ARG(
							s += "const ";
							Base_t::Name( s );
							s += " &";
						 ),
						 MULTILINE_ARG(
							s += "const ";
							Base_t::Name( s );
							s += " &in";
						 ));
		
	AE_DECL_SCRIPT_WRAP( T &,
						 MULTILINE_ARG(
							Base_t::Name( s );
							s += " &";
						 ),
						 MULTILINE_ARG(
							Base_t::Name( s );
							s += " &inout";
						 ));
		
	AE_DECL_SCRIPT_WRAP( const T *,
						 MULTILINE_ARG(
							s += "const ";
							Base_t::Name( s );
							s += " &";
						 ),
						 MULTILINE_ARG(
							s += "const ";
							Base_t::Name( s );
							s += " &in";
						 ));
		
	AE_DECL_SCRIPT_WRAP( T *,
						 MULTILINE_ARG(
							Base_t::Name( s );
							s += " &";
						 ),
						 MULTILINE_ARG(
							Base_t::Name( s );
							s += " &inout";
						 ));
#	undef MULTILINE_ARG




	//
	// Angel Script Helper
	//

	struct AngelScriptHelper final
	{
		struct SimpleRefCounter
		{
		private:
			int		_counter	= 1;

		private:
			SimpleRefCounter (SimpleRefCounter &&) = delete;
			SimpleRefCounter (const SimpleRefCounter &) = delete;
			void operator = (SimpleRefCounter &&) = delete;
			void operator = (const SimpleRefCounter &) = delete;

		public:
			SimpleRefCounter ()			{}
			virtual ~SimpleRefCounter () {}

			void __AddRef ()		{ ASSERT( _counter > 0 );  ++_counter; }
			void __Release ()		{ ASSERT( _counter > 0 );  if ( (--_counter) == 0 ) { delete this; }}
			int  __Counter () const	{ return _counter; }
		};


		template <typename T>
		struct SharedPtr
		{
			STATIC_ASSERT( IsBaseOf< SimpleRefCounter, T > );

		private:
			T *		_ptr = null;

		private:
			void _IncRef ()		{ if ( _ptr ) { _ptr->__AddRef(); }}
			void _DecRef ()		{ if ( _ptr ) { _ptr->__Release(); _ptr = null; }}

		public:
			SharedPtr ()												{}
			explicit SharedPtr (T *ptr, bool incRef = true) : _ptr{ptr} { if ( incRef ) { _IncRef(); }}
			SharedPtr (const SharedPtr<T> &other) : _ptr{other._ptr}	{ _IncRef(); }
			SharedPtr (SharedPtr<T> &&other) : _ptr{other._ptr}			{ other._ptr = null; }
			~SharedPtr ()												{ _DecRef(); }

			void operator = (const SharedPtr<T> &rhs)		{ _DecRef();  _ptr = rhs._ptr;  _IncRef(); }
			void operator = (SharedPtr<T> &&rhs)			{ _DecRef();  _ptr = rhs._ptr;  rhs._ptr = null; }

			ND_ bool operator == (const SharedPtr<T> &rhs)	{ return _ptr == rhs._ptr; }

			ND_ T*  operator -> ()			const			{ ASSERT( _ptr );  return _ptr; }
			ND_ T&  operator *  ()			const			{ ASSERT( _ptr );  return *_ptr; }
			ND_ T*  Get ()					const			{ return _ptr; }
			ND_ explicit operator bool ()	const			{ return _ptr; }
			ND_ int UseCount ()				const			{ return _ptr ? _ptr->__Counter() : 0; }
		};


		template <typename T>
		static T * FactoryCreate ()
		{
			return New<T>();
		}


		template <typename T>
		static void Constructor (AngelScript::asIScriptGeneric *gen)
		{
			PlacementNew<T>( gen->GetObject() );
		}

		
		template <typename T>
		static void CopyConstructor (AngelScript::asIScriptGeneric *gen)
		{
			T const*	src = static_cast< const T *>( gen->GetArgObject(0) );
			void *		dst = gen->GetObject();
			PlacementNew<T>( dst, *src );
		}
		

		template <typename T>
		static void Destructor (AngelScript::asIScriptGeneric *gen)
		{
			static_cast<T *>(gen->GetObject())->~T();
		}


		template <typename T>
		static void CopyAssign (AngelScript::asIScriptGeneric *gen)
		{
			T const*	src = static_cast< const T *>( gen->GetArgObject(0) );
			T*			dst = static_cast< T *>( gen->GetObject() );

			dst->~T();
			PlacementNew<T>( dst, *src );
		}
	};


	//
	// Script Function Descriptor
	//

	namespace _ae_scripting_hidden_
	{

		template <typename T>
		struct GlobalFunction
		{
			static void GetDescriptor (OUT String &, StringView, uint, uint);
		};

		template <typename T>
		struct MemberFunction
		{
			static void GetDescriptor (OUT String &, StringView, uint, uint);
		};
		

		struct ArgsToString_Func
		{
			String &	result;
			const uint	first;
			const uint	last;
		
			ArgsToString_Func (uint first, uint last, INOUT String &str) : 
				result(str), first(first), last(last)
			{
				ASSERT( first <= last );
			}

			template <typename T, size_t Index>
			void operator () ()
			{
				if ( Index < first or Index > last )	return;
				if ( Index > first )					result += ", ";
				ScriptTypeInfo<T>::ArgName( INOUT result );
			}
		};

		template <typename Typelist>
		struct ArgsToString
		{
			static void Get (OUT String &str, uint first, uint last)
			{
				ArgsToString_Func	func( first, last, INOUT str );
				Typelist::Visit( func );
			}
			
			static void GetArgs (OUT String &str, uint offsetFromStart, uint offsetFromEnd)
			{
				ASSERT( offsetFromEnd < Typelist::Count );

				str += '(';
				Get( INOUT str, offsetFromStart, Typelist::Count - offsetFromEnd );
				str += ')';
			}
		};


		template <typename Ret, typename ...Types>
		struct GlobalFunction < Ret (AE_CDECL *) (Types...) >
		{
			using TypeList_t = TypeList< Types... >;
				
			static void GetDescriptor (OUT String &str, StringView name, uint offsetFromStart = 0, uint offsetFromEnd = 0)
			{
				ScriptTypeInfo< Ret >::Name( OUT str );
				(str += ' ') += name;
				GetArgs( INOUT str, offsetFromStart, offsetFromEnd );
			}

			static void GetArgs (OUT String &str, uint offsetFromStart = 0, uint offsetFromEnd = 0)
			{
				ArgsToString< TypeList_t >::GetArgs( OUT str, offsetFromStart, offsetFromEnd );
			}
		};

		template <typename Ret>
		struct GlobalFunction < Ret (AE_CDECL *) () >
		{
			using TypeList_t = TypeList<>;
				
			static void GetDescriptor (OUT String &str, StringView name, uint = 0, uint = 0)
			{
				ScriptTypeInfo< Ret >::Name( OUT str );
				((str += ' ') += name) += "()";
			}

			static void GetArgs (OUT String &str, uint = 0, uint = 0)
			{
				str += "()";
			}
		};
			
		template <typename Ret, typename ...Types>
		struct GlobalFunction < Ret (Types...) > : GlobalFunction< Ret (AE_CDECL *) (Types...) >
		{};

		template <typename C, typename Ret, typename ...Types>
		struct MemberFunction < Ret (AE_THISCALL C:: *) (Types...) >
		{
			using TypeList_t = TypeList< Types... >;
				
			static void GetDescriptor (OUT String &str, StringView name, uint offsetFromStart = 0, uint offsetFromEnd = 0)
			{
				GlobalFunction< Ret (*) (Types...) >::GetDescriptor( OUT str, name, offsetFromStart, offsetFromEnd );
			}

			static void GetArgs (OUT String &str, uint offsetFromStart = 0, uint offsetFromEnd = 0)
			{
				GlobalFunction< Ret (*) (Types...) >::GetArgs( OUT str, offsetFromStart, offsetFromEnd );
			}
		};
			
		template <typename C, typename Ret>
		struct MemberFunction < Ret (AE_THISCALL C:: *) () >
		{
			using TypeList_t = TypeList<>;
				
			static void GetDescriptor (OUT String &str, StringView name, uint offsetFromStart = 0, uint offsetFromEnd = 0)
			{
				GlobalFunction< Ret (*) () >::GetDescriptor( OUT str, name, offsetFromStart, offsetFromEnd );
			}

			static void GetArgs (OUT String &str, uint offsetFromStart = 0, uint offsetFromEnd = 0)
			{
				GlobalFunction< Ret (*) () >::GetArgs( OUT str, offsetFromStart, offsetFromEnd );
			}
		};
			
		template <typename C, typename Ret, typename ...Types>
		struct MemberFunction < Ret (AE_THISCALL C:: *) (Types...) const >
		{
			using TypeList_t = TypeList< Types... >;
				
			static void GetDescriptor (OUT String &str, StringView name, uint offsetFromStart = 0, uint offsetFromEnd = 0)
			{
				GlobalFunction< Ret (*) (Types...) >::GetDescriptor( OUT str, name, offsetFromStart, offsetFromEnd );
				str += " const";
			}

			static void GetArgs (OUT String &str, uint offsetFromStart = 0, uint offsetFromEnd = 0)
			{
				GlobalFunction< Ret (*) (Types...) >::GetArgs( OUT str, offsetFromStart, offsetFromEnd );
			}
		};
			
		template <typename C, typename Ret>
		struct MemberFunction < Ret (AE_THISCALL C:: *) () const >
		{
			using TypeList_t = TypeList<>;
				
			static void GetDescriptor (OUT String &str, StringView name, uint offsetFromStart = 0, uint offsetFromEnd = 0)
			{
				GlobalFunction< Ret (*) () >::GetDescriptor( OUT str, name, offsetFromStart, offsetFromEnd );
				str += " const";
			}

			static void GetArgs (OUT String &str, uint offsetFromStart = 0, uint offsetFromEnd = 0)
			{
				GlobalFunction< Ret (*) () >::GetArgs( OUT str, offsetFromStart, offsetFromEnd );
			}
		};

	}	// _ae_scripting_hidden_

	

	//
	// Global Function
	//

	template <typename T>
	struct GlobalFunction : _ae_scripting_hidden_::GlobalFunction<T>
	{
	};


	//
	// Member Function
	//

	template <typename T>
	struct MemberFunction : _ae_scripting_hidden_::MemberFunction<T>
	{
	};


	namespace _ae_scripting_hidden_
	{

		//
		// Context Setter Getter
		//
		template <typename T, bool IsObject>
		struct ContextSetterGetter_Var
		{
			static T	Get (AngelScript::asIScriptContext *ctx)					{
				T* obj = static_cast<T *>(ctx->GetReturnObject());
				return *obj;
			}
			static int	Set (AngelScript::asIScriptContext *ctx, int arg, T& value)	{ return ctx->SetArgObject( arg, reinterpret_cast<void *>(&value) ); }
		};

		template <typename T>
		struct ContextSetterGetter_Var < AngelScriptHelper::SharedPtr<T>, true >
		{
			STATIC_ASSERT( ScriptTypeInfo<T*>::is_ref_counted );

			static AngelScriptHelper::SharedPtr<T>  Get (AngelScript::asIScriptContext *ctx) {
				return AngelScriptHelper::SharedPtr<T>{ static_cast<T *>(ctx->GetReturnObject()) };
			}

			static int  Set (AngelScript::asIScriptContext *ctx, int arg, const AngelScriptHelper::SharedPtr<T> &ptr) {
				return ctx->SetArgObject( arg, reinterpret_cast<void *>(ptr.Get()) );
			}
		};

#		define DECL_CONTEXT_RESULT( _type_, _get_, _set_ ) \
			template <> \
			struct ContextSetterGetter_Var < _type_, false > \
			{ \
				static _type_ Get (AngelScript::asIScriptContext *ctx)									{ return _type_(ctx->_get_()); } \
				static int    Set (AngelScript::asIScriptContext *ctx, int arg, const _type_ &value)	{ return ctx->_set_( arg, value ); } \
			}

		DECL_CONTEXT_RESULT( int8_t,	GetReturnByte,		SetArgByte );
		DECL_CONTEXT_RESULT( uint8_t,	GetReturnByte,		SetArgByte );
		DECL_CONTEXT_RESULT( int16_t,	GetReturnWord,		SetArgWord );
		DECL_CONTEXT_RESULT( uint16_t,	GetReturnWord,		SetArgWord );
		DECL_CONTEXT_RESULT( int32_t,	GetReturnDWord,		SetArgDWord );
		DECL_CONTEXT_RESULT( uint32_t,	GetReturnDWord,		SetArgDWord );
		DECL_CONTEXT_RESULT( int64_t,	GetReturnQWord,		SetArgQWord );
		DECL_CONTEXT_RESULT( uint64_t,	GetReturnQWord,		SetArgQWord );
		DECL_CONTEXT_RESULT( float,		GetReturnFloat,		SetArgFloat );
		DECL_CONTEXT_RESULT( double,	GetReturnDouble,	SetArgDouble );
#		undef DECL_CONTEXT_RESULT
	
		template <typename T, bool IsObject>
		struct _ContextSetterGetter_Ptr
		{
			static T *  Get (AngelScript::asIScriptContext *ctx)					{ return static_cast<T *>(ctx->GetReturnAddress()); }
			static int  Set (AngelScript::asIScriptContext *ctx, int arg, T* ptr)	{ return ctx->SetArgAddress( arg, (void *)(ptr) ); }
		};

		template <typename T>
		struct _ContextSetterGetter_Ptr < T, true >
		{
			static T *  Get (AngelScript::asIScriptContext *ctx) {
				T* result = static_cast<T *>(ctx->GetReturnObject());
				if ( result ) result->__AddRef();
				return result;
			}

			static int  Set (AngelScript::asIScriptContext *ctx, int arg, T* ptr)	{ return ctx->SetArgObject( arg, reinterpret_cast<void *>(ptr) ); }
		};
		
		template <typename T>
		struct ContextSetterGetter :
			ContextSetterGetter_Var< T, ScriptTypeInfo<T>::is_object >
		{};
		
		template <>
		struct ContextSetterGetter <void>
		{
			static void	Get (AngelScript::asIScriptContext *)		{}
			static int	Set (AngelScript::asIScriptContext *, int)	{ return 0; }
		};

		template <typename T>
		struct ContextSetterGetter < T * > :
			_ContextSetterGetter_Ptr< T, ScriptTypeInfo<T*>::is_ref_counted >
		{};


		//
		// Set Context Args
		//
		template <typename ...Args>
		struct SetContextArgs;
			
		template <typename Arg0, typename ...Args>
		struct SetContextArgs< Arg0, Args...>
		{
			static void Set (AngelScript::asIScriptContext *ctx, int index, Arg0 arg0, Args ...args)
			{
				AS_CALL( ContextSetterGetter<Arg0>::Set( ctx, index, arg0 ) );
				SetContextArgs<Args...>::Set( ctx, index+1, std::forward<Args>(args)... );
			}
		};

		template <typename Arg0>
		struct SetContextArgs< Arg0 >
		{
			static void Set (AngelScript::asIScriptContext *ctx, int index, Arg0 arg0)
			{
				AS_CALL( ContextSetterGetter<Arg0>::Set( ctx, index, arg0 ) );
			}
		};

		template <>
		struct SetContextArgs<>
		{
			static void Set (AngelScript::asIScriptContext *, int)
			{}
		};


		//
		// Context Result
		//
		template <typename T>
		struct ContextResult
		{
			using type	= T;

			T&	value;

			explicit ContextResult (T &value) : value{value} {}
		};

		template <>
		struct ContextResult <void>
		{
			using type	= void;
		};

	}	// _ae_scripting_hidden_

}	// AE::Scripting
