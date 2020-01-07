// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "PipelinePack.h"

namespace AE::PipelineCompiler
{

/*
=================================================
	operator == (PushConst)
=================================================
*/
	inline bool  operator == (const PushConstants::PushConst &lhs, const PushConstants::PushConst &rhs)
	{
		return	(lhs.stageFlags	== rhs.stageFlags)	&
				(lhs.offset		== rhs.offset)		&
				(lhs.size		== rhs.size);
	}
	
/*
=================================================
	operator == (PushConstants)
=================================================
*/
	inline bool  operator == (const PushConstants &lhs, const PushConstants &rhs)
	{
		return lhs.items == rhs.items;
	}

/*
=================================================
	operator == (FragmentOutput)
=================================================
*/
	inline bool  operator == (const GraphicsPipelineDesc::FragmentOutput &lhs, const GraphicsPipelineDesc::FragmentOutput &rhs)
	{
		return	(lhs.index	== rhs.index)	&
				(lhs.type	== rhs.type);
	}

/*
=================================================
	operator == (VertexAttrib)
=================================================
*/
	inline bool  operator == (const VertexInputState::VertexAttrib &lhs, const VertexInputState::VertexAttrib &rhs)
	{
		return	(lhs.id		== rhs.id)		&
				(lhs.index	== rhs.index)	&
				(lhs.type	== rhs.type);
	}

/*
=================================================
	operator == (DescSetLayout)
=================================================
*/
	inline bool  operator == (const PipelineLayoutDesc::DescSetLayout &lhs, const PipelineLayoutDesc::DescSetLayout &rhs)
	{
		return	(lhs.index	== rhs.index)	&
				(lhs.uid	== rhs.uid);
	}
//-----------------------------------------------------------------------------



/*
=================================================
	ImageHash
=================================================
*/
	ND_ HashVal  ImageHash (const DescriptorSetLayoutDesc::Image &img)
	{
		return HashOf( img.state ) + HashOf( img.type );
	}

/*
=================================================
	DescriptorSetLayoutHash
=================================================
*/
	ND_ HashVal  DescriptorSetLayoutHash (const DescriptorSetLayoutDesc &desc)
	{
		using EDescriptorType = DescriptorSetLayoutDesc::EDescriptorType;

		HashVal	res = HashOf( desc.uniforms.size() );

		for (auto&[name, un] : desc.uniforms)
		{
			res << HashOf( name ) << HashOf( un.type )
				<< HashOf( un.index ) << HashOf( un.arraySize );
			
			BEGIN_ENUM_CHECKS();
			switch ( un.type )
			{
				case EDescriptorType::UniformBuffer :
				case EDescriptorType::StorageBuffer :
					res << HashOf( un.buffer.state ) << HashOf( un.buffer.dynamicOffsetIndex ) << HashOf( un.buffer.staticSize ) << HashOf( un.buffer.arrayStride );
					break;

				case EDescriptorType::UniformTexelBuffer :
				case EDescriptorType::StorageTexelBuffer :
					res << HashOf( un.texelBuffer.state );
					break;

				case EDescriptorType::StorageImage :
					res << ImageHash( un.storageImage );
					break;

				case EDescriptorType::SampledImage :
					res << ImageHash( un.sampledImage );
					break;

				case EDescriptorType::CombinedImage :
					res << ImageHash( un.combinedImage );
					break;

				case EDescriptorType::CombinedImage_ImmutableSampler :
					CHECK_ERR( un.combinedImageWithSampler.samplerOffsetInStorage + un.arraySize <= desc.samplerStorage.size() );
					res << ImageHash( un.combinedImageWithSampler.image );

					for (size_t i = 0; i < un.arraySize; ++i) {
						res << HashOf( desc.samplerStorage[ un.combinedImageWithSampler.samplerOffsetInStorage + i ] );
					}
					break;

				case EDescriptorType::SubpassInput :
					res << ImageHash( un.subpassInput );
					break;

				case EDescriptorType::Sampler :
					res << HashOf( un.sampler.stageFlags );
					break;

				case EDescriptorType::ImmutableSampler :
					CHECK_ERR( un.immutableSampler.offsetInStorage + un.arraySize <= desc.samplerStorage.size() );
					
					for (size_t i = 0; i < un.arraySize; ++i) {
						res << HashOf( desc.samplerStorage[ un.immutableSampler.offsetInStorage + i ] );
					}
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

		return res;
	}

/*
=================================================
	PushConstantsHash
=================================================
*/
	ND_ HashVal  PushConstantsHash (const PushConstants &desc)
	{
		HashVal	res = HashOf( desc.items.size() );

		for (auto& pc : desc.items)
		{
			res << HashOf( pc.first ) << HashOf( pc.second.stageFlags ) << HashOf( pc.second.offset ) << HashOf( pc.second.size );
		}
		return res;
	}
	
/*
=================================================
	PipelineLayoutDesc_DSLayoutHash
=================================================
*/
	ND_ HashVal  PipelineLayoutDesc_DSLayoutHash (const PipelineLayoutDesc::DescSetLayout &desc)
	{
		return HashOf( desc.index ) + HashOf( desc.uid );
	}
	
/*
=================================================
	PipelineLayoutHash
=================================================
*/
	ND_ HashVal  PipelineLayoutHash (const PipelineLayoutDesc &desc)
	{
		HashVal	res = HashOf( desc.descrSets.size() );

		for (auto& ds : desc.descrSets) {
			res << HashOf( ds.first ) << HashOf( ds.second.index ) << HashOf( ds.second.uid );
		}

		res << PushConstantsHash( desc.pushConstants );
		return res;
	}
	
/*
=================================================
	FragmentOutputHash
=================================================
*/
	ND_ HashVal  FragmentOutputHash (const GraphicsPipelineDesc::FragmentOutputs_t &fragOutputs)
	{
		HashVal	res = HashOf( fragOutputs.size() );

		for (auto& fo : fragOutputs)
		{
			res << HashOf( fo.first ) << HashOf( fo.second.index ) << HashOf( fo.second.type );
		}
		return res;
	}
	
/*
=================================================
	VertexAttribsHash
=================================================
*/
	ND_ HashVal  VertexAttribsHash (const GraphicsPipelineDesc::VertexAttribs_t &attribs)
	{
		HashVal	res = HashOf( attribs.size() );

		for (auto& attr : attribs)
		{
			res << HashOf( attr.id ) << HashOf( attr.index ) << HashOf( attr.type );
		}
		return res;
	}

/*
=================================================
	GraphicsPipelineHash
=================================================
*/
	ND_ HashVal  GraphicsPipelineHash (const GraphicsPipelineDesc &desc)
	{
		HashVal	res;
		res << HashOf( desc.layout );
		res << HashOf( desc.shaders );
		res << HashOf( desc.supportedTopology );
		res << FragmentOutputHash( desc.fragmentOutputs );
		res << VertexAttribsHash( desc.vertexAttribs );
		res << HashOf( desc.patchControlPoints );
		res << HashOf( desc.earlyFragmentTests );
		return res;
	}

/*
=================================================
	MeshPipelineHash
=================================================
*/
	ND_ HashVal  MeshPipelineHash (const MeshPipelineDesc &desc)
	{
		HashVal	res;
		res << HashOf( desc.layout );
		res << HashOf( desc.shaders );
		res << FragmentOutputHash( desc.fragmentOutputs );
		res << HashOf( desc.topology );
		res << HashOf( desc.maxVertices );
		res << HashOf( desc.maxIndices );
		res << HashOf( desc.defaultTaskGroupSize );
		res << HashOf( desc.taskSizeSpec );
		res << HashOf( desc.defaultMeshGroupSize );
		res << HashOf( desc.meshSizeSpec );
		res << HashOf( desc.earlyFragmentTests );
		return res;
	}

/*
=================================================
	ComputePipelineHash
=================================================
*/
	ND_ HashVal  ComputePipelineHash (const ComputePipelineDesc &desc)
	{
		HashVal	res;
		res << HashOf( desc.layout );
		res << HashOf( desc.shader );
		res << HashOf( desc.defaultLocalGroupSize );
		res << HashOf( desc.localSizeSpec );
		return res;
	}

/*
=================================================
	SpirvShaderCodeHash
=================================================
*/
	ND_ HashVal  SpirvShaderCodeHash (const SpirvShaderCode &desc)
	{
		return HashOf( desc.code ) + HashOf( desc.spec );
	}
//-----------------------------------------------------------------------------


	
/*
=================================================
	Equal (DescriptorSetLayoutDesc::Image)
=================================================
*/
	ND_ bool  Equal (const DescriptorSetLayoutDesc::Image &lhs, const DescriptorSetLayoutDesc::Image &rhs)
	{
		return	(lhs.state	== rhs.state)	&
				(lhs.type	== rhs.type);
	}

/*
=================================================
	Equal (DescriptorSetLayoutDesc)
=================================================
*/
	ND_ bool  Equal (const DescriptorSetLayoutDesc &lhs, const DescriptorSetLayoutDesc &rhs)
	{
		using EDescriptorType = DescriptorSetLayoutDesc::EDescriptorType;

		if ( lhs.uniforms.size()		!= rhs.uniforms.size()		or
			 lhs.samplerStorage.size()	!= rhs.samplerStorage.size() )
		{
			return false;
		}

		for (size_t i = 0; i < lhs.uniforms.size(); ++i)
		{
			auto&	l_pair	= lhs.uniforms[i];
			auto&	r_pair	= rhs.uniforms[i];
			auto&	l_un	= l_pair.second;
			auto&	r_un	= r_pair.second;

			if ( l_pair.first	!= r_pair.first	or
				 l_un.type		!= r_un.type	or
				 l_un.index		!= r_un.index	or
				 l_un.arraySize	!= r_un.arraySize )
			{
				return false;
			}

			BEGIN_ENUM_CHECKS();
			switch ( l_un.type )
			{
				case EDescriptorType::UniformBuffer :
				case EDescriptorType::StorageBuffer :
					if ( l_un.buffer.state			!= r_un.buffer.state				or
						 //l_un.buffer.dynamicOffsetIndex	!= r_un.buffer.dynamicOffsetIndex	or
						 l_un.buffer.staticSize		!= r_un.buffer.staticSize			or
						 l_un.buffer.arrayStride	!= r_un.buffer.arrayStride )
					{
						return false;
					}
					break;
					
				case EDescriptorType::UniformTexelBuffer :
				case EDescriptorType::StorageTexelBuffer :
					if ( l_un.texelBuffer.state != r_un.texelBuffer.state )
						return false;
					break;

				case EDescriptorType::StorageImage :
					if ( not Equal( l_un.storageImage, r_un.storageImage ))
						return false;
					break;

				case EDescriptorType::SampledImage :
					if ( not Equal( l_un.sampledImage, r_un.sampledImage ))
						return false;
					break;

				case EDescriptorType::CombinedImage :
					if ( not Equal( l_un.combinedImage, r_un.combinedImage ))
						return false;
					break;

				case EDescriptorType::CombinedImage_ImmutableSampler :
					if ( not Equal( l_un.combinedImageWithSampler.image, r_un.combinedImageWithSampler.image ))
						return false;
					
					CHECK_ERR( l_un.combinedImageWithSampler.samplerOffsetInStorage + l_un.arraySize <= lhs.samplerStorage.size() );
					CHECK_ERR( r_un.combinedImageWithSampler.samplerOffsetInStorage + r_un.arraySize <= rhs.samplerStorage.size() );

					for (size_t j = 0; j < l_un.arraySize; ++j) {
						if ( lhs.samplerStorage[ l_un.combinedImageWithSampler.samplerOffsetInStorage + j ] != rhs.samplerStorage[ r_un.combinedImageWithSampler.samplerOffsetInStorage + j ] )
							return false;
					}
					break;

				case EDescriptorType::SubpassInput :
					if ( not Equal( l_un.subpassInput, r_un.subpassInput ))
						return false;
					break;

				case EDescriptorType::Sampler :
					break;

				case EDescriptorType::ImmutableSampler :
					CHECK_ERR( l_un.immutableSampler.offsetInStorage + l_un.arraySize <= lhs.samplerStorage.size() );
					CHECK_ERR( r_un.immutableSampler.offsetInStorage + r_un.arraySize <= rhs.samplerStorage.size() );

					for (size_t j = 0; j < l_un.arraySize; ++j) {
						if ( lhs.samplerStorage[ l_un.immutableSampler.offsetInStorage + j ] != rhs.samplerStorage[ r_un.immutableSampler.offsetInStorage + j ] )
							return false;
					}
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

		return true;
	}

/*
=================================================
	Equal (PipelineLayoutDesc)
=================================================
*/
	ND_ bool  Equal (const PipelineLayoutDesc &lhs, const PipelineLayoutDesc &rhs)
	{
		return	(lhs.descrSets		== rhs.descrSets)	&
				(lhs.pushConstants	== rhs.pushConstants);
	}

/*
=================================================
	Equal (VertexAttribs)
=================================================
*/
	ND_ bool  Equal (const GraphicsPipelineDesc::VertexAttribs_t &lhs, const GraphicsPipelineDesc::VertexAttribs_t &rhs)
	{
		if ( lhs.size() != rhs.size() )
			return false;

		for (size_t i = 0; i < lhs.size(); ++i)
		{
			if ( not (lhs[i] == rhs[i]) )
				return false;
		}
		return true;
	}
	
/*
=================================================
	Equal (GraphicsPipelineDesc)
=================================================
*/
	ND_ bool  Equal (const GraphicsPipelineDesc &lhs, const GraphicsPipelineDesc &rhs)
	{
		return	lhs.layout				== rhs.layout				and
				lhs.shaders				== rhs.shaders				and
				lhs.supportedTopology	== rhs.supportedTopology	and
				lhs.fragmentOutputs		== rhs.fragmentOutputs		and
				Equal( lhs.vertexAttribs, rhs.vertexAttribs )		and
				lhs.patchControlPoints	== rhs.patchControlPoints	and
				lhs.earlyFragmentTests	== rhs.earlyFragmentTests;
	}
	
/*
=================================================
	Equal (MeshPipelineDesc)
=================================================
*/
	ND_ bool  Equal (const MeshPipelineDesc &lhs, const MeshPipelineDesc &rhs)
	{
		return	lhs.layout						== rhs.layout					and
				lhs.shaders						== rhs.shaders					and
				lhs.topology					== rhs.topology					and
				lhs.fragmentOutputs				== rhs.fragmentOutputs			and
				lhs.maxVertices					== rhs.maxVertices				and
				lhs.maxIndices					== rhs.maxIndices				and
				All( lhs.defaultTaskGroupSize	== rhs.defaultTaskGroupSize )	and
				All( lhs.taskSizeSpec			== rhs.taskSizeSpec )			and
				All( lhs.defaultMeshGroupSize	== rhs.defaultMeshGroupSize )	and
				All( lhs.meshSizeSpec			== rhs.meshSizeSpec )			and
				lhs.earlyFragmentTests			== rhs.earlyFragmentTests;
	}

/*
=================================================
	Equal (ComputePipelineDesc)
=================================================
*/
	ND_ bool  Equal (const ComputePipelineDesc &lhs, const ComputePipelineDesc &rhs)
	{
		return	lhs.layout						== rhs.layout				and
				lhs.shader						== rhs.shader				and
				All( lhs.localSizeSpec			== rhs.localSizeSpec )		and
				All( lhs.defaultLocalGroupSize	== rhs.defaultLocalGroupSize );
	}

/*
=================================================
	Equal (SpirvShaderCode)
=================================================
*/
	ND_ bool  Equal (const SpirvShaderCode &lhs, const SpirvShaderCode &rhs)
	{
		return (lhs.code == rhs.code) & (lhs.spec == rhs.spec);
	}

}	// AE::PipelineCompiler
