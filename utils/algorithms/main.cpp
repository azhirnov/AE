// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Common.h"

extern void Alg_RTMemoryManager ();
extern void Alg_RenderGraph ();
extern void Alg_ECSMultithreaded ();


int main()
{
	Alg_RTMemoryManager();
	Alg_RenderGraph();
	//Alg_ECSMultithreaded();

	return 0;
}
