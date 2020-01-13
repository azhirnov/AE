// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "PipelineCompiler.h"
#include "Private/ScriptObjects.h"

#include "scripting/Bindings/CoreBindings.h"
#include "scripting/Impl/ScriptFn.h"
#include "scripting/Impl/ClassBinder.h"
#include "scripting/Impl/EnumBinder.h"

#include "serializing/Serializer.h"

#include "stl/Stream/FileStream.h"
#include "stl/CompileTime/FunctionInfo.h"


namespace AE::Scripting
{
	AE_DECL_SCRIPT_TYPE(   AE::PipelineCompiler::EShaderVersion,				"Version"			);
	AE_DECL_SCRIPT_TYPE(   AE::PipelineCompiler::EFragOutput,					"EFragOutput"		);
	AE_DECL_SCRIPT_OBJ_RC( AE::PipelineCompiler::GraphicsPipelineScriptBinding,	"GraphicsPipeline"	);
	AE_DECL_SCRIPT_OBJ_RC( AE::PipelineCompiler::MeshPipelineScriptBinding,		"MeshPipeline"		);
	AE_DECL_SCRIPT_OBJ_RC( AE::PipelineCompiler::ComputePipelineScriptBinding,	"ComputePipeline"	);
	AE_DECL_SCRIPT_OBJ_RC( AE::PipelineCompiler::RenderPassScriptBinding,		"RenderPass"		);

}	// AE::Scripting


namespace AE::PipelineCompiler
{
namespace 
{
/*
=================================================
	BindPipelineClasses
=================================================
*/
	bool BindPipelineClasses (const Scripting::ScriptEnginePtr &se)
	{
		CHECK_ERR( se->Create() );

		CoreBindings::BindScalarMath( se );
		CoreBindings::BindString( se );

		// bind shader version enums
		{
			EnumBinder<EShaderVersion>	binder{ se };
			CHECK_ERR( binder.Create() );
			CHECK_ERR( binder.AddValue( "Spirv_120", EShaderVersion::Spirv_120 ));
			CHECK_ERR( binder.AddValue( "Spirv_130", EShaderVersion::Spirv_130 ));
			CHECK_ERR( binder.AddValue( "Spirv_140", EShaderVersion::Spirv_140 ));
		}
		
		// bind fragment output enums
		{
			EnumBinder<EFragOutput>	binder{ se };
			CHECK_ERR( binder.Create() );
			CHECK_ERR( binder.AddValue( "Int4",		EFragOutput::Int4 ));
			CHECK_ERR( binder.AddValue( "UInt4",	EFragOutput::UInt4 ));
			CHECK_ERR( binder.AddValue( "Float4",	EFragOutput::Float4 ));
		}

		// bind graphics pipeline
		{
			ClassBinder<GraphicsPipelineScriptBinding>	binder{ se };
			CHECK_ERR( binder.CreateRef() );
			CHECK_ERR( binder.AddMethod( &GraphicsPipelineScriptBinding::SetName,				"SetName"				));
			CHECK_ERR( binder.AddMethod( &GraphicsPipelineScriptBinding::SetRenderPass,			"SetRenderPass"			));
			CHECK_ERR( binder.AddMethod( &GraphicsPipelineScriptBinding::Define,				"Define"				));
			CHECK_ERR( binder.AddMethod( &GraphicsPipelineScriptBinding::SetVertexShader,		"SetVertexShader"		));
			CHECK_ERR( binder.AddMethod( &GraphicsPipelineScriptBinding::SetTessControlShader,	"SetTessControlShader"	));
			CHECK_ERR( binder.AddMethod( &GraphicsPipelineScriptBinding::SetTessEvalShader,		"SetTessEvalShader"		));
			CHECK_ERR( binder.AddMethod( &GraphicsPipelineScriptBinding::SetGeometryShader,		"SetGeometryShader"		));
			CHECK_ERR( binder.AddMethod( &GraphicsPipelineScriptBinding::SetFragmentShader,		"SetFragmentShader"		));
		}

		// bind mesh pipeline
		{
			ClassBinder<MeshPipelineScriptBinding>	binder{ se };
			CHECK_ERR( binder.CreateRef() );
			CHECK_ERR( binder.AddMethod( &MeshPipelineScriptBinding::SetName,			"SetName"			));
			CHECK_ERR( binder.AddMethod( &MeshPipelineScriptBinding::SetRenderPass,		"SetRenderPass"		));
			CHECK_ERR( binder.AddMethod( &MeshPipelineScriptBinding::Define,			"Define"			));
			CHECK_ERR( binder.AddMethod( &MeshPipelineScriptBinding::SetTaskShader,		"SetTaskShader"		));
			CHECK_ERR( binder.AddMethod( &MeshPipelineScriptBinding::SetMeshShader,		"SetMeshShader"		));
			CHECK_ERR( binder.AddMethod( &MeshPipelineScriptBinding::SetFragmentShader,	"SetFragmentShader"	));
		}

		// bind compute pipeline
		{
			ClassBinder<ComputePipelineScriptBinding>	binder{ se };
			CHECK_ERR( binder.CreateRef() );
			CHECK_ERR( binder.AddMethod( &ComputePipelineScriptBinding::SetName,	"SetName"	));
			CHECK_ERR( binder.AddMethod( &ComputePipelineScriptBinding::Define,		"Define"	));
			CHECK_ERR( binder.AddMethod( &ComputePipelineScriptBinding::SetShader,	"SetShader" ));
		}

		// bind ray tracing pipeline
		{
		}

		// bind render pass
		{
			ClassBinder<RenderPassScriptBinding>	binder{ se };
			CHECK_ERR( binder.CreateRef() );
			CHECK_ERR( binder.AddMethod( &RenderPassScriptBinding::SetName,		"SetName"	));
			CHECK_ERR( binder.AddMethod( &RenderPassScriptBinding::SetOutput,	"SetOutput"	));
			CHECK_ERR( binder.AddMethod( &RenderPassScriptBinding::SetSource,	"SetSource"	));
		}

		return true;
	}
	
/*
=================================================
	BuildPipelineList
=================================================
*/
	bool BuildPipelineList (const PipelinesInfo *info, OUT HashSet<Filename> &outPipelines, OUT Array<Filename> &outIncludeDirs, OUT Array<Filename> &outShaderDirs)
	{
		for (uint i = 0; i < info->pipelineFolderCount; ++i)
		{
			Path	path{ info->pipelineFolders[i] };

			if ( not FileSystem::Exists( path ) or not FileSystem::IsDirectory( path ))
			{
				AE_LOGI( "Can't find folder: '"s << path.string() << "'" );
				continue;
			}

			for (auto& file : FileSystem::Enum( path ))
			{
				if ( file.is_directory() )
					continue;

				outPipelines.insert( FileSystem::ToAbsolute( file.path() ).wstring() );
			}
		}

		for (uint i = 0; i < info->inPipelineCount; ++i)
		{
			Path	path{ info->inPipelines[i] };

			if ( not FileSystem::Exists( path ) or FileSystem::IsDirectory( path ))
			{
				AE_LOGI( "Can't find pipeline: '"s << path.string() << "'" );
				continue;
			}

			outPipelines.insert( FileSystem::ToAbsolute( path ).wstring() );
		}

		for (uint i = 0; i < info->includeDirCount; ++i)
		{
			Path	path{ info->includeDirs[i] };

			if ( not FileSystem::Exists( path ) or not FileSystem::IsDirectory( path ))
			{
				AE_LOGI( "Can't find include folder: '"s << path.string() << "'" );
				continue;
			}

			outIncludeDirs.push_back( FileSystem::ToAbsolute( path ).wstring() );
		}

		for (uint i = 0; i < info->shaderFolderCount; ++i)
		{
			Path	path{ info->shaderFolders[i] };

			if ( not FileSystem::Exists( path ) or not FileSystem::IsDirectory( path ))
			{
				AE_LOGI( "Can't find shader folder: '"s << path.string() << "'" );
				continue;
			}

			outShaderDirs.push_back( FileSystem::ToAbsolute( path ).wstring() );
		}

		return true;
	}

/*
=================================================
	LoadPipelines
=================================================
*/
	bool LoadPipelines (const ScriptEnginePtr &scriptEngine, const HashSet<Filename> &pipelines)
	{
		CHECK_ERR( BindPipelineClasses( scriptEngine ));

		for (auto& path : pipelines)
		{
			FileRStream		file{ path };

			if ( not file.IsOpen() )
			{
				AE_LOGI( "Failed to open pipeline file: '"s << Path{path}.string() << "'" );
				continue;
			}
			
			ScriptEngine::ModuleSource	src;
			src.name = Path{path}.filename().replace_extension("").string();

			if ( not file.Read( size_t(file.RemainingSize()), OUT src.script ))
			{
				AE_LOGI( "Failed to read pipeline file: '"s << Path{path}.string() << "'" );
				continue;
			}

			ScriptModulePtr		module = scriptEngine->CreateModule({ src });
			if ( not module )
			{
				AE_LOGI( "Failed to parse pipeline file: '"s << Path{path}.string() << "'" );
				continue;
			}

			auto	fn = scriptEngine->CreateScript< void() >( "main", module );
			if ( not fn )
			{
				AE_LOGI( "Failed to create script context for file: '"s << Path{path}.string() << "'" );
				continue;
			}

			ShaderStorage::Instance()->pipelineFilename = path;

			Unused( fn->Run() );
			
			ShaderStorage::Instance()->pipelineFilename.clear();
		}

		return true;
	}
	
/*
=================================================
	ToSpirvVersion
=================================================
*/
	ND_ uint  ToSpirvVersion (EShaderVersion ver)
	{
		CHECK_ERR( (uint(ver) & uint(EShaderVersion::_Mask)) == uint(EShaderVersion::_SPIRV), 0 );

		return uint(ver) & ~uint(EShaderVersion::_Mask);
	}
	
/*
=================================================
	BuildShaderSource
=================================================
*/
	void BuildShaderSource (const ShaderInfo &info, INOUT String &source)
	{
		String	header =
			"#version 460 core\n"
			"#extension GL_ARB_separate_shader_objects : require\n"
			"#extension GL_ARB_shading_language_420pack : require\n"
			"#extension GL_GOOGLE_include_directive : require\n"
			"#extension GL_EXT_control_flow_attributes : require\n";

		BEGIN_ENUM_CHECKS();
		switch ( info.type )
		{
			// graphics
			case EShader::Vertex :
			case EShader::TessControl :
			case EShader::TessEvaluation :
			case EShader::Geometry :
			case EShader::Fragment :
				break;

			// compute
			case EShader::Compute :
				break;

			// mesh
			case EShader::MeshTask :
			case EShader::Mesh :
				header << "#extension GL_NV_mesh_shader : require\n";
				break;
		
			// ray tracing
			case EShader::RayGen :
			case EShader::RayAnyHit :
			case EShader::RayClosestHit :
			case EShader::RayMiss :
			case EShader::RayIntersection :
			case EShader::RayCallable :
				header << "#extension GL_NV_ray_tracing : require\n";
				break;

			// other
			case EShader::_Count :
			case EShader::Unknown :
			default :
				break;
		}
		END_ENUM_CHECKS();

		header << '\n';

		for (auto& def : info.defines) {
			header << def << '\n';
		}
		header << '\n';

		source.insert( 0, header.c_str() );
	}

/*
=================================================
	CompileShaders
=================================================
*/
	bool CompileShaders (const Array<Filename> &includeDirs)
	{
		CHECK_ERR( ShaderStorage::Instance()->CacheShaders() );

		// build shader reflection
		SpirvCompiler	spv_comp{ includeDirs };
		String			log;

		for (auto& sh : ShaderStorage::Instance()->uniqueShaders)
		{
			FileRStream		file{ sh.first.filename };

			if ( not file.IsOpen() )
			{
				AE_LOGI( "Failed to open shader file: '"s << Path{ sh.first.filename }.string() << "'" );
				continue;
			}

			if ( not file.Read( size_t(file.RemainingSize()), OUT sh.second.source ))
			{
				AE_LOGI( "Failed to read shader file: '"s << Path{ sh.first.filename }.string() << "'" );
				continue;
			}

			BuildShaderSource( sh.first, INOUT sh.second.source );

			if ( not spv_comp.BuildReflection( sh.first.type, ToSpirvVersion( sh.first.version ), "main", sh.second.source, OUT sh.second.reflection, OUT log ))
			{
				sh.second.compiled = false;
				AE_LOGI( "Failed to parse shader '"s << Path{ sh.first.filename }.string() << "':\n" << log );
				continue;
			}

			sh.second.compiled = true;
		}

		// compile shaders
		Array<uint>		spirv;
		for (auto& sh : ShaderStorage::Instance()->uniqueShaders)
		{
			if ( not spv_comp.Compile( sh.first.type, ToSpirvVersion( sh.first.version ), "main", sh.second.source, OUT spirv, OUT log ))
			{
				sh.second.compiled = false;
				AE_LOGI( "Failed to compile shader '"s << Path{ sh.first.filename }.string() << "':\n" << log );
				continue;
			}

			sh.second.uid = ShaderStorage::Instance()->storage->AddShader( std::move(spirv), sh.second.reflection.layout.specConstants );
		}

		// merge descriptor set layouts
		CHECK_ERR( ShaderStorage::Instance()->MergePipelines() );

		CHECK_ERR( ShaderStorage::Instance()->BuildPipelines() );

		return true;
	}
	
/*
=================================================
	SavePack
=================================================
*/
	bool SavePack (NtWStringView filename)
	{
		auto	file = MakeShared<FileWStream>( filename );
		CHECK_ERR( file->IsOpen() );

		Serializing::Serializer		ser;
		ser.stream = file;

		CHECK_ERR( ShaderStorage::Instance()->storage->Serialize( ser ));
		return true;
	}
}

/*
=================================================
	CompilePipelines
=================================================
*/
	extern "C" bool AE_PC_API  CompilePipelines (const PipelinesInfo *info)
	{
		CHECK_ERR( info );
		CHECK_ERR( (info->includeDirCount > 0) == (info->includeDirs != null) );
		CHECK_ERR( (info->inPipelineCount > 0) == (info->inPipelines != null) );
		CHECK_ERR( (info->pipelineFolderCount > 0) == (info->pipelineFolders != null) );
		CHECK_ERR( (info->shaderFolderCount > 0) == (info->shaderFolders != null) );
		CHECK_ERR( info->outputPackName );


		HashSet< Filename >		pipelines;
		Array< Filename >		include_dirs;
		Array< Filename >		shader_dirs;
		CHECK_ERR( BuildPipelineList( info, OUT pipelines, OUT include_dirs, OUT shader_dirs ));


		PipelineStorage		ppln_storage;
		ShaderStorage		shader_storage;
		ScriptEnginePtr		script_engine	= MakeShared<ScriptEngine>();

		shader_storage.storage		= &ppln_storage;
		shader_storage.shaderFolders= std::move(shader_dirs);
		ShaderStorage::SetInstance( &shader_storage );

		CHECK_ERR( LoadPipelines( script_engine, pipelines ));
		CHECK_ERR( CompileShaders( include_dirs ));
		CHECK_ERR( SavePack( info->outputPackName ));

		CHECK_ERR( not shader_storage.hashCollisionCheck.HasCollisions() );

		ShaderStorage::SetInstance( null );
		return true;
	}

}	// AE::PipelineCompiler


// TODO: dll main
int main ()
{
	return 0;
}
