// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef AE_ENABLE_VULKAN

# include "graphics/Vulkan/VCommon.h"
# include "pipeline_compiler/Public/PipelinePack.h"

namespace AE::Graphics
{

	//
	// Vulkan Descriptor Set Layout
	//

	class VDescriptorSetLayout final
	{
	// types
	public:
		using DescriptorSet			= Pair< VkDescriptorSet, /*pool index*/uint8_t >;
		
		using Uniform				= PipelineCompiler::DescriptorSetLayoutDesc::Uniform;
		using Uniforms_t			= Tuple< uint, UniformName const*, Uniform const* >;

	private:
		using Image				= PipelineCompiler::DescriptorSetLayoutDesc::Image;
		using Buffer			= PipelineCompiler::DescriptorSetLayoutDesc::Buffer;
		using TexelBuffer		= PipelineCompiler::DescriptorSetLayoutDesc::TexelBuffer;
		using Sampler			= PipelineCompiler::DescriptorSetLayoutDesc::Sampler;
		using ImmutableSampler	= PipelineCompiler::DescriptorSetLayoutDesc::ImmutableSampler;
		using ImageWithSampler	= PipelineCompiler::DescriptorSetLayoutDesc::ImageWithSampler;
		using SubpassInput		= PipelineCompiler::DescriptorSetLayoutDesc::SubpassInput;
		using RayTracingScene	= PipelineCompiler::DescriptorSetLayoutDesc::RayTracingScene;
		using EDescriptorType	= PipelineCompiler::DescriptorSetLayoutDesc::EDescriptorType;

		using DescriptorBinding_t	= Array< VkDescriptorSetLayoutBinding >;
		using PoolSizeArray_t		= FixedArray< VkDescriptorPoolSize, 10 >;
		//using DynamicDataPtr		= PipelineResources::DynamicDataPtr;
		using DescSetCache_t		= FixedArray< DescriptorSet, 32 >;


	// variables
	private:
		VkDescriptorSetLayout	_layout				= VK_NULL_HANDLE;
		Uniforms_t				_uniforms;
		PoolSizeArray_t			_poolSize;
		uint					_dynamicOffsetCount	= 0;
		uint					_maxIndex			= 0;
		
		DebugName_t				_debugName;
		RWDataRaceCheck			_drCheck;


	// methods
	public:
		VDescriptorSetLayout () {}
		~VDescriptorSetLayout ();

		bool Create (const VDevice &dev, StringView dbgName);
		bool Create (const VDevice &dev, const Uniforms_t &uniforms, ArrayView<VkSampler> samplerStorage, StringView dbgName);
		void Destroy (const VDevice &dev);

		bool AllocDescriptorSet (VResourceManager &, OUT DescriptorSet &) const;
		void ReleaseDescriptorSet (VResourceManager &, const DescriptorSet &) const;

		ND_ VkDescriptorSetLayout	Handle ()			const	{ SHAREDLOCK( _drCheck );  return _layout; }
		//ND_ UniformMapPtr const&	GetUniforms ()		const	{ SHAREDLOCK( _drCheck );  return _uniforms; }
		//ND_ DynamicDataPtr const&	GetResources ()		const	{ SHAREDLOCK( _drCheck );  return _resourcesTemplate; }
		ND_ uint					GetMaxIndex ()		const	{ SHAREDLOCK( _drCheck );  return _maxIndex; }
		ND_ StringView				GetDebugName ()		const	{ SHAREDLOCK( _drCheck );  return _debugName; }


	private:
		void _IncDescriptorCount (VkDescriptorType type);
		bool _AddUniform (const Uniform &un, ArrayView<VkSampler> samplerStorage, INOUT DescriptorBinding_t &binding);
		void _AddBuffer (const Buffer &buf, uint bindingIndex, uint arraySize, VkDescriptorType descrType, INOUT DescriptorBinding_t &binding);
		void _AddImage (const Image &img, uint bindingIndex, uint arraySize, VkDescriptorType descrType, INOUT DescriptorBinding_t &binding);
		void _AddTexelBuffer (const TexelBuffer &buf, uint bindingIndex, uint arraySize, VkDescriptorType descrType, INOUT DescriptorBinding_t &binding);
		void _AddCombinedImage (const Image &tex, uint bindingIndex, uint arraySize, INOUT DescriptorBinding_t &binding);
		void _AddSampler (const Sampler &samp, uint bindingIndex, uint arraySize, INOUT DescriptorBinding_t &binding);
		void _AddRayTracingScene (const RayTracingScene &rts, uint bindingIndex, uint arraySize, INOUT DescriptorBinding_t &binding);
		void _AddCombinedImageWithImmutableSampler (const ImageWithSampler &tex, uint bindingIndex, uint arraySize,
													ArrayView<VkSampler> samplerStorage, INOUT DescriptorBinding_t &binding);
		void _AddImmutableSampler (const ImmutableSampler &samp, uint bindingIndex, uint arraySize,
								   ArrayView<VkSampler> samplerStorage, INOUT DescriptorBinding_t &binding);
	};

}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
