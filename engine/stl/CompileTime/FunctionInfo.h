// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/CompileTime/TypeList.h"

namespace AE::STL::_hidden_
{

	template <typename T>
	struct _FuncInfo {};
		
	template <typename T>
	struct _FuncInfo< T * > {};
		
	template <typename T, typename Class>
	struct _FuncInfo< T (Class::*) >	{};
		
		
	template <typename Result, typename ...Args>
	struct _FuncInfo< Result (Args...) >
	{
		using args		= AE::STL::TypeList< Args... >;
		using result	= Result;
		using type		= Result (*) (Args...);
		using clazz		= void;

		static constexpr bool	is_const	= false;
		static constexpr bool	is_volatile	= false;
	};
		
	template <typename Result, typename ...Args>
	struct _FuncInfo< Result (*) (Args...) >
	{
		using args		= AE::STL::TypeList< Args... >;
		using result	= Result;
		using type		= Result (*) (Args...);
		using clazz		= void;
		
		static constexpr bool	is_const	= false;
		static constexpr bool	is_volatile	= false;
	};
		
	template <typename Class, typename Result, typename ...Args>
	struct _FuncInfo< Result (Class::*) (Args...) >
	{
		using args		= AE::STL::TypeList< Args... >;
		using result	= Result;
		using type		= Result (Class::*) (Args...);
		using clazz		= Class;
		
		static constexpr bool	is_const	= false;
		static constexpr bool	is_volatile	= false;
	};
		
	template <typename Result, typename ...Args>
	struct _FuncInfo< Function< Result (Args...) > >
	{
		using args		= AE::STL::TypeList< Args... >;
		using result	= Result;
		using type		= Result (*) (Args...);
		using clazz		= void;

		static constexpr bool	is_const	= false;
		static constexpr bool	is_volatile	= false;
	};

	#define _DECL_FUNC_INFO( _cv_qual_ ) \
		template <typename Class, typename Result, typename ...Args> \
		struct _FuncInfo< Result (Class::*) (Args...) _cv_qual_ > \
		{ \
			using args		= AE::STL::TypeList< Args... >; \
			using result	= Result; \
			using type		= Result (Class::*) (Args...) _cv_qual_; \
			using clazz		= Class; \
			\
			static constexpr bool	is_const	= IsConst< int _cv_qual_ >; \
			static constexpr bool	is_volatile	= IsVolatile< int _cv_qual_ >; \
		};
	_DECL_FUNC_INFO( const );
	_DECL_FUNC_INFO( volatile );
	_DECL_FUNC_INFO( const volatile );
	_DECL_FUNC_INFO( & );
	_DECL_FUNC_INFO( const & );
	_DECL_FUNC_INFO( volatile & );
	_DECL_FUNC_INFO( const volatile & );
	_DECL_FUNC_INFO( && );
	_DECL_FUNC_INFO( const && );
	_DECL_FUNC_INFO( volatile && );
	_DECL_FUNC_INFO( const volatile && );
		
#ifdef AE_HAS_EXCEPTIONS
	_DECL_FUNC_INFO( noexcept );
	_DECL_FUNC_INFO( const noexcept );
	_DECL_FUNC_INFO( volatile noexcept );
	_DECL_FUNC_INFO( const volatile noexcept );
	_DECL_FUNC_INFO( & noexcept );
	_DECL_FUNC_INFO( const & noexcept );
	_DECL_FUNC_INFO( volatile & noexcept );
	_DECL_FUNC_INFO( const volatile & noexcept );
	_DECL_FUNC_INFO( && noexcept );
	_DECL_FUNC_INFO( const && noexcept );
	_DECL_FUNC_INFO( volatile && noexcept );
	_DECL_FUNC_INFO( const volatile && noexcept );
#endif
		
	#undef _DECL_FUNC_INFO

		
	template < typename T, bool L >
	struct _FuncInfo2 {
		using type = _FuncInfo<T>;
	};
	
	template < typename T >
	struct _FuncInfo2<T, true> {
		using type = _FuncInfo< decltype(&T::operator()) >;
	};

	template < typename T >
	struct _FuncInfo3 {
		using type = typename _FuncInfo2< T, IsClass<T> >::type;
	};

}	// AE::STL::_hidden_


namespace AE::STL
{
	template <typename T>
	using FunctionInfo = typename STL::_hidden_::_FuncInfo3<T>::type;

}	// AE::STL
