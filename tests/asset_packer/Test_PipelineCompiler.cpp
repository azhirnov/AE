// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Platforms/WindowsLibrary.h"
#include "stl/Platforms/PosixLibrary.h"
#include "stl/Stream/FileStream.h"
#include "serializing/Deserializer.h"
#include "serializing/ObjectFactory.h"
#include "pipeline_compiler/PipelineCompiler.h"
#include "pipeline_compiler/Public/PipelinePack.h"

using namespace AE::STL;
using namespace AE::PipelineCompiler;

#define TEST	CHECK_FATAL

namespace
{
	decltype(&CompilePipelines)		compile_pipelines = null;

	using EMarker	= PipelineStorage::EMarker;


	void PipelineCompiler_Test1 ()
	{
		const wchar_t*	pipeline_folder[]	= { L"pipelines" };
		const wchar_t*	shader_folder[]		= { L"shaders" };
		const wchar_t*	include_dir[]		= { L"shaders/include" };
		Path			output				= L"_output";
		
		FileSystem::RemoveAll( output );
		TEST( FileSystem::CreateDirectories( output ));

		output.append("pipelines.bin");

		PipelinesInfo	info = {};
		info.inPipelines		= null;
		info.inPipelineCount	= 0;
		info.pipelineFolders	= pipeline_folder;
		info.pipelineFolderCount= uint(CountOf( pipeline_folder ));
		info.includeDirs		= include_dir;
		info.includeDirCount	= uint(CountOf( include_dir ));
		info.shaderFolders		= shader_folder;
		info.shaderFolderCount	= uint(CountOf( shader_folder ));
		info.outputPackName		= output.c_str();

		TEST( compile_pipelines( &info ));


		auto	file = MakeShared<FileRStream>( output );
		TEST( file->IsOpen() );

		AE::Serializing::Deserializer	des;
		des.stream = file;

		EMarker	marker;
		uint	version;
		TEST( des( OUT version ) and version == PipelineStorage::Version );

		Array< DescriptorSetLayoutDesc >	ds_layouts;
		TEST( des( OUT marker ) and marker == EMarker::DescrSetLayouts );
		TEST( des( OUT ds_layouts ));
		
		Array< PipelineLayoutDesc >		ppln_layouts;
		TEST( des( OUT marker ) and marker == EMarker::PipelineLayouts );
		TEST( des( OUT ppln_layouts ));
		
		Array< SpirvShaderCode >		spirv_shaders;
		TEST( des( OUT marker ) and marker == EMarker::SpirvShaders );
		TEST( des( OUT spirv_shaders ));
		
		Array< GraphicsPipelineDesc >	gpipelines;
		TEST( des( OUT marker ) and marker == EMarker::GraphicsPipelines );
		TEST( des( OUT gpipelines ));
		
		Array< MeshPipelineDesc >		mpipelines;
		TEST( des( OUT marker ) and marker == EMarker::MeshPipelines );
		TEST( des( OUT mpipelines ));
		
		Array< ComputePipelineDesc >	cpipelines;
		TEST( des( OUT marker ) and marker == EMarker::ComputePipelines );
		TEST( des( OUT cpipelines ));

		uint	rt_count;
		TEST( des( OUT marker ) and marker == EMarker::RayTracingPipelines );
		TEST( des( OUT rt_count ));

		Array<Pair< PipelineName, PipelineUID >>	ppln_names;
		TEST( des( OUT marker ) and marker == EMarker::PipelineNames );
		TEST( des( OUT ppln_names ));


		using EDescriptorType = DescriptorSetLayoutDesc::EDescriptorType;

		TEST( ds_layouts.size() == 3 );
		TEST( ds_layouts[0].samplerStorage.size() == 0 );
		TEST( ds_layouts[1].samplerStorage.size() == 0 );
		TEST( ds_layouts[2].samplerStorage.size() == 0 );
		TEST( ds_layouts[0].uniforms.size() == 1 );
		TEST( ds_layouts[1].uniforms.size() == 1 );
		TEST( ds_layouts[2].uniforms.size() == 1 );
		
		TEST( ds_layouts[0].uniforms[0].first == UniformName{"un_ColorTexture"} );
		TEST( ds_layouts[0].uniforms[0].second.type == EDescriptorType::CombinedImage );
		TEST( ds_layouts[0].uniforms[0].second.index == 0 );
		TEST( ds_layouts[0].uniforms[0].second.arraySize == 1 );
		TEST( ds_layouts[0].uniforms[0].second.combinedImage.state == (EResourceState::ShaderSample | EResourceState::_FragmentShader) );
		TEST( ds_layouts[0].uniforms[0].second.combinedImage.type == EImageType::Sampler2D );
		
		TEST( ds_layouts[1].uniforms[0].first == UniformName{"DrawCmdUBuf"} );
		TEST( ds_layouts[1].uniforms[0].second.type == EDescriptorType::UniformBuffer );
		TEST( ds_layouts[1].uniforms[0].second.index == 0 );
		TEST( ds_layouts[1].uniforms[0].second.arraySize == 1 );
		TEST( ds_layouts[1].uniforms[0].second.buffer.dynamicOffsetIndex == UMax );
		TEST( ds_layouts[1].uniforms[0].second.buffer.arrayStride == 0_b );
		TEST( ds_layouts[1].uniforms[0].second.buffer.state == (EResourceState::UniformRead | EResourceState::_VertexShader) );
		TEST( ds_layouts[1].uniforms[0].second.buffer.staticSize == 64_b );

		TEST( ds_layouts[2].uniforms[0].first == UniformName{"un_OutImage"} );
		TEST( ds_layouts[2].uniforms[0].second.type == EDescriptorType::StorageImage );
		TEST( ds_layouts[2].uniforms[0].second.index == 0 );
		TEST( ds_layouts[2].uniforms[0].second.arraySize == 1 );
		TEST( ds_layouts[2].uniforms[0].second.storageImage.state == (EResourceState::ShaderWrite | EResourceState::_ComputeShader) );
		TEST( ds_layouts[2].uniforms[0].second.storageImage.type == EImageType::Sampler2D );

		TEST( ppln_layouts.size() == 3 );
		TEST( ppln_layouts[0].descrSets.size() == 2 );
		TEST( ppln_layouts[0].pushConstants.items.size() == 0 );
		TEST( ppln_layouts[1].descrSets.size() == 1 );
		TEST( ppln_layouts[1].pushConstants.items.size() == 0 );
		TEST( ppln_layouts[2].descrSets.size() == 1 );
		TEST( ppln_layouts[2].pushConstants.items.size() == 0 );
		
		TEST( ppln_layouts[0].descrSets[0].first == DescriptorSetName{"2"} );
		TEST( ppln_layouts[0].descrSets[0].second.index == 2 );
		TEST( ppln_layouts[0].descrSets[0].second.uid == DescrSetUID(1) );
		TEST( ppln_layouts[0].descrSets[1].first == DescriptorSetName{"1"} );
		TEST( ppln_layouts[0].descrSets[1].second.index == 1 );
		TEST( ppln_layouts[0].descrSets[1].second.uid == DescrSetUID(0) );
		
		TEST( ppln_layouts[1].descrSets[0].first == DescriptorSetName{"1"} );
		TEST( ppln_layouts[1].descrSets[0].second.index == 1 );
		TEST( ppln_layouts[1].descrSets[0].second.uid == DescrSetUID(0) );
		
		TEST( ppln_layouts[2].descrSets[0].first == DescriptorSetName{"0"} );
		TEST( ppln_layouts[2].descrSets[0].second.index == 0 );
		TEST( ppln_layouts[2].descrSets[0].second.uid == DescrSetUID(2) );

		TEST( spirv_shaders.size() == 4 );
		TEST( gpipelines.size() == 1 );
		TEST( mpipelines.size() == 1 );
		TEST( cpipelines.size() == 1 );
		TEST( rt_count == 0 );

		TEST( ppln_names.size() == 4 );
		TEST( ppln_names[0].first == PipelineName{"graphics_2"} );
		TEST( ppln_names[1].first == PipelineName{"mesh_1"} );
		TEST( ppln_names[2].first == PipelineName{"compute_1"} );
		TEST( ppln_names[3].first == PipelineName{"graphics_1"} );
	}
}


extern void Test_PipelineCompiler ()
{
	{
		Path	dll_path;
		TEST( FileSystem::Search( "PipelineCompiler.dll", 3, 3, OUT dll_path ));

		Library		lib;
		TEST( lib.Load( dll_path ));
		TEST( lib.GetProcAddr( "CompilePipelines", OUT compile_pipelines ));
		
		TEST( FileSystem::FindAndSetCurrent( "pipeline_test", 5 ));

		PipelineCompiler_Test1();
	}
	AE_LOGI( "Test_PipelineCompiler - passed" );
}

#include "pipeline_compiler/Public/PipelinePackDeserializer.cpp"
