// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include <tuple>

namespace AE::STL
{

	//
	// Tuple
	//

	template <typename ...Types>
	struct Tuple : std::tuple< Types... >
	{
	// types
		using Self		= Tuple< Types... >;
		using Base_t	= std::tuple< Types... >;


	// methods
		constexpr Tuple () = default;

		Tuple (const Self &) = default;
		Tuple (Self &&) = default;

		Self&	operator = (const Self &) = default;
		Self&	operator = (Self &&) = default;

		template <typename ...UTypes>
		Tuple (UTypes&& ...args) : Base_t{ std::forward<UTypes>(args)... } {}

		ND_ constexpr bool  operator == (const Self &rhs)	const	{ return Base_t::operator == ( rhs ); }
		ND_ constexpr bool  operator != (const Self &rhs)	const	{ return Base_t::operator != ( rhs ); }
		ND_ constexpr bool  operator >  (const Self &rhs)	const	{ return Base_t::operator >  ( rhs ); }
		ND_ constexpr bool  operator <  (const Self &rhs)	const	{ return Base_t::operator <  ( rhs ); }
		ND_ constexpr bool  operator >= (const Self &rhs)	const	{ return Base_t::operator >= ( rhs ); }
		ND_ constexpr bool  operator <= (const Self &rhs)	const	{ return Base_t::operator <= ( rhs ); }

		template <typename T>
		ND_ constexpr T&				Get ()			{ return std::get<T>( *this ); }

		template <typename T>
		ND_ constexpr T const&			Get ()	const	{ return std::get<T>( *this ); }

		template <size_t I>
		ND_ constexpr decltype(auto)	Get ()			{ return std::get<I>( *this ); }
		
		template <size_t I>
		ND_ constexpr decltype(auto)	Get ()	const	{ return std::get<I>( *this ); }

		static constexpr size_t			Size	= sizeof... (Types);
	};


/*
=================================================
	MakeTuple
=================================================
*/
	template <typename ...Types>
	ND_ forceinline constexpr auto  MakeTuple (Types&& ...args)
	{
		return std::make_tuple( std::forward<Types>(args)... );
	}

}	// AE::STL
