// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Common.h"

namespace AE::STL
{

/*
=================================================
	CT_IntLog2
=================================================
*/
namespace _hidden_
{
	template <typename T, T X, uint Bit>
	struct _IntLog2 {
		static const int	value = int((X >> Bit) != 0) + _IntLog2<T, X, Bit-1 >::value;
	};

	template <typename T, T X>
	struct _IntLog2< T, X, 0 > {
		static const int	value = 0;
	};

}	// _hidden_

	template <auto X>
	static constexpr int	CT_IntLog2 = (X ? STL::_hidden_::_IntLog2< decltype(X), X, sizeof(X)*8-1 >::value : -1);

	
/*
=================================================
	CT_Pow
=================================================
*/
	template <auto Power, typename T>
	inline constexpr T  CT_Pow (const T &base)
	{
		STATIC_ASSERT( IsInteger<T> and IsInteger<decltype(Power)> and Power >= 0 );

		if constexpr( Power == 0 )
		{
			Unused( base );
			return 1;
		}
		else
			return CT_Pow<Power-1>( base ) * base;
	}


}	// AE::STL
