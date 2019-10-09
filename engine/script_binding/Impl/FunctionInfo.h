// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/CompileTime/TypeList.h"

namespace FGScript
{
namespace _fgscript_hidden_
{
	template <typename T>
	struct FunctionInfo {};
		
	template <typename T>
	struct FunctionInfo< T * > {};
		
	template <typename T, typename Class>
	struct FunctionInfo< T (Class::*) >	{};
		
		
	template <typename Result, typename ...Args>
	struct FunctionInfo< Result (Args...) >
	{
		using args		= FGC::TypeList< Args... >;
		using result	= Result;
		using type		= Result (*) (Args...);
		using clazz		= void;

		static constexpr bool	is_const	= false;
		static constexpr bool	is_volatile	= false;
	};
		
	template <typename Result, typename ...Args>
	struct FunctionInfo< Result (*) (Args...) >
	{
		using args		= FGC::TypeList< Args... >;
		using result	= Result;
		using type		= Result (*) (Args...);
		using clazz		= void;
		
		static constexpr bool	is_const	= false;
		static constexpr bool	is_volatile	= false;
	};
		
	template <typename Class, typename Result, typename ...Args>
	struct FunctionInfo< Result (Class::*) (Args...) >
	{
		using args		= FGC::TypeList< Args... >;
		using result	= Result;
		using type		= Result (Class::*) (Args...);
		using clazz		= Class;
		
		static constexpr bool	is_const	= false;
		static constexpr bool	is_volatile	= false;
	};
		
	template <typename Class, typename Result, typename ...Args>
	struct FunctionInfo< Result (Class::*) (Args...) const >
	{
		using args		= FGC::TypeList< Args... >;
		using result	= Result;
		using type		= Result (Class::*) (Args...) const;
		using clazz		= Class;
		
		static constexpr bool	is_const	= true;
		static constexpr bool	is_volatile	= false;
	};
		
	template <typename Class, typename Result, typename ...Args>
	struct FunctionInfo< Result (Class::*) (Args...) volatile >
	{
		using args		= FGC::TypeList< Args... >;
		using result	= Result;
		using type		= Result (Class::*) (Args...) volatile;
		using clazz		= Class;
		
		static constexpr bool	is_const	= false;
		static constexpr bool	is_volatile	= true;
	};

	template <typename Class, typename Result, typename ...Args>
	struct FunctionInfo< Result (Class::*) (Args...) volatile const >
	{
		using args		= FGC::TypeList< Args... >;
		using result	= Result;
		using type		= Result (Class::*) (Args...) volatile const;
		using clazz		= Class;
		
		static constexpr bool	is_const	= true;
		static constexpr bool	is_volatile	= true;
	};
		
	template <typename Result, typename ...Args>
	struct FunctionInfo< std::function< Result (Args...) > >
	{
		using args		= FGC::TypeList< Args... >;
		using result	= Result;
		using type		= Result (*) (Args...);
		using clazz		= void;

		static constexpr bool	is_const	= false;
		static constexpr bool	is_volatile	= false;
	};

}	// _fgscript_hidden_
}	// FGScript
