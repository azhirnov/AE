// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Memory/LinearAllocator.h"
#include "UnitTest_Common.h"


namespace
{
	void LinearAllocator_Test1 ()
	{
		using T = DebugInstanceCounter<uint, 1>;
	
		LinearAllocator<>	allocator;
		allocator.SetBlockSize( 4_Mb );


		T::ClearStatistic();
		{
			std::vector< T, StdLinearAllocator<T> >	vec{ allocator };

			vec.resize( 100 );
			vec.push_back( T(101) );
		}
		TEST( T::CheckStatistic() );
	}
}


extern void UnitTest_LinearAllocator ()
{
	LinearAllocator_Test1();
	AE_LOGI( "UnitTest_LinearAllocator - passed" );
}
