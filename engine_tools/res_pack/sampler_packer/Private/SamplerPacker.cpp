// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "SamplerPacker.h"
#include "Private/SamplerDesc.h"

#include "serializing/Serializer.h"

#include "scripting/Impl/ScriptEngine.inl.h"
#include "scripting/Impl/ScriptFn.h"

#include "stl/Stream/FileStream.h"
#include "stl/Algorithms/StringUtils.h"

namespace AE::SamplerPacker
{
	using Filename = WString;

	using namespace AE::Scripting;
	
/*
=================================================
	PackSamplers
=================================================
*/
	extern "C" bool AE_SP_API PackSamplers (const SamplersInfo *info)
	{
		CHECK_ERR( info );
		CHECK_ERR( info->inSamplerCount > 0 and info->inSamplers != null );
		CHECK_ERR( info->outputPackName != null );
		
		HashSet<Filename>	sampler_files;
		ScriptEnginePtr		script_engine	= MakeShared<ScriptEngine>();
		SamplerStorage		sampler_storage;
		SamplerStorage::SetInstance( &sampler_storage );

		CHECK_ERR( script_engine->Create() );
		CHECK_ERR( SamplerDesc::Bind( script_engine ));
		
		for (uint i = 0; i < info->inSamplerCount; ++i)
		{
			Path	path{ info->inSamplers[i] };

			if ( not FileSystem::Exists( path ) or not FileSystem::IsFile( path ))
			{
				AE_LOGI( "Can't find sampler file: '"s << path.string() << "'" );
				continue;
			}

			sampler_files.insert( FileSystem::ToAbsolute( path ).wstring() );
		}

		for (auto& path : sampler_files)
		{
			FileRStream		file{ path };

			if ( not file.IsOpen() )
			{
				AE_LOGI( "Failed to open sampler file: '"s << Path{path}.string() << "'" );
				continue;
			}
			
			ScriptEngine::ModuleSource	src;
			
			if ( not file.Read( size_t(file.RemainingSize()), OUT src.script ))
			{
				AE_LOGI( "Failed to read sampler file: '"s << Path{path}.string() << "'" );
				continue;
			}

			ScriptModulePtr		module = script_engine->CreateModule({ src });
			if ( not module )
			{
				AE_LOGI( "Failed to parse pipeline file: '"s << Path{path}.string() << "'" );
				continue;
			}

			auto	fn = script_engine->CreateScript< void() >( "main", module );
			if ( not fn )
			{
				AE_LOGI( "Failed to create script context for file: '"s << Path{path}.string() << "'" );
				continue;
			}

			Unused( fn->Run() );
		}

		auto	file = MakeShared<FileWStream>( info->outputPackName );
		CHECK_ERR( file->IsOpen() );

		Serializing::Serializer	ser;
		ser.stream	= file;
		CHECK_ERR( sampler_storage.Serialize( ser ));

		SamplerStorage::SetInstance( null );
		return true;
	}

}	// AE::SamplerPacker
