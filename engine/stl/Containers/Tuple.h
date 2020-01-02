// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

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

		constexpr Tuple (const Self &) = default;
		constexpr Tuple (Self &&) = default;

		constexpr Self&	operator = (const Self &) = default;
		constexpr Self&	operator = (Self &&) = default;

		template <typename ...UTypes>
		constexpr explicit Tuple (UTypes&& ...args) : Base_t{ std::forward<UTypes>(args)... } {}

		template <typename ...UTypes>
		constexpr Tuple (const Tuple<UTypes...> &other) : Base_t{ other.AsBase() } {}

		template <typename ...UTypes>
		constexpr Tuple (Tuple<UTypes...>&& other) : Base_t{ std::move(other).AsBase() } {}

		ND_ constexpr bool  operator == (const Self &rhs)	const	{ return Base_t::operator == ( rhs ); }
		ND_ constexpr bool  operator != (const Self &rhs)	const	{ return Base_t::operator != ( rhs ); }
		ND_ constexpr bool  operator >  (const Self &rhs)	const	{ return Base_t::operator >  ( rhs ); }
		ND_ constexpr bool  operator <  (const Self &rhs)	const	{ return Base_t::operator <  ( rhs ); }
		ND_ constexpr bool  operator >= (const Self &rhs)	const	{ return Base_t::operator >= ( rhs ); }
		ND_ constexpr bool  operator <= (const Self &rhs)	const	{ return Base_t::operator <= ( rhs ); }

		template <typename T>
		ND_ constexpr T&				Get () 	&		{ return std::get<T>( *this ); }

		template <typename T>
		ND_ constexpr T const&			Get ()	const&	{ return std::get<T>( *this ); }
		
		template <typename T>
		ND_ constexpr T &&				Get () 	&&		{ return std::get<T>( std::move(*this) ); }

		template <size_t I>
		ND_ constexpr decltype(auto)	Get ()	&		{ return std::get<I>( *this ); }
		
		template <size_t I>
		ND_ constexpr decltype(auto)	Get ()	const&	{ return std::get<I>( *this ); }
		
		template <size_t I>
		ND_ constexpr decltype(auto)	Get ()	&&		{ return std::get<I>( std::move(*this) ); }

		static constexpr size_t			Size	= sizeof... (Types);

		ND_ constexpr Base_t &			AsBase ()	&	{ return static_cast<Base_t &>(*this); }
		ND_ constexpr Base_t &&			AsBase ()	&&	{ return static_cast<Base_t &&>( std::move(*this) ); }

		template <typename Fn>
		constexpr decltype(auto)  Apply (Fn &&fn)
		{
			return std::apply( std::forward<Fn>(fn), static_cast<Base_t &>(*this) );
		}

		template <typename Fn>
		constexpr decltype(auto)  Apply (Fn &&fn) const
		{
			return std::apply( std::forward<Fn>(fn), static_cast<const Base_t &>(*this) );
		}
	};

	
	template <typename ...Types>
	Tuple (Types...) -> Tuple< Types... >;

	template <typename T1, typename T2>
	Tuple (std::pair<T1, T2>) -> Tuple<T1, T2>;


/*
=================================================
	MakeTuple
=================================================
*/
	template <typename ...Types>
	ND_ forceinline constexpr auto  MakeTuple (Types&& ...args)
	{
		// TODO: reference_wrapper
		return Tuple< std::decay_t<Types>... >{ std::forward<Types>(args)... };
	}

}	// AE::STL

namespace std
{
	template <typename ...Types>
	struct tuple_size< AE::STL::Tuple<Types...> > :
		public std::integral_constant< std::size_t, sizeof...(Types) >
	{};

	template< size_t I, typename ...Types >
	struct tuple_element< I, AE::STL::Tuple<Types...> >
	{
		using type = typename tuple_element< I, std::tuple<Types...> >::type;
	};

}	// std