// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Math/Math.h"

namespace AE::STL
{
namespace _ae_stl_hidden_
{

	template <typename RefType, size_t I, typename TL>
	struct TL_GetFirstIndex;

	template <typename RefType, size_t I>
	struct TL_GetFirstIndex< RefType, I, Tuple<> >
	{
		inline static constexpr size_t	value = UMax;
	};

	template <typename RefType, size_t I, typename Head, typename... Tail>
	struct TL_GetFirstIndex< RefType, I, Tuple<Head, Tail...> >
	{
		inline static constexpr size_t	value = Conditional< IsSameTypes<RefType, Head>,
													std::integral_constant<size_t, I>,
													TL_GetFirstIndex< RefType, I+1, Tuple<Tail...> > >::value;
	};
	

	template <typename RefType, size_t I, typename TL>
	struct TL_GetLastIndex;
	
	template <typename RefType, size_t I>
	struct TL_GetLastIndex< RefType, I, Tuple<> >
	{
		inline static constexpr size_t	value = UMax;
	};
	
	template <typename RefType, size_t I, typename Head, typename... Tail>
	struct TL_GetLastIndex< RefType, I, Tuple<Head, Tail...> >
	{
		using result = TL_GetLastIndex< RefType, I+1, Tuple<Tail...> >;

		inline static constexpr size_t	value = Conditional< result::value == UMax and IsSameTypes<RefType, Head>,
													std::integral_constant<size_t, I>,
													result >::value;
	};


	template <typename TL, typename T0, typename ...Types>
	struct TL_PopFront {
		using type		= typename TL_PopFront< TL, Types... >::result;
		using result	= typename type::template PushFront< T0 >;
	};

	template <typename TL, typename T0>
	struct TL_PopFront< TL, T0 > {
		using type		= TL;
		using result	= typename TL::template PushFront<T0>;
	};

	template <typename TL, typename T0, typename ...Types>
	struct TL_PopBack {
		using type = typename TL_PopBack< typename TL::template PushBack<T0>, Types... >::type;
	};
	
	template <typename TL, typename T0>
	struct TL_PopBack< TL, T0 > {
		using type = TL;
	};

}	// _ae_stl_hidden_


	//
	// Type List
	//

	template <typename... Types>
	struct TypeList
	{
	public:
		using							AsTuple		= Tuple< Types... >;

		template <typename T>
		inline static constexpr size_t	FirstIndex	= _ae_stl_hidden_::TL_GetFirstIndex< T, 0, AsTuple >::value;
		
		template <typename T>
		inline static constexpr size_t	LastIndex	= _ae_stl_hidden_::TL_GetLastIndex< T, 0, AsTuple >::value;
		
		template <typename T>
		inline static constexpr size_t	Index	= FirstIndex<T>;

		inline static constexpr size_t	Count	= std::tuple_size_v< AsTuple >;

		template <typename T>
		inline static constexpr bool	HasType	= (Index<T> != UMax);
		
		template <size_t I>		using	Get		= typename std::tuple_element<I, AsTuple>::type;
		template <size_t I>		using	GetT	= std::tuple_element<I, AsTuple>;

		struct Front { using			type	= Get<0>; };
		struct Back  { using			type	= Get<Count-1>; };

		struct Self	 { using			type	= TypeList< Types... >; };

		struct PopFront	{ using			type	= typename _ae_stl_hidden_::TL_PopFront< TypeList<>, Types... >::type; };
		struct PopBack	{ using			type	= typename _ae_stl_hidden_::TL_PopBack< TypeList<>, Types... >::type; };

		template <typename T>	using	PushBack = TypeList< Types..., T >;
		template <typename T>	using	PushFront = TypeList< T, Types... >;


		template <template <typename> class Tmpl>
		static constexpr auto			ForEach_Or ()	{ return (... or Tmpl<Types>::value); }
		
		template <template <typename> class Tmpl>
		static constexpr auto			ForEach_And ()	{ return (... and Tmpl<Types>::value); }
		
		template <template <typename> class Tmpl>
		static constexpr auto			ForEach_Add	()	{ return (... + Tmpl<Types>::value); }
		
		template <template <typename> class Tmpl>
		static constexpr auto			ForEach_Max	()	{ return Math::Max( Tmpl<Types>::value... ); }
		
		template <template <typename> class Tmpl>
		static constexpr auto			ForEach_Min	()	{ return Math::Min( Tmpl<Types>::value... ); }


		template <typename FN>
		static constexpr void 			Visit (FN&& fn)	{ return _Visit<0>( std::forward<FN>(fn) ); }

	private:
		template <size_t I, typename FN>
		static constexpr void _Visit (FN&& fn)
		{
			if constexpr( I < Count )
			{
				using T = Get<I>;
				fn.template operator()<T,I>();
				_Visit< I+1 >( std::forward<FN>(fn) );
			}
			Unused( fn );
		}
	};

	
	template <typename... Types>
	struct TypeList< std::tuple<Types...> > final : TypeList< Types... >
	{};
	

	template <typename... Types>
	struct TypeList< Tuple<Types...> > final : TypeList< Types... >
	{};


}	// AE::STL
