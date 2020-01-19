// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

extern void UnitTest_Ranges ();
extern void Test_SpinLock ();
extern void Test_LfIndexedPool ();
extern void Test_LfIndexedPool2 ();
extern void Test_LfStaticPool ();


int main ()
{
	UnitTest_Ranges();
	Test_SpinLock();
	Test_LfIndexedPool();
	Test_LfIndexedPool2();
	Test_LfStaticPool();
}
