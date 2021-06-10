// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

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

	namespace _hidden_
	{
		template <usize First, typename SeqType>
		struct MakeIntSequenceRange;

		template <usize First, usize ...I>
		struct MakeIntSequenceRange< First, std::integer_sequence<usize, I...> >
		{
			using type = std::integer_sequence< usize, (I + First)... >;
		};

	}	// _hidden_

	template <usize First, usize Count>
	using MakeIntSequence = typename STL::_hidden_::MakeIntSequenceRange< First, std::make_integer_sequence< usize, Count > >::type;


}	// AE::STL
