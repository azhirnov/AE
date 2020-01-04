// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Common.h"

extern void UnitTest_Array ();
extern void UnitTest_BrotliStream ();
extern void UnitTest_Color ();
extern void UnitTest_CT_Counter ();
extern void UnitTest_FixedArray ();
extern void UnitTest_FixedMap ();
extern void UnitTest_FileSystem ();
extern void UnitTest_LinearAllocator ();
extern void UnitTest_Math ();
extern void UnitTest_Math_Matrix ();
extern void UnitTest_Math_Transformation ();
extern void UnitTest_Math_Rectangle ();
extern void UnitTest_Math_Fractional ();
extern void UnitTest_Math_PhysicalQuantity ();
extern void UnitTest_NtStringView ();
extern void UnitTest_StaticString ();
extern void UnitTest_StructView ();
extern void UnitTest_StringParser ();
extern void UnitTest_ToString ();
extern void UnitTest_TypeList ();
extern void UnitTest_TypeTraits ();
extern void UnitTest_FixedTupleArray ();

#ifdef PLATFORM_ANDROID
extern int Test_STL ()
#else
int main ()
#endif
{
	UnitTest_Array();
	UnitTest_BrotliStream();
	UnitTest_Color();
	UnitTest_CT_Counter();
	UnitTest_FixedArray();
	UnitTest_FixedMap();
	UnitTest_FileSystem();
	UnitTest_LinearAllocator();
	
	UnitTest_Math();
	UnitTest_Math_Matrix();
	UnitTest_Math_Transformation();
	UnitTest_Math_Rectangle();
	UnitTest_Math_Fractional();
	UnitTest_Math_PhysicalQuantity();

	UnitTest_NtStringView();
	UnitTest_StaticString();
	UnitTest_StructView();
	UnitTest_StringParser();
	UnitTest_ToString();
	UnitTest_TypeList();
	UnitTest_TypeTraits();
	UnitTest_FixedTupleArray();

	AE_LOGI( "Tests.STL finished" );
	return 0;
}
