// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

namespace AE::STL
{

	//
	// Value to Type
	//

	template <auto Value>
	struct ValueToType
	{
		static constexpr auto	value = Value;
	};


	
	//
	// Deferred Template Type
	//

	template <template <typename ...> class Templ, typename ...Types>
	struct DeferredTemplate
	{
		using type	= Templ< Types... >;
	};



	//
	// Make Integer Sequence
	//

namespace _ae_stl_hidden_
{
	template <size_t First, typename SeqType>
	struct MakeIntSequenceRange;

	template <size_t First, size_t ...I>
	struct MakeIntSequenceRange< First, std::integer_sequence<size_t, I...> >
	{
		using type = std::integer_sequence< size_t, (I + First)... >;
	};

}	// _ae_stl_hidden_

	template <size_t First, size_t Count>
	using MakeIntSequence = typename _ae_stl_hidden_::MakeIntSequenceRange< First, std::make_integer_sequence< size_t, Count > >::type;


	
	//
	// Unreference
	//
	
namespace _ae_stl_hidden_
{
	template <typename T>	struct Unreference								{ using type = T; };
	template <typename T>	struct Unreference< std::reference_wrapper<T> >	{ using type = T&; };
}
	template <typename T>	using Unreference = typename _ae_stl_hidden_::Unreference<T>::type;


}	// AE::STL
