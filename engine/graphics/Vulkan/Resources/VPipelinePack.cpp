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

		for (auto& sm : _shaderModules) {
			dev.vkDestroyShaderModule( dev.GetVkDevice(), sm, null );
		}
		_shaderModules.clear();

		for (auto& id : _dsLayouts) {
			resMngr.ReleaseResource( INOUT id );
		}
		_dsLayouts.clear();

		for (auto& id : _pplnLayouts) {
			resMngr.ReleaseResource( INOUT id );
		}
		_pplnLayouts.clear();

		for (auto& id : _gpipelines) {
			resMngr.ReleaseResource( INOUT id );
		}
		_gpipelines.clear();

		for (auto& id : _mpipelines) {
			resMngr.ReleaseResource( INOUT id );
		}
		_mpipelines.clear();

		for (auto& id : _cpipelines) {
			resMngr.ReleaseResource( INOUT id );
		}
		_cpipelines.clear();

		for (auto& id : _renderPassOutputs) {
			resMngr.ReleaseResource( INOUT id );
		}
		_renderPassOutputs.clear();
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

		StackAllocator_t			allocator;
		Serializing::Deserializer	des;
		des.stream = stream;
		
		uint	version	= 0;
		CHECK_ERR( des( OUT version ) and version == PipelineStorage::Version );

		CHECK_ERR( _LoadDescrSetLayouts( resMngr, des, allocator ));
		CHECK_ERR( _LoadPipelineLayouts( resMngr, des ));
		CHECK_ERR( _LoadRenderPasses( resMngr, des, INOUT refs ));
		CHECK_ERR( _LoadSpirvShaders( resMngr.GetDevice(), des, allocator ));
		CHECK_ERR( _LoadGraphicsPipelines( resMngr, des, allocator ));
		CHECK_ERR( _LoadMeshPipelines( resMngr, des, allocator ));
		CHECK_ERR( _LoadComputePipelines( resMngr, des ));
		CHECK_ERR( _LoadPipelineNames( des, INOUT refs ));
		return true;
	}
	
/*
=================================================
	_LoadDescrSetLayouts
=================================================
*/
	bool VPipelinePack::_LoadDescrSetLayouts (VResourceManager &resMngr, Serializing::Deserializer &des, StackAllocator_t &allocator)
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
			
			_dsLayouts[i].Set( resMngr.CreateDescriptorSetLayout(
								VDescriptorSetLayout::Uniforms_t{ uniform_count, un_names, un_data },
								ArrayView{ vk_samplers, sampler_count }));
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

			_pplnLayouts[i].Set( resMngr.CreatePipelineLayout( desc_sets, desc.pushConstants.items ));
			CHECK_ERR( _pplnLayouts[i] );
		}
		return true;
	}
	
/*
=================================================
	_LoadRenderPasses
=================================================
*/
	bool VPipelinePack::_LoadRenderPasses (VResourceManager &resMngr, Serializing::Deserializer &des, INOUT PipelineRefs &refs)
	{
		EMarker	marker;
		uint	count	= 0;
		CHECK_ERR( des( OUT marker, OUT count ) and marker == EMarker::RenderPasses );
		
		_renderPassOutputs.resize( count );

		for (uint i = 0; i < count; ++i)
		{
			PipelineCompiler::RenderPassInfo	info;
			CHECK_ERR( des( OUT info ));

			_renderPassOutputs[i].Set( resMngr.CreateRenderPassOutput( info.fragmentOutputs ));
			CHECK_ERR( _renderPassOutputs[i] );
		}
		
		CHECK_ERR( des( OUT marker, OUT count ) and marker == EMarker::RenderPassNames );
		
		for (uint i = 0; i < count; ++i)
		{
			RenderPassName	name;
			uint			idx;
			CHECK_ERR( des( OUT name, OUT idx ));

			CHECK_ERR( idx < _renderPassOutputs.size() );

			EXLOCK( refs.renderPassNames.guard );
			CHECK( refs.renderPassNames.map.insert_or_assign( name, _renderPassOutputs[idx] ).second );
		}

		return true;
	}

/*
=================================================
	_LoadSpirvShaders
=================================================
*/
	bool VPipelinePack::_LoadSpirvShaders (const VDevice &dev, Serializing::Deserializer &des, StackAllocator_t &allocator)
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
	bool VPipelinePack::_LoadGraphicsPipelines (VResourceManager &resMngr, Serializing::Deserializer &des, StackAllocator_t &allocator)
	{
		EMarker	marker;
		uint	count	= 0;
		CHECK_ERR( des( OUT marker, OUT count ) and marker == EMarker::GraphicsPipelines );
		
		if ( not count )
			return true;

		_gpipelines.resize( count );
		
		PipelineCompiler::GraphicsPipelineDesc	desc;
		for (uint i = 0; i < count; ++i)
		{
			CHECK_ERR( desc.Deserialize( des ));
			CHECK_ERR( size_t(desc.layout) < _pplnLayouts.size() );
			CHECK_ERR( size_t(desc.renderPass) < _renderPassOutputs.size() );
			
			auto				bm			= allocator.Push();
			auto *				modules		= allocator.Alloc<ShaderModule>( desc.shaders.size() );
			VPipelineLayoutID	layout_id	= _pplnLayouts[ size_t(desc.layout) ];
			VRenderPassOutputID	rp_output_id= _renderPassOutputs[ size_t(desc.renderPass) ];

			for (size_t j = 0; j < desc.shaders.size(); ++j)
			{
				const uint	idx = uint(desc.shaders[j].second & ~PipelineCompiler::ShaderUID::SPIRV);
				CHECK_ERR( EnumEq( desc.shaders[j].second, PipelineCompiler::ShaderUID::SPIRV ));
				CHECK_ERR( idx < _shaderModules.size() );

				modules[j].stage	= VEnumCast( desc.shaders[j].first );
				modules[j].module	= _shaderModules[idx];
			}

			_gpipelines[i].Set( resMngr.CreateGPTemplate( layout_id, rp_output_id, desc, ArrayView{modules, desc.shaders.size()} ));
			CHECK_ERR( _gpipelines[i] );
			
			allocator.Pop( bm );
		}
		return true;
	}
	
/*
=================================================
	_LoadMeshPipelines
=================================================
*/
	bool VPipelinePack::_LoadMeshPipelines (VResourceManager &resMngr, Serializing::Deserializer &des, StackAllocator_t &allocator)
	{
		EMarker	marker;
		uint	count	= 0;
		CHECK_ERR( des( OUT marker, OUT count ) and marker == EMarker::MeshPipelines );
		
		if ( not count )
			return true;
		
		_mpipelines.resize( count );
		
		PipelineCompiler::MeshPipelineDesc	desc;
		for (uint i = 0; i < count; ++i)
		{
			CHECK_ERR( desc.Deserialize( des ));
			CHECK_ERR( size_t(desc.layout) < _pplnLayouts.size() );
			CHECK_ERR( size_t(desc.renderPass) < _renderPassOutputs.size() );
			
			auto				bm			= allocator.Push();
			auto *				modules		= allocator.Alloc<ShaderModule>( desc.shaders.size() );
			VPipelineLayoutID	layout_id	= _pplnLayouts[ size_t(desc.layout) ];
			VRenderPassOutputID	rp_output_id= _renderPassOutputs[ size_t(desc.renderPass) ];

			for (size_t j = 0; j < desc.shaders.size(); ++j)
			{
				const uint	idx = uint(desc.shaders[j].second & ~PipelineCompiler::ShaderUID::SPIRV);
				CHECK_ERR( EnumEq( desc.shaders[j].second, PipelineCompiler::ShaderUID::SPIRV ));
				CHECK_ERR( idx < _shaderModules.size() );

				modules[j].stage	= VEnumCast( desc.shaders[j].first );
				modules[j].module	= _shaderModules[idx];
			}

			_mpipelines[i].Set( resMngr.CreateMPTemplate( layout_id, rp_output_id, desc, ArrayView{modules, desc.shaders.size()} ));
			CHECK_ERR( _mpipelines[i] );
			
			allocator.Pop( bm );
		}
		return true;
	}
	
/*
=================================================
	_LoadComputePipelines
=================================================
*/
	bool VPipelinePack::_LoadComputePipelines (VResourceManager &resMngr, Serializing::Deserializer &des)
	{
		EMarker	marker;
		uint	count	= 0;
		CHECK_ERR( des( OUT marker, OUT count ) and marker == EMarker::ComputePipelines );
		
		if ( not count )
			return true;
		
		_cpipelines.resize( count );
		
		PipelineCompiler::ComputePipelineDesc	desc;
		for (uint i = 0; i < count; ++i)
		{
			CHECK_ERR( desc.Deserialize( des ));
			CHECK_ERR( size_t(desc.layout) < _pplnLayouts.size() );
			
			VPipelineLayoutID	id	= _pplnLayouts[ size_t(desc.layout) ];
			const uint			idx	= uint(desc.shader & ~PipelineCompiler::ShaderUID::SPIRV);

			CHECK_ERR( EnumEq( desc.shader, PipelineCompiler::ShaderUID::SPIRV ));
			CHECK_ERR( idx < _shaderModules.size() );

			_cpipelines[i].Set( resMngr.CreateCPTemplate( id, desc, _shaderModules[idx] ));
			CHECK_ERR( _cpipelines[i] );
		}
		return true;
	}

/*
=================================================
	_LoadPipelineNames
=================================================
*/
	bool VPipelinePack::_LoadPipelineNames (Serializing::Deserializer &des, INOUT PipelineRefs &refs)
	{
		using PipelineUID		= PipelineCompiler::PipelineUID;
		using PipelineNameMap_t	= Array<Pair< PipelineName, PipelineUID >>;	// TODO: allocator

		PipelineNameMap_t	pipeline_names;
		EMarker				marker;

		CHECK_ERR( des( OUT marker ) and marker == EMarker::PipelineNames );
		CHECK_ERR( des( OUT pipeline_names ));

		for (auto&[name, uid] : pipeline_names)
		{
			const uint	idx = uint(uid & ~PipelineUID::_Mask);

			BEGIN_ENUM_CHECKS();
			switch ( uid & PipelineUID::_Mask )
			{
				case PipelineUID::Graphics : {
					EXLOCK( refs.graphics.guard );
					CHECK( refs.graphics.map.insert_or_assign( name, _gpipelines[idx] ).second );
					break;
				}
				case PipelineUID::Mesh : {
					EXLOCK( refs.mesh.guard );
					CHECK( refs.mesh.map.insert_or_assign( name, _mpipelines[idx] ).second );
					break;
				}
				case PipelineUID::Compute : {
					EXLOCK( refs.compute.guard );
					CHECK( refs.compute.map.insert_or_assign( name, _cpipelines[idx] ).second );
					break;
				}
				case PipelineUID::RayTracing :
					//EXLOCK( refs.rayTracing.guard );
					//CHECK( refs.rayTracing.map.insert_or_assign( name, _rtpipelines[idx] ).second );
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
