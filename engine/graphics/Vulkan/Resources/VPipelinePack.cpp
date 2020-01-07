// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#ifdef AE_ENABLE_VULKAN

# include "graphics/Vulkan/Resources/VPipelinePack.h"
# include "graphics/Vulkan/VResourceManager.h"
# include "graphics/Vulkan/VEnumCast.h"

# include "stl/Memory/StackAllocator.h"

# include "serializing/Deserializer.h"
# include "serializing/ObjectFactory.h"

namespace AE::PipelineCompiler
{
	bool Deserialize_Uniform (Serializing::Deserializer const& des, uint samplerStorageSize, OUT DescriptorSetLayoutDesc::Uniform &un);
}

namespace AE::Graphics
{
	using ShaderModule = VGraphicsPipelineTemplate::ShaderModule;
	
/*
=================================================
	destructor
=================================================
*/
	VPipelinePack::~VPipelinePack ()
	{
		EXLOCK( _drCheck );
		CHECK( _shaderModules.empty() );
	}
	
/*
=================================================
	Destroy
=================================================
*/
	void VPipelinePack::Destroy (VResourceManager &resMngr)
	{
		EXLOCK( _drCheck );

		auto&	dev = resMngr.GetDevice();

		for (auto& sm : _shaderModules)
		{
			dev.vkDestroyShaderModule( dev.GetVkDevice(), sm, null );
		}
		_shaderModules.clear();

		_dsLayouts.clear();
		_pplnLayouts.clear();
	}

/*
=================================================
	Create
----
	TODO: multithreading
=================================================
*/
	bool VPipelinePack::Create (VResourceManager &resMngr, const SharedPtr<RStream> &stream, INOUT PipelineRefs &refs)
	{
		EXLOCK( _drCheck );

		Allocator_t					allocator;
		Serializing::Deserializer	des;
		des.stream = stream;
		
		uint	version	= 0;
		CHECK_ERR( des( OUT version ) and version == PipelineStorage::Version );

		CHECK_ERR( _LoadDescrSetLayouts( resMngr, des, allocator ));
		CHECK_ERR( _LoadPipelineLayouts( resMngr, des ));
		CHECK_ERR( _LoadSpirvShaders( resMngr.GetDevice(), des, allocator ));
		
		ArrayView<VGraphicsPipelineTemplateID>	graphics_pipelines;
		CHECK_ERR( _LoadGraphicsPipelines( resMngr, des, allocator, OUT graphics_pipelines ));
		
		ArrayView<VMeshPipelineTemplateID>		mesh_pipelines;
		CHECK_ERR( _LoadMeshPipelines( resMngr, des, allocator, OUT mesh_pipelines ));
		
		ArrayView<VComputePipelineTemplateID>	compute_pipelines;
		CHECK_ERR( _LoadComputePipelines( resMngr, des, allocator, OUT compute_pipelines ));

		CHECK_ERR( _LoadPipelineNames( graphics_pipelines, mesh_pipelines, compute_pipelines, des, INOUT refs ));
		return true;
	}
	
/*
=================================================
	_LoadDescrSetLayouts
=================================================
*/
	bool VPipelinePack::_LoadDescrSetLayouts (VResourceManager &resMngr, Serializing::Deserializer &des, Allocator_t &allocator)
	{
		EMarker	marker;
		uint	count	= 0;
		CHECK_ERR( des( OUT marker, OUT count ) and marker == EMarker::DescrSetLayouts );
		
		if ( not count )
			return true;

		_dsLayouts.resize( count );

		for (uint i = 0; i < count; ++i)
		{
			const auto	bm				= allocator.Push();
			VkSampler *	vk_samplers		= null;
			uint		sampler_count	= 0;
			uint		uniform_count	= 0;

			CHECK_ERR( des( OUT sampler_count, OUT uniform_count ));

			if ( sampler_count )
			{
				vk_samplers = allocator.Alloc< VkSampler >( sampler_count );
				CHECK_ERR( vk_samplers );

				for (uint j = 0; j < sampler_count; ++j)
				{
					SamplerName	name;
					CHECK_ERR( des( OUT name ));

					vk_samplers[j] = resMngr.GetVkSampler( name );
					CHECK_ERR( vk_samplers[j] );
				}
			}
			
			auto*	un_names	= allocator.Alloc< UniformName >( uniform_count );
			auto*	un_data		= allocator.Alloc< VDescriptorSetLayout::Uniform >( uniform_count );
			bool	result		= true;

			for (uint j = 0; j < uniform_count; ++j) {
				result &= Deserialize_Uniform( des, sampler_count, OUT un_data[j] );
			}
			CHECK_ERR( result );
			
			_dsLayouts[i] = resMngr.CreateDescriptorSetLayout(
								VDescriptorSetLayout::Uniforms_t{ uniform_count, un_names, un_data },
								ArrayView{ vk_samplers, sampler_count });
			CHECK_ERR( _dsLayouts[i] );

			allocator.Pop( bm );
		}

		return true;
	}
	
/*
=================================================
	_LoadPipelineLayouts
=================================================
*/
	bool VPipelinePack::_LoadPipelineLayouts (VResourceManager &resMngr, Serializing::Deserializer &des)
	{
		EMarker	marker;
		uint	count	= 0;
		CHECK_ERR( des( OUT marker, OUT count ) and marker == EMarker::PipelineLayouts );
		
		if ( not count )
			return true;

		_pplnLayouts.resize( count );

		for (uint i = 0; i < count; ++i)
		{
			PipelineLayoutDesc	desc;
			CHECK_ERR( des( OUT desc ));

			VPipelineLayout::DescriptorSets_t	desc_sets;

			for (size_t j = 0; j < desc.descrSets.size(); ++j)
			{
				auto&	src = desc.descrSets[j];
				CHECK_ERR( size_t(src.second.uid) < _dsLayouts.size() );

				VDescriptorSetLayoutID	id			= _dsLayouts[ size_t(src.second.uid) ];
				auto*					ds_layout	= resMngr.GetResource( id );
				CHECK_ERR( ds_layout );

				desc_sets.insert_or_assign( src.first, VPipelineLayout::DescSetLayout{ id, ds_layout->Handle(), src.second.index });
			}

			_pplnLayouts[i] = resMngr.CreatePipelineLayout( desc_sets, desc.pushConstants.items );
			CHECK_ERR( _pplnLayouts[i] );
		}
		return true;
	}
	
/*
=================================================
	_LoadSpirvShaders
=================================================
*/
	bool VPipelinePack::_LoadSpirvShaders (const VDevice &dev, Serializing::Deserializer &des, Allocator_t &allocator)
	{
		EMarker	marker;
		uint	count	= 0;
		CHECK_ERR( des( OUT marker, OUT count ) and marker == EMarker::SpirvShaders );
		
		if ( not count )
			return true;

		_shaderModules.resize( count );

		for (uint i = 0; i < count; ++i)
		{
			auto	bm			= allocator.Push();
			uint	code_size	= 0;

			PipelineCompiler::SpirvShaderCode::SpecConstants_t	spec;
			CHECK_ERR( des( OUT spec, OUT code_size ));

			uint*	code = allocator.Alloc<uint>( code_size );
			CHECK_ERR( code );
			CHECK_ERR( des.stream->Read( code, code_size * SizeOf<uint> ));
			
			VkShaderModuleCreateInfo	shader_info = {};
			shader_info.sType		= VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			shader_info.codeSize	= code_size;
			shader_info.pCode		= code;
			
			VK_CHECK( dev.vkCreateShaderModule( dev.GetVkDevice(), &shader_info, null, OUT &_shaderModules[i] ));

			allocator.Pop( bm );
		}
		return true;
	}
	
/*
=================================================
	_LoadGraphicsPipelines
=================================================
*/
	bool VPipelinePack::_LoadGraphicsPipelines (VResourceManager &resMngr, Serializing::Deserializer &des, Allocator_t &allocator,
												OUT ArrayView<VGraphicsPipelineTemplateID> &outPipelines)
	{
		EMarker	marker;
		uint	count	= 0;
		CHECK_ERR( des( OUT marker, OUT count ) and marker == EMarker::GraphicsPipelines );
		
		if ( not count )
			return true;

		auto*	gpipelines = allocator.Alloc<VGraphicsPipelineTemplateID>( count );
		CHECK_ERR( gpipelines );
		
		PipelineCompiler::GraphicsPipelineDesc	desc;
		for (uint i = 0; i < count; ++i)
		{
			CHECK_ERR( desc.Deserialize( des ));
			CHECK_ERR( size_t(desc.layout) < _pplnLayouts.size() );
			
			auto *				modules	= allocator.Alloc<ShaderModule>( desc.shaders.size() );
			VPipelineLayoutID	id		= _pplnLayouts[ size_t(desc.layout) ];

			for (size_t j = 0; j < desc.shaders.size(); ++j)
			{
				const uint	idx = uint(desc.shaders[j].second & ~PipelineCompiler::ShaderUID::SPIRV);
				CHECK_ERR( EnumEq( desc.shaders[j].second, PipelineCompiler::ShaderUID::SPIRV ));
				CHECK_ERR( idx < _shaderModules.size() );

				modules[j].stage	= VEnumCast( desc.shaders[j].first );
				modules[j].module	= _shaderModules[idx];
			}

			gpipelines[i] = resMngr.CreateGPTemplate( id, desc, ArrayView{modules, desc.shaders.size()} );
			CHECK_ERR( gpipelines[i] );
		}

		outPipelines = ArrayView<VGraphicsPipelineTemplateID>{ gpipelines, count };
		return true;
	}
	
/*
=================================================
	_LoadMeshPipelines
=================================================
*/
	bool VPipelinePack::_LoadMeshPipelines (VResourceManager &resMngr, Serializing::Deserializer &des, Allocator_t &allocator,
											OUT ArrayView<VMeshPipelineTemplateID> &outPipelines)
	{
		EMarker	marker;
		uint	count	= 0;
		CHECK_ERR( des( OUT marker, OUT count ) and marker == EMarker::MeshPipelines );
		
		if ( not count )
			return true;
		
		auto*	mpipelines = allocator.Alloc<VMeshPipelineTemplateID>( count );
		CHECK_ERR( mpipelines );
		
		PipelineCompiler::MeshPipelineDesc	desc;
		for (uint i = 0; i < count; ++i)
		{
			CHECK_ERR( desc.Deserialize( des ));
			CHECK_ERR( size_t(desc.layout) < _pplnLayouts.size() );
			
			auto *				modules	= allocator.Alloc<ShaderModule>( desc.shaders.size() );
			VPipelineLayoutID	id		= _pplnLayouts[ size_t(desc.layout) ];

			for (size_t j = 0; j < desc.shaders.size(); ++j)
			{
				const uint	idx = uint(desc.shaders[j].second & ~PipelineCompiler::ShaderUID::SPIRV);
				CHECK_ERR( EnumEq( desc.shaders[j].second, PipelineCompiler::ShaderUID::SPIRV ));
				CHECK_ERR( idx < _shaderModules.size() );

				modules[j].stage	= VEnumCast( desc.shaders[j].first );
				modules[j].module	= _shaderModules[idx];
			}

			mpipelines[i] = resMngr.CreateMPTemplate( id, desc, ArrayView{modules, desc.shaders.size()} );
			CHECK_ERR( mpipelines[i] );
		}

		outPipelines = ArrayView<VMeshPipelineTemplateID>{ mpipelines, count };
		return true;
	}
	
/*
=================================================
	_LoadComputePipelines
=================================================
*/
	bool VPipelinePack::_LoadComputePipelines (VResourceManager &resMngr, Serializing::Deserializer &des, Allocator_t &allocator,
											   OUT ArrayView<VComputePipelineTemplateID> &outPipelines)
	{
		EMarker	marker;
		uint	count	= 0;
		CHECK_ERR( des( OUT marker, OUT count ) and marker == EMarker::ComputePipelines );
		
		if ( not count )
			return true;
		
		auto*	cpipelines = allocator.Alloc<VComputePipelineTemplateID>( count );
		CHECK_ERR( cpipelines );
		
		PipelineCompiler::ComputePipelineDesc	desc;
		for (uint i = 0; i < count; ++i)
		{
			CHECK_ERR( desc.Deserialize( des ));
			CHECK_ERR( size_t(desc.layout) < _pplnLayouts.size() );
			
			VPipelineLayoutID	id	= _pplnLayouts[ size_t(desc.layout) ];
			const uint			idx	= uint(desc.shader & ~PipelineCompiler::ShaderUID::SPIRV);

			CHECK_ERR( EnumEq( desc.shader, PipelineCompiler::ShaderUID::SPIRV ));
			CHECK_ERR( idx < _shaderModules.size() );

			cpipelines[i] = resMngr.CreateCPTemplate( id, desc, _shaderModules[idx] );
			CHECK_ERR( cpipelines[i] );
		}

		outPipelines = ArrayView<VComputePipelineTemplateID>{ cpipelines, count };
		return true;
	}

/*
=================================================
	_LoadPipelineNames
=================================================
*/
	bool VPipelinePack::_LoadPipelineNames (ArrayView<VGraphicsPipelineTemplateID>	graphicsPipelines,
											ArrayView<VMeshPipelineTemplateID>		meshPipelines,
											ArrayView<VComputePipelineTemplateID>	computePipelines,
											//ArrayView<VRayTracingPipelineTemplateID>	rayTracingPipelines,
											Serializing::Deserializer				&des,
											INOUT PipelineRefs						&refs)
	{
		using PipelineUID		= PipelineCompiler::PipelineUID;
		using PipelineNameMap_t	= Array<Pair< PipelineName, PipelineUID >>;

		PipelineNameMap_t	pipeline_names;
		EMarker				marker;

		CHECK_ERR( des( OUT marker ) and marker == EMarker::PipelineNames );
		CHECK_ERR( des( OUT pipeline_names ));

		EXLOCK( refs.guard );

		for (auto&[name, uid] : pipeline_names)
		{
			const uint	idx = uint(uid & ~PipelineUID::_Mask);

			BEGIN_ENUM_CHECKS();
			switch ( uid & PipelineUID::_Mask )
			{
				case PipelineUID::Graphics :
					CHECK( refs.graphics.insert_or_assign( name, graphicsPipelines[idx] ).second );
					break;

				case PipelineUID::Mesh :
					CHECK( refs.mesh.insert_or_assign( name, meshPipelines[idx] ).second );
					break;

				case PipelineUID::Compute :
					CHECK( refs.compute.insert_or_assign( name, computePipelines[idx] ).second );
					break;

				case PipelineUID::RayTracing :
					//CHECK( refs.rayTracing.insert_or_assign( name, rayTracingPipelines[idx] ).second );
					//break;

				case PipelineUID::_Mask :
				case PipelineUID::Unknown :
					break;
			}
			END_ENUM_CHECKS();
		}
		return true;
	}


}	// AE::Graphics

# include "pipeline_compiler/Public/PipelinePackDeserializer.cpp"

#endif	// AE_ENABLE_VULKAN
