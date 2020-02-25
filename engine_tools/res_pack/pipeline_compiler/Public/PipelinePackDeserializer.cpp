// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "PipelinePack.h"
#include "serializing/Deserializer.h"
#include "serializing/ObjectFactory.h"

namespace AE::PipelineCompiler
{
/*
=================================================
	Deserialize
=================================================
*/
	bool Deserialize_Uniform (Serializing::Deserializer const& des, uint samplerStorageSize, OUT DescriptorSetLayoutDesc::Uniform &un)
	{
		using EDescriptorType = DescriptorSetLayoutDesc::EDescriptorType;
		
		const auto	Deserialize_Image = [&des] (OUT DescriptorSetLayoutDesc::Image &img)
		{
			return des( OUT img.state, OUT img.type );
		};

		bool	result = true;
		result &= des( OUT un.type, OUT un.index, OUT un.arraySize );

		BEGIN_ENUM_CHECKS();
		switch ( un.type )
		{
			case EDescriptorType::UniformBuffer :
			case EDescriptorType::StorageBuffer :
				result &= des( OUT un.buffer.state, OUT un.buffer.dynamicOffsetIndex, OUT un.buffer.staticSize, OUT un.buffer.arrayStride );
				break;
					
			case EDescriptorType::UniformTexelBuffer :
			case EDescriptorType::StorageTexelBuffer :
				result &= des( OUT un.texelBuffer.state );
				break;

			case EDescriptorType::StorageImage :
				result &= Deserialize_Image( OUT un.storageImage );
				break;

			case EDescriptorType::SampledImage :
				result &= Deserialize_Image( OUT un.sampledImage );
				break;

			case EDescriptorType::CombinedImage :
				result &= Deserialize_Image( OUT un.combinedImage );
				break;

			case EDescriptorType::CombinedImage_ImmutableSampler :
				result &= Deserialize_Image( OUT un.combinedImageWithSampler.image ) and des( OUT un.combinedImageWithSampler.samplerOffsetInStorage );
				CHECK_ERR( un.combinedImageWithSampler.samplerOffsetInStorage + un.arraySize <= samplerStorageSize );
				break;

			case EDescriptorType::SubpassInput :
				result &= Deserialize_Image( OUT un.subpassInput );
				break;

			case EDescriptorType::Sampler :
				result &= des( OUT un.sampler.stageFlags );
				break;

			case EDescriptorType::ImmutableSampler :
				result &= des( OUT un.immutableSampler.offsetInStorage );
				CHECK_ERR( un.immutableSampler.offsetInStorage + un.arraySize <= samplerStorageSize );
				break;

			case EDescriptorType::RayTracingScene :
				// TODO
				//break;

			case EDescriptorType::Unknown :
			default :
				RETURN_ERR( "unknown descriptor type" );
		}
		END_ENUM_CHECKS();

		return true;
	}

/*
=================================================
	Deserialize
=================================================
*/
	bool DescriptorSetLayoutDesc::Deserialize (Serializing::Deserializer const& des)
	{
		uniforms.clear();
		samplerStorage.clear();

		bool	result		= true;
		uint	un_count	= 0;
		uint	samp_count	= 0;
		CHECK_ERR( des( OUT un_count, OUT samp_count ));

		if ( samp_count )
		{
			samplerStorage.resize( samp_count );

			for (auto& id : samplerStorage) {
				des( OUT id );
			}
		}

		uniforms.resize( un_count );
		
		for (auto&[name, un] : uniforms)
		{
			result &= des( OUT name );
		}

		for (auto&[name, un] : uniforms)
		{
			result &= Deserialize_Uniform( des, samp_count, OUT un );

			if ( not result )
				break;
		}

		return true;
	}
//-----------------------------------------------------------------------------


	
/*
=================================================
	Deserialize
=================================================
*/
	bool PushConstants::Deserialize (Serializing::Deserializer const& des)
	{
		bool	result	= true;
		uint	count	= 0;

		items.clear();

		result &= des( OUT count );

		for (uint i = 0; i < count; ++i)
		{
			PushConstantName	name;
			PushConst			data;

			result &= des( OUT name, OUT data.stageFlags, OUT data.offset, OUT data.size );
		}
		return result;
	}
//-----------------------------------------------------------------------------



/*
=================================================
	Deserialize
=================================================
*/
	bool PipelineLayoutDesc::Deserialize (Serializing::Deserializer const& des)
	{
		return des( OUT descrSets, OUT pushConstants );
	}
//-----------------------------------------------------------------------------


	
/*
=================================================
	Deserialize_VertexAttribs
=================================================
*/
namespace {
	static bool  Deserialize_VertexAttribs (Serializing::Deserializer const& des, OUT GraphicsPipelineDesc::VertexAttribs_t &vertexAttribs)
	{
		uint	count	= 0;
		bool	result	= true;

		result &= des( OUT count );
		CHECK_ERR( result and count <= vertexAttribs.capacity() );

		vertexAttribs.resize( count );

		for (auto& attr : vertexAttribs)
		{
			result &= des( OUT attr.id, OUT attr.index, OUT attr.type );
		}
		return result;
	}
}	// namespace

/*
=================================================
	Deserialize
=================================================
*/
	bool GraphicsPipelineDesc::Deserialize (Serializing::Deserializer const& des)
	{
		bool	result = des( OUT layout, OUT renderPass, OUT shaders, OUT supportedTopology, OUT patchControlPoints, OUT specialization, OUT earlyFragmentTests );
		result &= Deserialize_VertexAttribs( des, OUT vertexAttribs );
		return result;
	}
//-----------------------------------------------------------------------------



/*
=================================================
	Deserialize
=================================================
*/
	bool ComputePipelineDesc::Deserialize (Serializing::Deserializer const& des)
	{
		return des( OUT layout, OUT shader, OUT defaultLocalGroupSize, OUT localSizeSpec, OUT specialization );
	}
//-----------------------------------------------------------------------------
	


/*
=================================================
	Deserialize
=================================================
*/
	bool MeshPipelineDesc::Deserialize (Serializing::Deserializer const& des)
	{
		bool	result = true;
		result &= des( OUT layout, OUT renderPass, OUT shaders, OUT topology, OUT maxVertices, OUT maxIndices, OUT specialization );
		result &= des( OUT defaultTaskGroupSize, OUT taskSizeSpec, OUT defaultMeshGroupSize, OUT meshSizeSpec, OUT earlyFragmentTests );
		return result;
	}
//-----------------------------------------------------------------------------
	


/*
=================================================
	Deserialize
=================================================
*/
	bool SpirvShaderCode::Deserialize (Serializing::Deserializer const& des)
	{
		return des( OUT spec, OUT code );
	}
//-----------------------------------------------------------------------------
	


/*
=================================================
	Deserialize
=================================================
*/
	bool RenderPassInfo::Deserialize (Serializing::Deserializer const& des)
	{
		uint	count	= 0;
		bool	result	= true;
		
		result &= des( OUT count );
		CHECK_ERR( result and count <= fragmentOutputs.capacity() );

		for (uint i = 0; i < count; ++i)
		{
			RenderTargetName	name;
			FragmentOutput		frag;

			result &= des( OUT name, OUT frag.index, OUT frag.type );

			fragmentOutputs.insert_or_assign( name, frag );
		}
		return result;
	}
//-----------------------------------------------------------------------------
	

# ifndef AE_BUILD_PIPELINE_COMPILER
	
	bool DescriptorSetLayoutDesc::Serialize (Serializing::Serializer &) const
	{
		return false;
	}

	bool PushConstants::Serialize (Serializing::Serializer &) const
	{
		return false;
	}

	bool PipelineLayoutDesc::Serialize (Serializing::Serializer &) const
	{
		return false;
	}

	bool GraphicsPipelineDesc::Serialize (Serializing::Serializer &) const
	{
		return false;
	}

	bool ComputePipelineDesc::Serialize (Serializing::Serializer &) const
	{
		return false;
	}

	bool MeshPipelineDesc::Serialize (Serializing::Serializer &) const
	{
		return false;
	}

	bool SpirvShaderCode::Serialize (Serializing::Serializer &) const
	{
		return false;
	}

	bool RenderPassInfo::Serialize (Serializing::Serializer &) const
	{
		return false;
	}
//-----------------------------------------------------------------------------
# endif
}	// AE::PipelineCompiler
