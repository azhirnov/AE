// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "UnitTest_Common.h"


extern void UnitTest_TypeTraits ()
{
	{
		using T1 = ArrayView<int>;
		STATIC_ASSERT( IsSpecializationOf< T1, ArrayView > );
		STATIC_ASSERT( not IsSpecializationOf< T1, std::tuple > );

		using T2 = ArrayView<int> *;
		STATIC_ASSERT( not IsSpecializationOf< T2, ArrayView > );
		STATIC_ASSERT( not IsSpecializationOf< T2, std::tuple > );

		using T3 = std::tuple< int >;
		STATIC_ASSERT( IsSpecializationOf< T3, std::tuple > );
		STATIC_ASSERT( not IsSpecializationOf< T3, ArrayView > );
	}
}
