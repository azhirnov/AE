// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Platforms/PlatformUtils.h"
#include "stl/Stream/FileStream.h"

#include "serializing/Deserializer.h"
#include "serializing/ObjectFactory.h"

#include "sampler_packer/SamplerPacker.h"
#include "sampler_packer/Private/SamplerDesc.h"

using namespace AE::STL;
using namespace AE::SamplerPacker;

#define TEST	CHECK_FATAL

namespace
{
	decltype(&PackSamplers)		pack_samplers = null;


	static void  SamplerPacker_Test1 ()
	{
		const wchar_t*	samplers_dir[]		= { L"default.samp" };
		Path			output				= L"_output";
		
		FileSystem::RemoveAll( output );
		TEST( FileSystem::CreateDirectories( output ));
		
		output.append("samplers.bin");

		SamplersInfo	info;
		info.inSamplers		= samplers_dir;
		info.inSamplerCount	= uint(CountOf( samplers_dir ));
		info.outputPackName	= output.c_str();

		TEST( pack_samplers( &info ));
		

		auto	file = MakeShared<FileRStream>( output );
		TEST( file->IsOpen() );

		AE::Serializing::Deserializer	des;
		des.stream = file;

		uint	version = 0;
		TEST( des( OUT version ) and version == SamplerStorage_Version );

		Array<Pair< SamplerName, uint >>	samp_names;
		TEST( des( OUT samp_names ));

		Array<SamplerDesc>	samplers;
		TEST( des( OUT samplers ));
		
		TEST( samp_names.size() == 7 );
		TEST( samplers.size() == 7 );

		TEST( samp_names[0].first == SamplerName{"NearestClamp"} );
		TEST( samp_names[1].first == SamplerName{"LinearMipmapRepeat"} );
		TEST( samp_names[2].first == SamplerName{"LinearMipmapClamp"} );
		TEST( samp_names[3].first == SamplerName{"LinearClamp"} );
		TEST( samp_names[4].first == SamplerName{"NearestRepeat"} );
		TEST( samp_names[5].first == SamplerName{"AnisotrophyRepeat"} );
		TEST( samp_names[6].first == SamplerName{"LinearRepeat"} );

		TEST( samplers[ samp_names[0].second ].addressMode[0] == EAddressMode::ClampToEdge );
		TEST( samplers[ samp_names[0].second ].addressMode[1] == EAddressMode::ClampToEdge );
		TEST( samplers[ samp_names[0].second ].addressMode[2] == EAddressMode::ClampToEdge );
		TEST( samplers[ samp_names[0].second ].magFilter == EFilter::Nearest );
		TEST( samplers[ samp_names[0].second ].minFilter == EFilter::Nearest );
		TEST( samplers[ samp_names[0].second ].mipmapMode == EMipmapFilter::Nearest );
		TEST( samplers[ samp_names[0].second ].maxAnisotropy.has_value() == false );
		
		TEST( samplers[ samp_names[1].second ].addressMode[0] == EAddressMode::Repeat );
		TEST( samplers[ samp_names[1].second ].addressMode[1] == EAddressMode::Repeat );
		TEST( samplers[ samp_names[1].second ].addressMode[2] == EAddressMode::Repeat );
		TEST( samplers[ samp_names[1].second ].magFilter == EFilter::Linear );
		TEST( samplers[ samp_names[1].second ].minFilter == EFilter::Linear );
		TEST( samplers[ samp_names[1].second ].mipmapMode == EMipmapFilter::Linear );
		TEST( samplers[ samp_names[1].second ].maxAnisotropy.has_value() == false );
		
		TEST( samplers[ samp_names[2].second ].addressMode[0] == EAddressMode::ClampToEdge );
		TEST( samplers[ samp_names[2].second ].addressMode[1] == EAddressMode::ClampToEdge );
		TEST( samplers[ samp_names[2].second ].addressMode[2] == EAddressMode::ClampToEdge );
		TEST( samplers[ samp_names[2].second ].magFilter == EFilter::Linear );
		TEST( samplers[ samp_names[2].second ].minFilter == EFilter::Linear );
		TEST( samplers[ samp_names[2].second ].mipmapMode == EMipmapFilter::Linear );
		TEST( samplers[ samp_names[2].second ].maxAnisotropy.has_value() == false );
		
		TEST( samplers[ samp_names[3].second ].addressMode[0] == EAddressMode::ClampToEdge );
		TEST( samplers[ samp_names[3].second ].addressMode[1] == EAddressMode::ClampToEdge );
		TEST( samplers[ samp_names[3].second ].addressMode[2] == EAddressMode::ClampToEdge );
		TEST( samplers[ samp_names[3].second ].magFilter == EFilter::Linear );
		TEST( samplers[ samp_names[3].second ].minFilter == EFilter::Linear );
		TEST( samplers[ samp_names[3].second ].mipmapMode == EMipmapFilter::Nearest );
		TEST( samplers[ samp_names[3].second ].maxAnisotropy.has_value() == false );
		
		TEST( samplers[ samp_names[4].second ].addressMode[0] == EAddressMode::Repeat );
		TEST( samplers[ samp_names[4].second ].addressMode[1] == EAddressMode::Repeat );
		TEST( samplers[ samp_names[4].second ].addressMode[2] == EAddressMode::Repeat );
		TEST( samplers[ samp_names[4].second ].magFilter == EFilter::Nearest );
		TEST( samplers[ samp_names[4].second ].minFilter == EFilter::Nearest );
		TEST( samplers[ samp_names[4].second ].mipmapMode == EMipmapFilter::Nearest );
		TEST( samplers[ samp_names[4].second ].maxAnisotropy.has_value() == false );
		
		TEST( samplers[ samp_names[5].second ].addressMode[0] == EAddressMode::Repeat );
		TEST( samplers[ samp_names[5].second ].addressMode[1] == EAddressMode::Repeat );
		TEST( samplers[ samp_names[5].second ].addressMode[2] == EAddressMode::Repeat );
		TEST( samplers[ samp_names[5].second ].magFilter == EFilter::Linear );
		TEST( samplers[ samp_names[5].second ].minFilter == EFilter::Linear );
		TEST( samplers[ samp_names[5].second ].mipmapMode == EMipmapFilter::Linear );
		TEST( samplers[ samp_names[5].second ].maxAnisotropy.has_value() );
		TEST( samplers[ samp_names[5].second ].maxAnisotropy.value() == 50.0f );
		
		TEST( samplers[ samp_names[6].second ].addressMode[0] == EAddressMode::Repeat );
		TEST( samplers[ samp_names[6].second ].addressMode[1] == EAddressMode::Repeat );
		TEST( samplers[ samp_names[6].second ].addressMode[2] == EAddressMode::Repeat );
		TEST( samplers[ samp_names[6].second ].magFilter == EFilter::Linear );
		TEST( samplers[ samp_names[6].second ].minFilter == EFilter::Linear );
		TEST( samplers[ samp_names[6].second ].mipmapMode == EMipmapFilter::Nearest );
		TEST( samplers[ samp_names[6].second ].maxAnisotropy.has_value() == false );
	}
}


extern void Test_SamplerPacker ()
{
	{
		Path	dll_path;
		TEST( FileSystem::Search( "SamplerPacker.dll", 3, 3, OUT dll_path ));

		Library		lib;
		TEST( lib.Load( dll_path ));
		TEST( lib.GetProcAddr( "PackSamplers", OUT pack_samplers ));
		
		TEST( FileSystem::FindAndSetCurrent( "sampler_test", 5 ));

		SamplerPacker_Test1();
	}
	AE_LOGI( "Test_SamplerPacker - passed" );
}

#include "sampler_packer/Private/SamplerDescDeserializer.cpp"
