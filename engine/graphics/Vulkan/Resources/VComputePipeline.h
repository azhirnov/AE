// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef AE_ENABLE_VULKAN

# include "graphics/Vulkan/Resources/VGraphicsPipeline.h"

namespace AE::Graphics
{

	//
	// Vulkan Compute Pipeline
	//

	class VComputePipeline final
	{
	// types
	public:
		using SpecValues_t	= ComputePipelineDesc::SpecValues_t;


	// variables
	private:
		VkPipeline						_handle			= VK_NULL_HANDLE;
		VkPipelineLayout				_layout			= VK_NULL_HANDLE;

		VComputePipelineTemplateID		_templ;
		ComputePipelineDesc				_desc;
		
		RWDataRaceCheck					_drCheck;


	// methods
	public:
		VComputePipeline () {}
		~VComputePipeline ();

		bool  Create (VComputePipelineTemplateID templId, const ComputePipelineDesc &desc, VkPipeline ppln, VkPipelineLayout layout);
		void  Destroy (VResourceManagerImpl &);

		ND_ ComputePipelineDesc const&	Description ()	const	{ SHAREDLOCK( _drCheck );  return _desc; }
		ND_ VkPipeline					Handle ()		const	{ SHAREDLOCK( _drCheck );  return _handle; }
		ND_ VkPipelineLayout			Layout ()		const	{ SHAREDLOCK( _drCheck );  return _layout; }
		ND_ VComputePipelineTemplateID	TemplateID ()	const	{ SHAREDLOCK( _drCheck );  return _templ; }
	};


	
	//
	// Vulkan Compute Pipeline Template
	//

	class VComputePipelineTemplate final
	{
		friend class VResourceManagerImpl;
		
	// types
	public:
		using ShaderModule		= VGraphicsPipelineTemplate::ShaderModule;
		using SpecValues_t		= ComputePipelineDesc::SpecValues_t;
		using PipelineID_t		= ComputePipelineID;
		using PipelineMap_t		= HashMultiMap< HashVal, PipelineID_t >;		// TODO: use flat map

		static constexpr uint	UNDEFINED_SPECIALIZATION = UMax;


	// variables
	private:
		mutable SharedMutex		_pipelineMapGuard;
		mutable PipelineMap_t	_pipelineMap;

		VPipelineLayoutID		_baseLayoutId;
		ShaderModule			_shader;
		
		uint3					_defaultLocalGroupSize;
		uint3					_localSizeSpec			{UNDEFINED_SPECIALIZATION};
		SpecValues_t			_specialization;

		DebugName_t				_debugName;
		
		RWDataRaceCheck			_drCheck;


	// methods
	public:
		VComputePipelineTemplate () {}
		~VComputePipelineTemplate ();

		bool  Create (VPipelineLayoutID layoutId, const PipelineCompiler::ComputePipelineDesc &desc, const ShaderModule &module, StringView dbgName);
		void  Destroy (const VResourceManagerImpl &);
		
		ND_ VPipelineLayoutID	GetLayoutID ()		const	{ SHAREDLOCK( _drCheck );  return _baseLayoutId; }

		ND_ StringView			GetDebugName ()		const	{ SHAREDLOCK( _drCheck );  return _debugName; }
	};


}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
