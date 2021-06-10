// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef AE_ENABLE_VULKAN

# include "graphics/Public/DescriptorSet.h"
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
		using Uniform			= PipelineCompiler::DescriptorSetLayoutDesc::Uniform;
		using Uniforms_t		= Tuple< uint, UniformName const*, Uniform const* >;

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
		using DynamicDataPtr	= DescriptorSet::DynamicDataPtr;

		struct DescriptorBinding
		{
			Array< VkDescriptorSetLayoutBinding >	desc;
			uint									dbCount	= 0;
		};


	// variables
	private:
		VkDescriptorSetLayout	_layout				= VK_NULL_HANDLE;
		uint					_maxIndex			= 0;
		DynamicDataPtr			_dsTemplate;
		
		DebugName_t				_debugName;
		RWDataRaceCheck			_drCheck;


	// methods
	public:
		VDescriptorSetLayout () {}
		~VDescriptorSetLayout ();

		bool  Create (const VDevice &dev, StringView dbgName);
		bool  Create (const VDevice &dev, const Uniforms_t &uniforms, ArrayView<VkSampler> samplerStorage, StringView dbgName);
		void  Destroy (const VResourceManagerImpl &);

		ND_ VkDescriptorSetLayout	Handle ()					const	{ SHAREDLOCK( _drCheck );  return _layout; }
		ND_ uint					GetMaxIndex ()				const	{ SHAREDLOCK( _drCheck );  return _maxIndex; }
		ND_ StringView				GetDebugName ()				const	{ SHAREDLOCK( _drCheck );  return _debugName; }
		ND_ DynamicDataPtr const&	GetDescriptorTemplate ()	const	{ SHAREDLOCK( _drCheck );  return _dsTemplate; }


	private:
		void  _IncDescriptorCount (VkDescriptorType type);
		bool  _AddUniform (const Uniform &un, ArrayView<VkSampler> samplerStorage, INOUT DescriptorBinding &binding);
		void  _AddBuffer (const Buffer &buf, uint bindingIndex, uint arraySize, VkDescriptorType descrType, INOUT DescriptorBinding &binding);
		void  _AddImage (const Image &img, uint bindingIndex, uint arraySize, VkDescriptorType descrType, INOUT DescriptorBinding &binding);
		void  _AddTexelBuffer (const TexelBuffer &buf, uint bindingIndex, uint arraySize, VkDescriptorType descrType, INOUT DescriptorBinding &binding);
		void  _AddCombinedImage (const Image &tex, uint bindingIndex, uint arraySize, INOUT DescriptorBinding &binding);
		void  _AddSampler (const Sampler &samp, uint bindingIndex, uint arraySize, INOUT DescriptorBinding &binding);
		void  _AddRayTracingScene (const RayTracingScene &rts, uint bindingIndex, uint arraySize, INOUT DescriptorBinding &binding);
		void  _AddCombinedImageWithImmutableSampler (const ImageWithSampler &tex, uint bindingIndex, uint arraySize,
													 ArrayView<VkSampler> samplerStorage, INOUT DescriptorBinding &binding);
		void  _AddImmutableSampler (const ImmutableSampler &samp, uint bindingIndex, uint arraySize,
									ArrayView<VkSampler> samplerStorage, INOUT DescriptorBinding &binding);
	};

}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
