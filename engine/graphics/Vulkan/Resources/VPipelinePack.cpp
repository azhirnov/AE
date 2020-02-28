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
		CHECK( _shaderModules.Get<1>() == null );

		_allocator.SetBlockSize( 64_Kb );
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

		for (size_t i = 0, cnt = _shaderModules.Get<0>(); i < cnt; ++i) {
			dev.vkDestroyShaderModule( dev.GetVkDevice(), _shaderModules.Get<1>()[i], null );
		}
		_shaderModules = Default;

		for (size_t i = 0, cnt = _dsLayouts.Get<0>(); i < cnt; ++i) {
			resMngr.ReleaseResource( INOUT _dsLayouts.Get<1>()[i] );
		}
		_dsLayouts = Default;

		for (size_t i = 0, cnt = _pplnLayouts.Get<0>(); i < cnt; ++i) {
			resMngr.ReleaseResource( INOUT _pplnLayouts.Get<1>()[i] );
		}
		_pplnLayouts = Default;

		for (size_t i = 0, cnt = _gpipelines.Get<0>(); i < cnt; ++i) {
			resMngr.ReleaseResource( INOUT _gpipelines.Get<1>()[i] );
		}
		_gpipelines = Default;

		for (size_t i = 0, cnt = _mpipelines.Get<0>(); i < cnt; ++i) {
			resMngr.ReleaseResource( INOUT _mpipelines.Get<1>()[i] );
		}
		_mpipelines = Default;

		for (size_t i = 0, cnt = _cpipelines.Get<0>(); i < cnt; ++i) {
			resMngr.ReleaseResource( INOUT _cpipelines.Get<1>()[i] );
		}
		_cpipelines = Default;

		for (size_t i = 0, cnt = _renderPassOutputs.Get<0>(); i < cnt; ++i) {
			resMngr.ReleaseResource( INOUT _renderPassOutputs.Get<1>()[i] );
		}
		_renderPassOutputs = Default;

		_allocator.Release();
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

		_allocator.Discard();

		StackAllocator_t			stack_alloc;
		Serializing::Deserializer	des;
		des.stream = stream;

		stack_alloc.SetBlockSize( 64_Kb );
		
		uint	version	= 0;
		CHECK_ERR( des( OUT version ) and version == PipelineStorage::Version );

		CHECK_ERR( _LoadDescrSetLayouts( resMngr, des, stack_alloc ));
		CHECK_ERR( _LoadPipelineLayouts( resMngr, des ));
		CHECK_ERR( _LoadRenderPasses( resMngr, des, INOUT refs ));
		CHECK_ERR( _LoadSpirvShaders( resMngr.GetDevice(), des, stack_alloc ));
		CHECK_ERR( _LoadGraphicsPipelines( resMngr, des, stack_alloc ));
		CHECK_ERR( _LoadMeshPipelines( resMngr, des, stack_alloc ));
		CHECK_ERR( _LoadComputePipelines( resMngr, des ));
		CHECK_ERR( _LoadRayTracingPipelines( resMngr, des ));
		CHECK_ERR( _LoadPipelineNames( des, INOUT refs ));
		return true;
	}
	
/*
=================================================
	_LoadDescrSetLayouts
=================================================
*/
	bool VPipelinePack::_LoadDescrSetLayouts (VResourceManager &resMngr, Serializing::Deserializer &des, StackAllocator_t &stackAlloc)
	{
		EMarker	marker;
		uint	count	= 0;
		CHECK_ERR( des( OUT marker, OUT count ) and marker == EMarker::DescrSetLayouts );
		
		if ( not count )
			return true;

		_dsLayouts = { count, _allocator.Alloc<UniqueID<DescriptorSetLayoutID>>( count )};

		for (uint i = 0; i < count; ++i)
		{
			const auto	bm				= stackAlloc.Push();
			VkSampler *	vk_samplers		= null;
			uint		sampler_count	= 0;
			uint		uniform_count	= 0;

			CHECK_ERR( des( OUT uniform_count, OUT sampler_count ));

			if ( sampler_count )
			{
				vk_samplers = stackAlloc.Alloc< VkSampler >( sampler_count );
				CHECK_ERR( vk_samplers );

				for (uint j = 0; j < sampler_count; ++j)
				{
					SamplerName	name;
					CHECK_ERR( des( OUT name ));

					vk_samplers[j] = resMngr.GetVkSampler( name );
					CHECK_ERR( vk_samplers[j] );
				}
			}
			
			auto*	un_names	= stackAlloc.Alloc< UniformName >( uniform_count );
			auto*	un_data		= stackAlloc.Alloc< VDescriptorSetLayout::Uniform >( uniform_count );
			bool	result		= true;
			
			for (uint j = 0; j < uniform_count; ++j) {
				result &= des( OUT un_names[j] );
			}
			for (uint j = 0; j < uniform_count; ++j) {
				result &= Deserialize_Uniform( des, sampler_count, OUT un_data[j] );
			}
			CHECK_ERR( result );
			
			auto	item = resMngr.CreateDescriptorSetLayout(
								VDescriptorSetLayout::Uniforms_t{ uniform_count, un_names, un_data },
								ArrayView{ vk_samplers, sampler_count });
			CHECK_ERR( item );

			PlacementNew<UniqueID<DescriptorSetLayoutID>>( _dsLayouts.Get<1>() + i, std::move(item) );

			stackAlloc.Pop( bm );
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

		_pplnLayouts = { count, _allocator.Alloc<UniqueID<VPipelineLayoutID>>( count )};

		for (uint i = 0; i < count; ++i)
		{
			PipelineLayoutDesc	desc;
			CHECK_ERR( des( OUT desc ));

			VPipelineLayout::DescriptorSets_t	desc_sets;

			for (size_t j = 0; j < desc.descrSets.size(); ++j)
			{
				auto&	src = desc.descrSets[j];
				CHECK_ERR( size_t(src.second.uid) < _dsLayouts.Get<0>() );

				DescriptorSetLayoutID	id			= _dsLayouts.Get<1>()[ size_t(src.second.uid) ];
				auto*					ds_layout	= resMngr.GetResource( id );
				CHECK_ERR( ds_layout );

				desc_sets.insert_or_assign( src.first, VPipelineLayout::DescSetLayout{ id, ds_layout->Handle(), src.second.index });
			}

			auto	item = resMngr.CreatePipelineLayout( desc_sets, desc.pushConstants.items );
			CHECK_ERR( item );
			
			PlacementNew<UniqueID<VPipelineLayoutID>>( _pplnLayouts.Get<1>() + i, std::move(item) );
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
		
		_renderPassOutputs = { count, _allocator.Alloc<UniqueID<VRenderPassOutputID>>( count )};

		for (uint i = 0; i < count; ++i)
		{
			PipelineCompiler::RenderPassInfo	info;
			CHECK_ERR( des( OUT info ));

			VRenderPassOutput::Output_t		frag_outputs;
			CHECK_ERR( info.fragmentOutputs.size() <= frag_outputs.capacity() );

			for (auto& item : info.fragmentOutputs) {
				frag_outputs.insert( item );
			}

			auto	item = resMngr.CreateRenderPassOutput( frag_outputs );
			CHECK_ERR( item );

			PlacementNew<UniqueID<VRenderPassOutputID>>( _renderPassOutputs.Get<1>() + i, std::move(item) );
		}
		
		CHECK_ERR( des( OUT marker, OUT count ) and marker == EMarker::RenderPassNames );
		
		for (uint i = 0; i < count; ++i)
		{
			RenderPassName	name;
			uint			idx;
			CHECK_ERR( des( OUT name, OUT idx ));

			CHECK_ERR( idx < _renderPassOutputs.Get<0>() );

			EXLOCK( refs.renderPassNames.guard );
			CHECK( refs.renderPassNames.map.insert_or_assign( name, _renderPassOutputs.Get<1>()[idx] ).second );
		}

		return true;
	}

/*
=================================================
	_LoadSpirvShaders
=================================================
*/
	bool VPipelinePack::_LoadSpirvShaders (const VDevice &dev, Serializing::Deserializer &des, StackAllocator_t &stackAlloc)
	{
		EMarker	marker;
		uint	count	= 0;
		CHECK_ERR( des( OUT marker, OUT count ) and marker == EMarker::SpirvShaders );
		
		if ( not count )
			return true;

		_shaderModules = { count, _allocator.Alloc<VkShaderModule>( count ), _allocator.Alloc<SpecConstants_t*>( count )};

		for (uint i = 0; i < count; ++i)
		{
			auto	bm			= stackAlloc.Push();
			uint	code_size	= 0;
			auto&	module		= _shaderModules.Get<1>()[i];
			auto&	spec_ref	= _shaderModules.Get<2>()[i];

			SpecConstants_t	spec;
			CHECK_ERR( des( OUT spec, OUT code_size ));

			if ( spec.size() )
			{
				spec_ref = _allocator.Alloc<SpecConstants_t>();
				PlacementNew<SpecConstants_t>( &spec_ref, spec );
			}
			else
				spec_ref = null;

			uint*	code = stackAlloc.Alloc<uint>( code_size );
			CHECK_ERR( code );
			CHECK_ERR( des.stream->Read( code, code_size * SizeOf<uint> ));
			
			VkShaderModuleCreateInfo	shader_info = {};
			shader_info.sType		= VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			shader_info.codeSize	= code_size * sizeof(uint);
			shader_info.pCode		= code;
			
			VK_CHECK( dev.vkCreateShaderModule( dev.GetVkDevice(), &shader_info, null, OUT &module ));

			stackAlloc.Pop( bm );
		}
		return true;
	}
	
/*
=================================================
	_LoadGraphicsPipelines
=================================================
*/
	bool VPipelinePack::_LoadGraphicsPipelines (VResourceManager &resMngr, Serializing::Deserializer &des, StackAllocator_t &stackAlloc)
	{
		EMarker	marker;
		uint	count	= 0;
		CHECK_ERR( des( OUT marker, OUT count ) and marker == EMarker::GraphicsPipelines );
		
		if ( not count )
			return true;

		_gpipelines = { count, _allocator.Alloc<UniqueID<VGraphicsPipelineTemplateID>>( count )};
		
		PipelineCompiler::GraphicsPipelineDesc	desc;
		for (uint i = 0; i < count; ++i)
		{
			CHECK_ERR( desc.Deserialize( des ));
			CHECK_ERR( size_t(desc.layout) < _pplnLayouts.Get<0>() );
			CHECK_ERR( size_t(desc.renderPass) < _renderPassOutputs.Get<0>() );
			
			auto				bm			= stackAlloc.Push();
			auto *				modules		= stackAlloc.Alloc<ShaderModule>( desc.shaders.size() );
			VPipelineLayoutID	layout_id	= _pplnLayouts.Get<1>()[ size_t(desc.layout) ];
			VRenderPassOutputID	rp_output_id= _renderPassOutputs.Get<1>()[ size_t(desc.renderPass) ];

			for (size_t j = 0; j < desc.shaders.size(); ++j)
			{
				const uint	idx = uint(desc.shaders[j].second & ~PipelineCompiler::ShaderUID::SPIRV);
				CHECK_ERR( EnumEq( desc.shaders[j].second, PipelineCompiler::ShaderUID::SPIRV ));
				CHECK_ERR( idx < _shaderModules.Get<0>() );

				modules[j].stage	= VEnumCast( desc.shaders[j].first );
				modules[j].module	= _shaderModules.Get<1>()[idx];
				modules[j].spec		= _shaderModules.Get<2>()[idx];
			}

			auto	item = resMngr.CreateGPTemplate( layout_id, rp_output_id, desc, ArrayView{modules, desc.shaders.size()} );
			CHECK_ERR( item );
			
			PlacementNew<UniqueID<VGraphicsPipelineTemplateID>>( _gpipelines.Get<1>() + i, std::move(item) );

			stackAlloc.Pop( bm );
		}
		return true;
	}
	
/*
=================================================
	_LoadMeshPipelines
=================================================
*/
	bool VPipelinePack::_LoadMeshPipelines (VResourceManager &resMngr, Serializing::Deserializer &des, StackAllocator_t &stackAlloc)
	{
		EMarker	marker;
		uint	count	= 0;
		CHECK_ERR( des( OUT marker, OUT count ) and marker == EMarker::MeshPipelines );
		
		if ( not count )
			return true;
		
		_mpipelines = { count, _allocator.Alloc<UniqueID<VMeshPipelineTemplateID>>( count )};
		
		PipelineCompiler::MeshPipelineDesc	desc;
		for (uint i = 0; i < count; ++i)
		{
			CHECK_ERR( desc.Deserialize( des ));
			CHECK_ERR( size_t(desc.layout) < _pplnLayouts.Get<0>() );
			CHECK_ERR( size_t(desc.renderPass) < _renderPassOutputs.Get<0>() );
			
			auto				bm			= stackAlloc.Push();
			auto *				modules		= stackAlloc.Alloc<ShaderModule>( desc.shaders.size() );
			VPipelineLayoutID	layout_id	= _pplnLayouts.Get<1>()[ size_t(desc.layout) ];
			VRenderPassOutputID	rp_output_id= _renderPassOutputs.Get<1>()[ size_t(desc.renderPass) ];

			for (size_t j = 0; j < desc.shaders.size(); ++j)
			{
				const uint	idx = uint(desc.shaders[j].second & ~PipelineCompiler::ShaderUID::SPIRV);
				CHECK_ERR( EnumEq( desc.shaders[j].second, PipelineCompiler::ShaderUID::SPIRV ));
				CHECK_ERR( idx < _shaderModules.Get<0>() );

				modules[j].stage	= VEnumCast( desc.shaders[j].first );
				modules[j].module	= _shaderModules.Get<1>()[idx];
				modules[j].spec		= _shaderModules.Get<2>()[idx];
			}

			auto	item = resMngr.CreateMPTemplate( layout_id, rp_output_id, desc, ArrayView{modules, desc.shaders.size()} );
			CHECK_ERR( item );

			PlacementNew<UniqueID<VMeshPipelineTemplateID>>( _mpipelines.Get<1>() + i, std::move(item) );
			
			stackAlloc.Pop( bm );
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
		
		_cpipelines = { count, _allocator.Alloc<UniqueID<VComputePipelineTemplateID>>( count )};
		
		PipelineCompiler::ComputePipelineDesc	desc;
		for (uint i = 0; i < count; ++i)
		{
			CHECK_ERR( desc.Deserialize( des ));
			CHECK_ERR( size_t(desc.layout) < _pplnLayouts.Get<0>() );
			
			VPipelineLayoutID	id	= _pplnLayouts.Get<1>()[ size_t(desc.layout) ];
			const uint			idx	= uint(desc.shader & ~PipelineCompiler::ShaderUID::SPIRV);

			CHECK_ERR( EnumEq( desc.shader, PipelineCompiler::ShaderUID::SPIRV ));
			CHECK_ERR( idx < _shaderModules.Get<0>() );

			ShaderModule	module;
			module.stage	= VK_SHADER_STAGE_COMPUTE_BIT;
			module.module	= _shaderModules.Get<1>()[idx];
			module.spec		= _shaderModules.Get<2>()[idx];

			auto	item = resMngr.CreateCPTemplate( id, desc, module );
			CHECK_ERR( item );

			PlacementNew<UniqueID<VComputePipelineTemplateID>>( _cpipelines.Get<1>() + i, std::move(item) );
		}
		return true;
	}
	
/*
=================================================
	_LoadRayTracingPipelines
=================================================
*/
	bool VPipelinePack::_LoadRayTracingPipelines (VResourceManager &, Serializing::Deserializer &des)
	{
		EMarker	marker;
		uint	count	= 0;
		CHECK_ERR( des( OUT marker, OUT count ) and marker == EMarker::RayTracingPipelines );
		
		if ( not count )
			return true;
		
		return false;
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
					CHECK( refs.graphics.map.insert_or_assign( name, _gpipelines.Get<1>()[idx] ).second );
					break;
				}
				case PipelineUID::Mesh : {
					EXLOCK( refs.mesh.guard );
					CHECK( refs.mesh.map.insert_or_assign( name, _mpipelines.Get<1>()[idx] ).second );
					break;
				}
				case PipelineUID::Compute : {
					EXLOCK( refs.compute.guard );
					CHECK( refs.compute.map.insert_or_assign( name, _cpipelines.Get<1>()[idx] ).second );
					break;
				}
				case PipelineUID::RayTracing :
					//EXLOCK( refs.rayTracing.guard );
					//CHECK( refs.rayTracing.map.insert_or_assign( name, _rtpipelines.Get<1>()[idx] ).second );
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
