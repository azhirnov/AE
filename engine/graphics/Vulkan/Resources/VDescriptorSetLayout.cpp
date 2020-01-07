// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#ifdef AE_ENABLE_VULKAN

# include "graphics/Vulkan/Resources/VDescriptorSetLayout.h"
# include "graphics/Vulkan/VResourceManager.h"
# include "graphics/Vulkan/VEnumCast.h"

namespace AE::Graphics
{	

/*
=================================================
	destructor
=================================================
*/
	VDescriptorSetLayout::~VDescriptorSetLayout ()
	{
		CHECK( not _layout );
	}
	
/*
=================================================
	Create
=================================================
*/
	bool VDescriptorSetLayout::Create (const VDevice &dev, StringView dbgName)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( not _layout );
		
		VkDescriptorSetLayoutCreateInfo	descriptor_info = {};
		descriptor_info.sType			= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptor_info.pBindings		= null;
		descriptor_info.bindingCount	= 0;
		VK_CHECK( dev.vkCreateDescriptorSetLayout( dev.GetVkDevice(), &descriptor_info, null, OUT &_layout ));

		if ( dbgName.size() )
			dev.SetObjectName( uint64_t(_layout), dbgName, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT );

		return true;
	}

/*
=================================================
	Create
=================================================
*/
	bool VDescriptorSetLayout::Create (const VDevice &dev, const Uniforms_t &uniforms, ArrayView<VkSampler> samplerStorage, StringView dbgName)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( not _layout );

		_uniforms	= uniforms;
		_debugName	= dbgName;

		DescriptorBinding_t	binding;

		for (uint i = 0; i < _uniforms.Get<0>(); ++i)
		{
			auto&	name	= _uniforms.Get<1>()[i];
			auto&	un		= _uniforms.Get<2>()[i];

			CHECK_ERR( name.IsDefined() );
			CHECK_ERR( _AddUniform( un, samplerStorage, INOUT binding ));
		}

		VkDescriptorSetLayoutCreateInfo	descriptor_info = {};
		descriptor_info.sType			= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptor_info.pBindings		= binding.data();
		descriptor_info.bindingCount	= uint(binding.size());
		VK_CHECK( dev.vkCreateDescriptorSetLayout( dev.GetVkDevice(), &descriptor_info, null, OUT &_layout ));

		if ( dbgName.size() )
			dev.SetObjectName( uint64_t(_layout), dbgName, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT );

		return true;
	}
	
/*
=================================================
	Destroy
=================================================
*/
	void VDescriptorSetLayout::Destroy (const VDevice &dev)
	{
		EXLOCK( _drCheck );

		if ( _layout ) {
			dev.vkDestroyDescriptorSetLayout( dev.GetVkDevice(), _layout, null );
		}

		_poolSize.clear();
		_debugName.clear();

		_uniforms			= Default;
		_layout				= VK_NULL_HANDLE;
		_dynamicOffsetCount	= 0;
		_maxIndex			= 0;
	}

/*
=================================================
	_AddUniform
=================================================
*/
	bool VDescriptorSetLayout::_AddUniform (const Uniform &un, ArrayView<VkSampler> samplerStorage, INOUT DescriptorBinding_t &binding)
	{
		CHECK_ERR( un.index != UMax );
		CHECK_ERR( un.arraySize > 0 );

		BEGIN_ENUM_CHECKS();
		switch ( un.type )
		{
			case EDescriptorType::UniformBuffer :
				_AddBuffer( un.buffer, un.index, un.arraySize, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, INOUT binding );
				break;

			case EDescriptorType::StorageBuffer :
				_AddBuffer( un.buffer, un.index, un.arraySize, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, INOUT binding );
				break;

			case EDescriptorType::UniformTexelBuffer :
				_AddTexelBuffer( un.texelBuffer, un.index, un.arraySize, VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, INOUT binding );
				break;

			case EDescriptorType::StorageTexelBuffer :
				_AddTexelBuffer( un.texelBuffer, un.index, un.arraySize, VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, INOUT binding );
				break;

			case EDescriptorType::StorageImage :
				_AddImage( un.storageImage, un.index, un.arraySize, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, INOUT binding );
				break;

			case EDescriptorType::SubpassInput :
				_AddImage( un.subpassInput, un.index, un.arraySize, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, INOUT binding );
				break;

			case EDescriptorType::SampledImage :
				_AddImage( un.sampledImage, un.index, un.arraySize, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, INOUT binding );
				break;

			case EDescriptorType::CombinedImage :
				_AddCombinedImage( un.combinedImage, un.index, un.arraySize, INOUT binding );
				break;

			case EDescriptorType::Sampler :	
				_AddSampler( un.sampler, un.index, un.arraySize, INOUT binding );
				break;

			case EDescriptorType::ImmutableSampler :
				_AddImmutableSampler( un.immutableSampler, un.index, un.arraySize, samplerStorage, INOUT binding );
				break;

			case EDescriptorType::CombinedImage_ImmutableSampler :
				_AddCombinedImageWithImmutableSampler( un.combinedImageWithSampler, un.index, un.arraySize, samplerStorage, INOUT binding );
				break;

			case EDescriptorType::RayTracingScene :
			case EDescriptorType::Unknown :
			default :
				RETURN_ERR( "unsupported descriptor type" );
		}
		END_ENUM_CHECKS();
		return true;
	}
	
/*
=================================================
	_IncDescriptorCount
=================================================
*/
	void VDescriptorSetLayout::_IncDescriptorCount (VkDescriptorType type)
	{
		for (auto& size : _poolSize)
		{
			if ( size.type == type )
			{
				++size.descriptorCount;
				return;
			}
		}

		_poolSize.emplace_back( type, 1u );
	}
	
/*
=================================================
	_AddBuffer
=================================================
*/
	void VDescriptorSetLayout::_AddBuffer (const Buffer &buf, uint bindingIndex, uint arraySize, VkDescriptorType descrType, INOUT DescriptorBinding_t &binding)
	{
		const bool	is_dynamic	= EnumEq( buf.state, EResourceState::_BufferDynamicOffset );
		ASSERT( is_dynamic == (buf.dynamicOffsetIndex != UMax) );

		if ( is_dynamic ) {
			descrType = 
				descrType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC :
				descrType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ? VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC : VK_DESCRIPTOR_TYPE_MAX_ENUM;
		}

		VkDescriptorSetLayoutBinding	bind = {};
		bind.descriptorType		= descrType;
		bind.stageFlags			= EResourceState_ToPipelineStages( buf.state );
		bind.binding			= bindingIndex;
		bind.descriptorCount	= arraySize;
		
		_dynamicOffsetCount		+= (is_dynamic ? arraySize : 0);
		_maxIndex				 = Max( _maxIndex, bind.binding );
		_IncDescriptorCount( bind.descriptorType );

		binding.push_back( std::move(bind) );
	}

/*
=================================================
	_AddImage
=================================================
*/
	void VDescriptorSetLayout::_AddImage (const Image &img, uint bindingIndex, uint arraySize, VkDescriptorType descrType, INOUT DescriptorBinding_t &binding)
	{
		VkDescriptorSetLayoutBinding	bind = {};
		bind.descriptorType		= descrType;
		bind.stageFlags			= EResourceState_ToPipelineStages( img.state );
		bind.binding			= bindingIndex;
		bind.descriptorCount	= arraySize;

		_maxIndex = Max( _maxIndex, bind.binding );
		_IncDescriptorCount( bind.descriptorType );

		binding.push_back( std::move(bind) );
	}
	
/*
=================================================
	_AddTexelBuffer
=================================================
*/
	void VDescriptorSetLayout::_AddTexelBuffer (const TexelBuffer &buf, uint bindingIndex, uint arraySize, VkDescriptorType descrType, INOUT DescriptorBinding_t &binding)
	{
		VkDescriptorSetLayoutBinding	bind = {};
		bind.descriptorType		= descrType;
		bind.stageFlags			= EResourceState_ToPipelineStages( buf.state );
		bind.binding			= bindingIndex;
		bind.descriptorCount	= arraySize;

		_maxIndex = Max( _maxIndex, bind.binding );
		_IncDescriptorCount( bind.descriptorType );

		binding.push_back( std::move(bind) );
	}

/*
=================================================
	_AddCombinedImage
=================================================
*/
	void VDescriptorSetLayout::_AddCombinedImage (const Image &tex, uint bindingIndex, uint arraySize, INOUT DescriptorBinding_t &binding)
	{
		ASSERT( EnumEq( tex.state, EResourceState::_Access_ShaderSample ));

		VkDescriptorSetLayoutBinding	bind = {};
		bind.descriptorType		= VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		bind.stageFlags			= EResourceState_ToPipelineStages( tex.state );
		bind.binding			= bindingIndex;
		bind.descriptorCount	= arraySize;
		
		_maxIndex = Max( _maxIndex, bind.binding );
		_IncDescriptorCount( bind.descriptorType );

		binding.push_back( std::move(bind) );
	}
	
/*
=================================================
	_AddSampler
=================================================
*/
	void VDescriptorSetLayout::_AddSampler (const Sampler &sampler, uint bindingIndex, uint arraySize, INOUT DescriptorBinding_t &binding)
	{
		VkDescriptorSetLayoutBinding	bind = {};
		bind.descriptorType		= VK_DESCRIPTOR_TYPE_SAMPLER;
		bind.stageFlags			= VEnumCast( sampler.stageFlags );
		bind.binding			= bindingIndex;
		bind.descriptorCount	= arraySize;
		
		_maxIndex = Max( _maxIndex, bind.binding );
		_IncDescriptorCount( bind.descriptorType );

		binding.push_back( std::move(bind) );
	}
	
/*
=================================================
	_AddCombinedImageWithImmutableSampler
=================================================
*/
	void VDescriptorSetLayout::_AddCombinedImageWithImmutableSampler (const ImageWithSampler &tex, uint bindingIndex, uint arraySize,
																	  ArrayView<VkSampler> samplerStorage, INOUT DescriptorBinding_t &binding)
	{
		VkDescriptorSetLayoutBinding	bind = {};
		bind.descriptorType		= VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		bind.stageFlags			= EResourceState_ToPipelineStages( tex.image.state );
		bind.binding			= bindingIndex;
		bind.descriptorCount	= arraySize;
		bind.pImmutableSamplers	= samplerStorage.data() + tex.samplerOffsetInStorage;
		
		_maxIndex = Max( _maxIndex, bind.binding );
		_IncDescriptorCount( bind.descriptorType );

		binding.push_back( std::move(bind) );
	}
	
/*
=================================================
	_AddImmutableSampler
=================================================
*/
	void VDescriptorSetLayout::_AddImmutableSampler (const ImmutableSampler &sampler, uint bindingIndex, uint arraySize,
													 ArrayView<VkSampler> samplerStorage, INOUT DescriptorBinding_t &binding)
	{
		VkDescriptorSetLayoutBinding	bind = {};
		bind.descriptorType		= VK_DESCRIPTOR_TYPE_SAMPLER;
		bind.stageFlags			= VEnumCast( sampler.stageFlags );
		bind.binding			= bindingIndex;
		bind.descriptorCount	= arraySize;
		bind.pImmutableSamplers	= samplerStorage.data() + sampler.offsetInStorage;
		
		_maxIndex = Max( _maxIndex, bind.binding );
		_IncDescriptorCount( bind.descriptorType );

		binding.push_back( std::move(bind) );
	}

/*
=================================================
	_AddRayTracingScene
=================================================
*
	void VDescriptorSetLayout::_AddRayTracingScene (const PipelineDescription::RayTracingScene &rts, uint bindingIndex,
													uint arraySize, EShaderStages stageFlags, INOUT DescriptorBinding_t &binding)
	{
		// calculate hash
		_hash << HashOf( rts.state )
			  << HashOf( arraySize );
		
		arraySize = (arraySize ? arraySize : VConfig::MaxElementsInUnsizedDesc);
		_elementCount += arraySize;

		// add binding
		VkDescriptorSetLayoutBinding	bind = {};
		bind.descriptorType		= VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV;
		bind.stageFlags			= VEnumCast( stageFlags );
		bind.binding			= bindingIndex;
		bind.descriptorCount	= arraySize;

		_maxIndex = Max( _maxIndex, bind.binding );
		_IncDescriptorCount( bind.descriptorType );

		binding.push_back( std::move(bind) );
	}
	*/
	

}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
