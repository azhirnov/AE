// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Utils/FileSystem.h"
using namespace AE::STL;

extern void Test_PipelineCompiler ();
extern void Test_SamplerPacker ();


int main ()
{
	std::atexit( [] () { CHECK_FATAL( AE_DUMP_MEMLEAKS() ); });

	const Path	curr = FileSystem::CurrentPath();

	Test_PipelineCompiler();
	FileSystem::SetCurrentPath( curr );

	Test_SamplerPacker();
	FileSystem::SetCurrentPath( curr );

	AE_LOGI( "Tests.AssetPacker finished" );
	return 0;
}
