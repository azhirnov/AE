// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef AE_ENABLE_VULKAN

# include "graphics/Vulkan/Resources/VDescriptorSetLayout.h"
# include "pipeline_compiler/Public/PipelinePack.h"

namespace AE::Graphics
{

	//
	// Vulkan Pipeline Layout
	//

	class VPipelineLayout final
	{
	// types
	public:
		struct DescSetLayout
		{
			VDescriptorSetLayoutID		layoutId;
			VkDescriptorSetLayout		layout		= VK_NULL_HANDLE;	// TODO: remove?
			uint						index		= 0;
		};

		using DescriptorSets_t		= FixedMap< DescriptorSetName, DescSetLayout, GraphicsConfig::MaxDescriptorSets >;
		using PushConst				= PipelineCompiler::PushConstants::PushConst;
		using PushConstants_t		= PipelineCompiler::PushConstants::PushConstMap_t;


	// variables
	private:
		VkPipelineLayout		_layout			= VK_NULL_HANDLE;
		PushConstants_t			_pushConstants;
		DescriptorSets_t		_descriptorSets;
		uint					_firstDescSet	= UMax;
		DebugName_t				_debugName;
		
		RWDataRaceCheck			_drCheck;

		
	// methods
	public:
		VPipelineLayout () {}
		~VPipelineLayout ();
		
		bool Create (const VDevice &dev, const DescriptorSets_t &descSetLayouts, const PushConstants_t &pusConstants,
					 VkDescriptorSetLayout emptyLayout, StringView dbgName);
		void Destroy (const VResourceManager &);
		
		bool GetDescriptorSetLayout (const DescriptorSetName &id, OUT VDescriptorSetLayoutID &layout, OUT uint &binding) const;

		ND_ VkPipelineLayout		Handle ()					const	{ SHAREDLOCK( _drCheck );  return _layout; }
		
		ND_ uint					GetFirstDescriptorSet ()	const	{ SHAREDLOCK( _drCheck );  return _firstDescSet; }
		ND_ DescriptorSets_t const&	GetDescriptorSets ()		const	{ SHAREDLOCK( _drCheck );  return _descriptorSets; }
		
		ND_ StringView				GetDebugName ()				const	{ SHAREDLOCK( _drCheck );  return _debugName; }
		ND_ PushConstants_t const&	GetPushConstants ()			const	{ SHAREDLOCK( _drCheck );  return _pushConstants; }
	};

}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
