// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef AE_ENABLE_VULKAN

# include "graphics/Vulkan/VCommon.h"
# include "graphics/Public/RenderState.h"
# include "graphics/Public/ResourceManager.h"
# include "pipeline_compiler/Public/PipelinePack.h"

namespace AE::Graphics
{

	//
	// Vulkan Graphics Pipeline
	//

	class VGraphicsPipeline
	{
	// types
	public:
		using SpecValues_t	= GraphicsPipelineDesc::SpecValues_t;


	// variables
	private:
		VkPipeline						_handle			= VK_NULL_HANDLE;
		VkPipelineLayout				_layout			= VK_NULL_HANDLE;

		VGraphicsPipelineTemplateID		_templ;
		GraphicsPipelineDesc			_desc;
		
		RWDataRaceCheck					_drCheck;


	// methods
	public:
		VGraphicsPipeline () {}
		~VGraphicsPipeline ();

		bool Create (VGraphicsPipelineTemplateID, const GraphicsPipelineDesc &, VkPipeline, VkPipelineLayout);
		void Destroy (VResourceManager &);

		ND_ GraphicsPipelineDesc const&	Description ()	const	{ SHAREDLOCK( _drCheck );  return _desc; }
		ND_ VkPipeline					Handle ()		const	{ SHAREDLOCK( _drCheck );  return _handle; }
		ND_ VkPipelineLayout			Layout ()		const	{ SHAREDLOCK( _drCheck );  return _layout; }
		ND_ VGraphicsPipelineTemplateID	TemplateID ()	const	{ SHAREDLOCK( _drCheck );  return _templ; }
	};



	//
	// Vulkan Graphics Pipeline Template
	//

	class VGraphicsPipelineTemplate final
	{
		friend class VResourceManager;

	// types
	public:
		using SpecConstants_t		= PipelineCompiler::SpirvShaderCode::SpecConstants_t;

		struct ShaderModule
		{
			VkShaderStageFlagBits	stage	= VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
			VkShaderModule			module	= VK_NULL_HANDLE;
			SpecConstants_t*		spec	= null;
		};

	private:
		using ShaderModules_t	= FixedArray< ShaderModule, 5 >;
		using TopologyBits_t	= PipelineCompiler::GraphicsPipelineDesc::TopologyBits_t;
		using VertexAttrib		= VertexInputState::VertexAttrib;
		using VertexAttribs_t	= PipelineCompiler::GraphicsPipelineDesc::VertexAttribs_t;
		using SpecValues_t		= GraphicsPipelineDesc::SpecValues_t;
		using PipelineID_t		= GraphicsPipelineID;
		using PipelineMap_t		= HashMultiMap< HashVal, PipelineID_t >;		// TODO: use flat map


	// variables
	private:
		mutable SharedMutex		_pipelineMapGuard;
		mutable PipelineMap_t	_pipelineMap;

		VPipelineLayoutID		_baseLayoutId;
		ShaderModules_t			_shaders;

		VRenderPassOutputID		_renderPassOutput;
		TopologyBits_t			_supportedTopology;
		VertexAttribs_t			_vertexAttribs;
		SpecValues_t			_specialization;
		uint					_patchControlPoints		= 0;
		bool					_earlyFragmentTests		= true;
		
		DebugName_t				_debugName;
		
		RWDataRaceCheck			_drCheck;


	// methods
	public:
		VGraphicsPipelineTemplate () {}
		~VGraphicsPipelineTemplate ();

		bool Create (VPipelineLayoutID layoutId, VRenderPassOutputID rpOutputId, const PipelineCompiler::GraphicsPipelineDesc &desc, ArrayView<ShaderModule> modules, StringView dbgName);
		void Destroy (const VResourceManager &);
		
		ND_ VPipelineLayoutID		GetLayoutID ()			const	{ SHAREDLOCK( _drCheck );  return _baseLayoutId; }
		ND_ ArrayView<VertexAttrib>	GetVertexAttribs ()		const	{ SHAREDLOCK( _drCheck );  return _vertexAttribs; }

		ND_ bool					IsEarlyFragmentTests ()	const	{ SHAREDLOCK( _drCheck );  return _earlyFragmentTests; }
		
		ND_ StringView				GetDebugName ()			const	{ SHAREDLOCK( _drCheck );  return _debugName; }
	};


}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
