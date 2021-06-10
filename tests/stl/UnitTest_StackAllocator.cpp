// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Memory/StackAllocator.h"
#include "UnitTest_Common.h"


namespace
{
	static void  StackAllocator_Test1 ()
	{
		StackAllocator< UntypedAlignedAllocator, 1 >	alloc;
		alloc.SetBlockSize( 1_Kb );

		TEST( alloc.Alloc( 100_b, 4_b ) != null );

		auto	bm = alloc.Push();

		TEST( alloc.Alloc( 1_Kb - 100_b, 4_b ) != null );
		TEST( alloc.Alloc( 100_b, 4_b ) == null );

		alloc.Pop( bm );
		
		TEST( alloc.Alloc( 1_Kb - 100_b, 4_b ) != null );
		TEST( alloc.Alloc( 100_b, 4_b ) == null );
	}


	static void  StackAllocator_Test2 ()
	{
		StackAllocator< UntypedAlignedAllocator, 1 >	alloc;
		alloc.SetBlockSize( 1_Kb );

		auto	bm = alloc.Push();

		TEST( alloc.Alloc( 128_b, 4_b ) != null );
		TEST( alloc.Alloc( 1_Kb - 128_b, 4_b ) != null );
		TEST( alloc.Alloc( 100_b, 4_b ) == null );

		alloc.Pop( bm );
		
		TEST( alloc.Alloc( 512_b, 4_b ) != null );
		TEST( alloc.Alloc( 512_b, 4_b ) != null );
		TEST( alloc.Alloc( 100_b, 4_b ) == null );
	}
}


extern void UnitTest_StackAllocator ()
{
	StackAllocator_Test1();
	StackAllocator_Test2();

	AE_LOGI( "UnitTest_StackAllocator - passed" );
}
