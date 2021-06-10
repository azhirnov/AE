// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "PipelinePack.h"
#include "PipelinePackHash.h"
#include "serializing/Serializer.h"
#include "serializing/ObjectFactory.h"
#include "stl/Algorithms/StringUtils.h"

namespace AE::PipelineCompiler
{

/*
=================================================
	Serialize
=================================================
*/
	bool DescriptorSetLayoutDesc::Serialize (Serializing::Serializer& ser) const
	{
		bool	result = true;
		
		result &= ser( uint(uniforms.size()) );
		result &= ser( uint(samplerStorage.size()) );

		for (auto& id : samplerStorage) {
			result &= ser( id );
		}

		const auto	Serialize_Image = [&ser] (const DescriptorSetLayoutDesc::Image &img)
		{
			return ser( img.state, img.type );
		};
		
		for (auto&[name, un] : uniforms)
		{
			result &= ser( name );
		}

		for (auto&[name, un] : uniforms)
		{
			result &= ser( un.type, un.index, un.arraySize );

			BEGIN_ENUM_CHECKS();
			switch ( un.type )
			{
				case EDescriptorType::UniformBuffer :
				case EDescriptorType::StorageBuffer :
					result &= ser( un.buffer.state, un.buffer.dynamicOffsetIndex, un.buffer.staticSize, un.buffer.arrayStride );
					break;
					
				case EDescriptorType::UniformTexelBuffer :
				case EDescriptorType::StorageTexelBuffer :
					result &= ser( un.texelBuffer.state );
					break;

				case EDescriptorType::StorageImage :
					result &= Serialize_Image( un.storageImage );
					break;

				case EDescriptorType::SampledImage :
					result &= Serialize_Image( un.sampledImage );
					break;

				case EDescriptorType::CombinedImage :
					result &= Serialize_Image( un.combinedImage );
					break;

				case EDescriptorType::CombinedImage_ImmutableSampler :
					CHECK_ERR( un.combinedImageWithSampler.samplerOffsetInStorage + un.arraySize <= samplerStorage.size() );
					result &= Serialize_Image( un.combinedImageWithSampler.image );
					result &= ser( un.combinedImageWithSampler.samplerOffsetInStorage );
					break;

				case EDescriptorType::SubpassInput :
					result &= Serialize_Image( un.subpassInput );
					break;

				case EDescriptorType::Sampler :
					result &= ser( un.sampler.stageFlags );
					break;

				case EDescriptorType::ImmutableSampler :
					CHECK_ERR( un.immutableSampler.offsetInStorage + un.arraySize <= samplerStorage.size() );
					result &= ser( un.immutableSampler.offsetInStorage );
					break;

				case EDescriptorType::RayTracingScene :
					// TODO
					//break;

				case EDescriptorType::Unknown :
				default :
					RETURN_ERR( "unknown descriptor type" );
			}
			END_ENUM_CHECKS();
		}
		return result;
	}
	
/*
=================================================
	Merge
=================================================
*/
	bool DescriptorSetLayoutDesc::Merge (const DescriptorSetLayoutDesc &other, OUT usize &merged)
	{
		merged = 0;

		auto	src = other.uniforms.begin();
		auto	dst = uniforms.begin();

		for (; dst != uniforms.end() and src != other.uniforms.end();)
		{
			if ( dst->first > src->first )
			{
				++src;
				continue;
			}

			if ( dst->first < src->first )
			{
				++dst;
				continue;
			}

			if ( dst->first == src->first )
			{
				auto&	dst_un	= dst->second;
				auto&	src_un	= src->second;

				CHECK_ERR( dst_un.index		== src_un.index );
				CHECK_ERR( dst_un.type		== src_un.type );
				CHECK_ERR( dst_un.arraySize	== src_un.arraySize );	// TODO: unsized arrays

				BEGIN_ENUM_CHECKS();
				switch ( dst_un.type )
				{
					case EDescriptorType::UniformBuffer :
					case EDescriptorType::StorageBuffer :
					{
						bool	dst_has_dyn_idx = (dst_un.buffer.dynamicOffsetIndex != UMax);
						bool	src_has_dyn_idx = (src_un.buffer.dynamicOffsetIndex != UMax);

						CHECK_ERR( dst_has_dyn_idx				== src_has_dyn_idx );
						CHECK_ERR( dst_un.buffer.staticSize		== src_un.buffer.staticSize );
						CHECK_ERR( dst_un.buffer.arrayStride	== src_un.buffer.arrayStride );
						CHECK_ERR( AllBits( dst_un.buffer.state, src_un.buffer.state, ~EResourceState::_ShaderMask ));

						merged += (dst_un.buffer.state != src_un.buffer.state);

						dst_un.buffer.state |= (src_un.buffer.state & EResourceState::_ShaderMask);
						break;
					}
					case EDescriptorType::UniformTexelBuffer :
					case EDescriptorType::StorageTexelBuffer :
					{
						CHECK_ERR( AllBits( dst_un.texelBuffer.state, src_un.texelBuffer.state, ~EResourceState::_ShaderMask ));

						merged += (dst_un.texelBuffer.state != src_un.texelBuffer.state);

						dst_un.texelBuffer.state |= (src_un.texelBuffer.state & EResourceState::_ShaderMask);
						break;
					}
					case EDescriptorType::StorageImage :
					{
						CHECK_ERR( AllBits( dst_un.storageImage.state, src_un.storageImage.state, ~EResourceState::_ShaderMask ));
						CHECK_ERR( dst_un.storageImage.type == src_un.storageImage.type );
						
						merged += (dst_un.storageImage.state != src_un.storageImage.state);

						dst_un.storageImage.state |= (src_un.storageImage.state & EResourceState::_ShaderMask);
						break;
					}
					case EDescriptorType::SampledImage :
					{
						CHECK_ERR( AllBits( dst_un.sampledImage.state, src_un.sampledImage.state, ~EResourceState::_ShaderMask ));
						CHECK_ERR( dst_un.sampledImage.type == src_un.sampledImage.type );
						
						merged += (dst_un.sampledImage.state != src_un.sampledImage.state);

						dst_un.sampledImage.state |= (src_un.sampledImage.state & EResourceState::_ShaderMask);
						break;
					}
					case EDescriptorType::CombinedImage :
					{
						CHECK_ERR( AllBits( dst_un.combinedImage.state, src_un.combinedImage.state, ~EResourceState::_ShaderMask ));
						CHECK_ERR( dst_un.combinedImage.type == src_un.combinedImage.type );
						
						merged += (dst_un.combinedImage.state != src_un.combinedImage.state);

						dst_un.combinedImage.state |= (src_un.combinedImage.state & EResourceState::_ShaderMask);
						break;
					}
					case EDescriptorType::CombinedImage_ImmutableSampler :
					{
						CHECK_ERR( dst_un.combinedImageWithSampler.samplerOffsetInStorage + dst_un.arraySize <= samplerStorage.size() );
						CHECK_ERR( src_un.combinedImageWithSampler.samplerOffsetInStorage + src_un.arraySize <= other.samplerStorage.size() );
						
						CHECK_ERR( AllBits( dst_un.combinedImageWithSampler.image.state, src_un.combinedImageWithSampler.image.state, ~EResourceState::_ShaderMask ));
						CHECK_ERR( dst_un.combinedImageWithSampler.image.type == src_un.combinedImageWithSampler.image.type );
						
						merged += (dst_un.combinedImageWithSampler.image.state != src_un.combinedImageWithSampler.image.state);

						dst_un.combinedImageWithSampler.image.state |= (src_un.combinedImageWithSampler.image.state & EResourceState::_ShaderMask);

						// compare samplers
						for (usize i = 0; i < dst_un.arraySize; ++i)
						{
							auto&	dst_samp = samplerStorage[ dst_un.combinedImageWithSampler.samplerOffsetInStorage + i ];
							auto&	src_samp = other.samplerStorage[ src_un.combinedImageWithSampler.samplerOffsetInStorage + i ];
							CHECK_ERR( dst_samp == src_samp );
						}
						break;
					}
					case EDescriptorType::SubpassInput :
					{
						CHECK_ERR( AllBits( dst_un.subpassInput.state, src_un.subpassInput.state, ~EResourceState::_ShaderMask ));
						CHECK_ERR( dst_un.subpassInput.type == src_un.subpassInput.type );
						
						merged += (dst_un.subpassInput.state != src_un.subpassInput.state);

						dst_un.subpassInput.state |= (src_un.subpassInput.state & EResourceState::_ShaderMask);
						break;
					}
					case EDescriptorType::Sampler :
					{
						dst_un.sampler.stageFlags |= src_un.sampler.stageFlags;
						break;
					}
					case EDescriptorType::ImmutableSampler :
					{
						for (usize i = 0; i < dst_un.arraySize; ++i)
						{
							auto&	dst_samp = samplerStorage[ dst_un.immutableSampler.offsetInStorage + i ];
							auto&	src_samp = other.samplerStorage[ src_un.immutableSampler.offsetInStorage + i ];
							CHECK_ERR( dst_samp == src_samp );
						}
						break;
					}
					case EDescriptorType::RayTracingScene :
						// TODO
						//break;

					case EDescriptorType::Unknown :
					default :
						RETURN_ERR( "unknown descriptor type" );
				}
				END_ENUM_CHECKS();

				++src;
				++dst;
			}
		}

		for (; src != other.uniforms.end(); ++src)
		{
			++merged;

			auto&	dst_un = uniforms.emplace_back( *src ).second;
			auto&	src_un = src->second;

			if ( src_un.type == EDescriptorType::CombinedImage_ImmutableSampler )
			{
				dst_un.combinedImageWithSampler.samplerOffsetInStorage = samplerStorage.size();

				for (usize i = 0; i < src_un.arraySize; ++i) {
					samplerStorage.push_back( other.samplerStorage[ src_un.combinedImageWithSampler.samplerOffsetInStorage + i ]);
				}
			}

			if ( src_un.type == EDescriptorType::ImmutableSampler )
			{
				dst_un.immutableSampler.offsetInStorage = samplerStorage.size();

				for (usize i = 0; i < src_un.arraySize; ++i) {
					samplerStorage.push_back( other.samplerStorage[ src_un.immutableSampler.offsetInStorage + i ]);
				}
			}
		}

		SortUniforms();
		return true;
	}
	
/*
=================================================
	SortUniforms
=================================================
*/
	void DescriptorSetLayoutDesc::SortUniforms ()
	{
		std::sort( uniforms.begin(), uniforms.end(),
				   [](auto& lhs, auto& rhs) { return lhs.first < rhs.first; });
	}
//-----------------------------------------------------------------------------


	
/*
=================================================
	Merge
=================================================
*/
	bool PushConstants::Merge (const PushConstants &other)
	{
		for (auto& src : other.items)
		{
			auto	dst = items.find( src.first );

			if ( dst != items.end() )
			{
				CHECK_ERR( dst->second.offset	== src.second.offset );
				CHECK_ERR( dst->second.size		== src.second.size );
				
				dst->second.stageFlags |= src.second.stageFlags;
			}
			else
			{
				CHECK( items.insert( src ).second );
			}
		}
		return true;
	}

/*
=================================================
	Serialize
=================================================
*/
	bool PushConstants::Serialize (Serializing::Serializer& ser) const
	{
		bool	result = true;

		result &= ser( uint(items.size()) );
		for (auto& pc : items) {
			result &= ser( pc.first, pc.second.stageFlags, pc.second.offset, pc.second.size );
		}

		return result;
	}
//-----------------------------------------------------------------------------
	


/*
=================================================
	Serialize
=================================================
*/
	bool PipelineLayoutDesc::Serialize (Serializing::Serializer& ser) const
	{
		return ser( descrSets, pushConstants );
	}
//-----------------------------------------------------------------------------

	

/*
=================================================
	Serialize_VertexAttribs
=================================================
*/
namespace {
	static bool  Serialize_VertexAttribs (Serializing::Serializer& ser, const GraphicsPipelineDesc::VertexAttribs_t &vertexAttribs)
	{
		bool	result = ser( uint(vertexAttribs.size()) );

		for (auto& attr : vertexAttribs)
		{
			result &= ser( attr.id, attr.index, attr.type );
		}
		return result;
	}
}	// namespace

/*
=================================================
	Serialize
=================================================
*/
	bool GraphicsPipelineDesc::Serialize (Serializing::Serializer& ser) const
	{
		bool	result = ser( layout, renderPass, shaders, supportedTopology, patchControlPoints, specialization, earlyFragmentTests );
		result &= Serialize_VertexAttribs( ser, vertexAttribs );
		return result;
	}
//-----------------------------------------------------------------------------
	

/*
=================================================
	Serialize
=================================================
*/
	bool MeshPipelineDesc::Serialize (Serializing::Serializer& ser) const
	{
		bool	result = true;
		result &= ser( layout, renderPass, shaders, topology, maxVertices, maxIndices, specialization );
		result &= ser( defaultTaskGroupSize, taskSizeSpec, defaultMeshGroupSize, meshSizeSpec, earlyFragmentTests );
		return result;
	}
//-----------------------------------------------------------------------------
	

/*
=================================================
	Serialize
=================================================
*/
	bool ComputePipelineDesc::Serialize (Serializing::Serializer& ser) const
	{
		return ser( layout, shader, defaultLocalGroupSize, localSizeSpec, specialization );
	}
//-----------------------------------------------------------------------------


/*
=================================================
	Serialize
=================================================
*/
	bool SpirvShaderCode::Serialize (Serializing::Serializer &ser) const
	{
		return ser( version, spec, code );
	}
//-----------------------------------------------------------------------------
	
	
/*
=================================================
	Serialize
=================================================
*/	
	bool RenderPassInfo::Serialize (Serializing::Serializer &ser) const
	{
		bool	result = true;
		result &= ser( uint(fragmentOutputs.size()) );

		for (auto& frag : fragmentOutputs)
		{
			result &= ser( frag.first, frag.second.index, frag.second.type );
		}
		return result;
	}
//-----------------------------------------------------------------------------

	
/*
=================================================
	constructor
=================================================
*/
	PipelineStorage::PipelineStorage ()
	{
		_dsLayouts.reserve( 128 );
		_dsLayoutMap.reserve( 128 );

		_pplnLayouts.reserve( 128 );
		_pplnLayoutMap.reserve( 128 );

		_graphicsPipelines.reserve( 128 );
		_gpipelineMap.reserve( 128 );

		_meshPipelines.reserve( 128 );
		_mpipelineMap.reserve( 128 );

		_computePipelines.reserve( 128 );
		_cpipelineMap.reserve( 128 );

		_spirvShaders.reserve( 128 );
		_spirvShaderMap.reserve( 128 );
	}
	
/*
=================================================
	UpdateBufferDynamicOffsets
=================================================
*/
	static void  UpdateBufferDynamicOffsets (INOUT DescriptorSetLayoutDesc &desc)
	{
		using EDescriptorType = DescriptorSet::EDescriptorType;
		
		FixedArray< DescriptorSetLayoutDesc::Uniform *, 128 >	sorted;

		for (auto& un : desc.uniforms)
		{
			switch ( un.second.type )
			{
				case EDescriptorType::UniformBuffer :
				case EDescriptorType::StorageBuffer :
					if ( un.second.buffer.dynamicOffsetIndex != UMax )
					{
						CHECK( sorted.try_push_back( &un.second ));
					}
					break;
			}
		}
		
		std::sort( sorted.begin(), sorted.end(), [](auto& lhs, auto& rhs) { return lhs->index < rhs->index; });
		
		uint	index = 0;
		for (auto* un : sorted)
		{
			un->buffer.dynamicOffsetIndex = index++;
		}
	}

/*
=================================================
	AddDescriptorSetLayout
=================================================
*/
	DescrSetUID  PipelineStorage::AddDescriptorSetLayout (DescriptorSetLayoutDesc &&desc)
	{
		std::sort( desc.uniforms.begin(), desc.uniforms.end(),
				   [](auto& lhs, auto& rhs) { return lhs.first < rhs.first; });

		UpdateBufferDynamicOffsets( INOUT desc );

		const usize	hash	= usize(DescriptorSetLayoutHash( desc ));
		auto			iter	= _dsLayoutMap.find( hash );

		for (; iter != _dsLayoutMap.end() and iter->first == hash; ++iter)
		{
			auto&	lhs = _dsLayouts[ usize(iter->second) ];

			if ( Equal( lhs, desc ))
				return iter->second;
		}

		auto	uid = DescrSetUID(_dsLayouts.size());

		_dsLayoutMap.insert({ hash, uid });
		_dsLayouts.push_back( std::move(desc) );

		return uid;
	}
	
/*
=================================================
	AddPipelineLayout
=================================================
*/
	PipelineLayoutUID  PipelineStorage::AddPipelineLayout (PipelineLayoutDesc &&desc)
	{
		const usize	hash	= usize(PipelineLayoutHash( desc ));
		auto			iter	= _pplnLayoutMap.find( hash );

		for (; iter != _pplnLayoutMap.end() and iter->first == hash; ++iter)
		{
			auto&	lhs = _pplnLayouts[ usize(iter->second) ];

			if ( Equal( lhs, desc ))
				return iter->second;
		}

		auto	uid = PipelineLayoutUID(_pplnLayouts.size());

		_pplnLayoutMap.insert({ hash, uid });
		_pplnLayouts.push_back( std::move(desc) );

		return uid;
	}
	
/*
=================================================
	AddPipeline
=================================================
*/
	PipelineUID  PipelineStorage::AddPipeline (const PipelineName &name, GraphicsPipelineDesc &&desc)
	{
		CHECK_ERR( name.IsDefined() );

		const usize	hash	= usize(GraphicsPipelineHash( desc ));
		auto			iter	= _gpipelineMap.find( hash );

		for (; iter != _gpipelineMap.end() and iter->first == hash; ++iter)
		{
			auto&	lhs = _graphicsPipelines[ usize(iter->second) & usize(~PipelineUID::_Mask) ];

			if ( Equal( lhs, desc ))
			{
				CHECK( _pipelineMap.insert_or_assign( name, iter->second ).second );
				return iter->second;
			}
		}

		auto	uid = PipelineUID(_graphicsPipelines.size() | uint(PipelineUID::Graphics));

		_gpipelineMap.insert({ hash, uid });
		_graphicsPipelines.push_back( std::move(desc) );
		
		CHECK( _pipelineMap.insert_or_assign( name, uid ).second );

		return uid;
	}
		
/*
=================================================
	AddPipeline
=================================================
*/
	PipelineUID  PipelineStorage::AddPipeline (const PipelineName &name, MeshPipelineDesc &&desc)
	{
		CHECK_ERR( name.IsDefined() );

		const usize	hash	= usize(MeshPipelineHash( desc ));
		auto			iter	= _mpipelineMap.find( hash );

		for (; iter != _mpipelineMap.end() and iter->first == hash; ++iter)
		{
			auto&	lhs = _meshPipelines[ usize(iter->second) & usize(~PipelineUID::_Mask) ];
			
			if ( Equal( lhs, desc ))
			{
				CHECK( _pipelineMap.insert_or_assign( name, iter->second ).second );
				return iter->second;
			}
		}

		auto	uid = PipelineUID(_meshPipelines.size() | uint(PipelineUID::Mesh));

		_mpipelineMap.insert({ hash, uid });
		_meshPipelines.push_back( std::move(desc) );

		CHECK( _pipelineMap.insert_or_assign( name, uid ).second );

		return uid;
	}

/*
=================================================
	AddPipeline
=================================================
*/
	PipelineUID  PipelineStorage::AddPipeline (const PipelineName &name, ComputePipelineDesc &&desc)
	{
		CHECK_ERR( name.IsDefined() );

		const usize	hash	= usize(ComputePipelineHash( desc ));
		auto			iter	= _cpipelineMap.find( hash );

		for (; iter != _cpipelineMap.end() and iter->first == hash; ++iter)
		{
			auto&	lhs = _computePipelines[ usize(iter->second) & usize(~PipelineUID::_Mask) ];
			
			if ( Equal( lhs, desc ))
			{
				CHECK( _pipelineMap.insert_or_assign( name, iter->second ).second );
				return iter->second;
			}
		}

		auto	uid = PipelineUID(_computePipelines.size() | uint(PipelineUID::Compute));

		_cpipelineMap.insert({ hash, uid });
		_computePipelines.push_back( std::move(desc) );
		
		CHECK( _pipelineMap.insert_or_assign( name, uid ).second );

		return uid;
	}
	
/*
=================================================
	AddShader
=================================================
*/
	ShaderUID  PipelineStorage::AddShader (uint version, Array<uint> &&spirv, const SpecConstants_t &spec)
	{
		SpirvShaderCode	code	{ version, std::move(spirv), spec };
		const usize	hash	= usize(SpirvShaderCodeHash( code ));
		auto			iter	= _spirvShaderMap.find( hash );
		
		for (; iter != _spirvShaderMap.end() and iter->first == hash; ++iter)
		{
			auto&	lhs = _spirvShaders[ usize(iter->second) & ~uint(ShaderUID::_Mask) ];

			if ( Equal( lhs, code ))
				return iter->second;
		}
		
		auto	uid = ShaderUID(_spirvShaders.size() | uint(ShaderUID::SPIRV));

		_spirvShaderMap.insert({ hash, uid });
		_spirvShaders.push_back( std::move(code) );

		return uid;
	}
	
/*
=================================================
	AddRenderPass
=================================================
*/
	RenderPassUID  PipelineStorage::AddRenderPass (const RenderPassName &name, const RenderPassInfo &info)
	{
		for (usize i = 0; i < _renderPasses.size(); ++i)
		{
			if ( _renderPasses[i].fragmentOutputs == info.fragmentOutputs )
			{
				if ( name.IsDefined() )
				{
					auto[iter, inserted] = _renderPassMap.insert({ name, RenderPassUID(i) });
					if ( not inserted )
						CHECK( iter->second == RenderPassUID(i) );
				}
				return RenderPassUID(i);
			}
		}

		RenderPassName	new_name = name;

		if ( new_name.GetName().empty() )
		{
			// make unique name
			for (usize i = 0; i < 1000; ++i)
			{
				new_name = RenderPassName{ "RP_"s << ToString(i) };

				if ( not _renderPassMap.count( new_name ))
					break;
			}
		}

		const auto	uid = RenderPassUID(_renderPasses.size());

		_renderPasses.push_back( info );

		CHECK( _renderPassMap.insert_or_assign( new_name, uid ).second );
		return uid;
	}

/*
=================================================
	Serialize
=================================================
*/
	bool  PipelineStorage::Serialize (Serializing::Serializer& ser) const
	{
		bool	result = true;

		result &= ser( Version );
		// TODO: write buffer size

		result &= ser( uint(EMarker::DescrSetLayouts), _dsLayouts );
		ASSERT( result );
		AE_LOGI( "Serialized descriptor set layouts: "s << ToString(_dsLayouts.size()) );

		result &= ser( uint(EMarker::PipelineLayouts), _pplnLayouts );
		ASSERT( result );
		AE_LOGI( "Serialized pipeline layouts: "s << ToString(_pplnLayouts.size()) );

		result &= ser( uint(EMarker::RenderPasses), _renderPasses );
		{
			Array<Pair< RenderPassName, RenderPassUID >>	rp_names;
			rp_names.reserve( _renderPassMap.size() );

			for (auto& item : _renderPassMap) {
				rp_names.push_back( item );
			}

			std::sort( rp_names.begin(), rp_names.end(), [](auto& lhs, auto& rhs) { return lhs.first < rhs.first; });

			result &= ser( uint(EMarker::RenderPassNames), rp_names );
		}
		ASSERT( result );
		AE_LOGI( "Serialized render passes: "s << ToString(_renderPassMap.size()) );

		result &= ser( uint(EMarker::SpirvShaders), _spirvShaders );
		ASSERT( result );
		AE_LOGI( "Serialized SPIRV shaders: "s << ToString(_spirvShaders.size()) );

		result &= ser( uint(EMarker::GraphicsPipelines), _graphicsPipelines );
		ASSERT( result );
		AE_LOGI( "Serialized graphics pipelines: "s << ToString(_graphicsPipelines.size()) );
		
		result &= ser( uint(EMarker::MeshPipelines), _meshPipelines );
		ASSERT( result );
		AE_LOGI( "Serialized mesh pipelines: "s << ToString(_meshPipelines.size()) );

		result &= ser( uint(EMarker::ComputePipelines), _computePipelines );
		ASSERT( result );
		AE_LOGI( "Serialized compute pipelines: "s << ToString(_computePipelines.size()) );

		// temp
		result &= ser( uint(EMarker::RayTracingPipelines), uint(0) );
		ASSERT( result );

		{
			Array<Pair< PipelineName, PipelineUID >>	ppln_names;
			ppln_names.reserve( _pipelineMap.size() );

			for (auto& item : _pipelineMap) {
				ppln_names.push_back( item );
			}

			std::sort( ppln_names.begin(), ppln_names.end(), [](auto& lhs, auto& rhs) { return lhs.first < rhs.first; });

			result &= ser( uint(EMarker::PipelineNames), ppln_names );
			ASSERT( result );
		}

		return result;
	}
	

}	// AE::PipelineCompiler
