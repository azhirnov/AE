// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef AE_ENABLE_VULKAN

# include "graphics/Vulkan/Resources/VGraphicsPipeline.h"

namespace AE::Graphics
{

	//
	// Vulkan Mesh Pipeline
	//

	class VMeshPipeline
	{
	// types
	public:
		using SpecValues_t	= MeshPipelineDesc::SpecValues_t;


	// variables
	private:
		VkPipeline					_handle			= VK_NULL_HANDLE;
		VkPipelineLayout			_layout			= VK_NULL_HANDLE;

		VMeshPipelineTemplateID		_templ;
		MeshPipelineDesc			_desc;
		
		RWDataRaceCheck				_drCheck;


	// methods
	public:
		VMeshPipeline () {}
		~VMeshPipeline ();

		bool Create (VMeshPipelineTemplateID templId, const MeshPipelineDesc &desc, VkPipeline ppln, VkPipelineLayout layout);
		void Destroy (VResourceManager &);

		ND_ MeshPipelineDesc const&		Description ()	const	{ SHAREDLOCK( _drCheck );  return _desc; }
		ND_ VkPipeline					Handle ()		const	{ SHAREDLOCK( _drCheck );  return _handle; }
		ND_ VkPipelineLayout			Layout ()		const	{ SHAREDLOCK( _drCheck );  return _layout; }
		ND_ VMeshPipelineTemplateID		TemplateID ()	const	{ SHAREDLOCK( _drCheck );  return _templ; }
	};



	//
	// Vulkan Mesh Pipeline Template
	//

	class VMeshPipelineTemplate final
	{
		friend class VResourceManager;

	// types
	private:
		using ShaderModule		= VGraphicsPipelineTemplate::ShaderModule;
		using ShaderModules_t	= FixedArray< ShaderModule, 3 >;
		using TopologyBits_t	= PipelineCompiler::GraphicsPipelineDesc::TopologyBits_t;
		using VertexAttrib		= VertexInputState::VertexAttrib;
		using VertexAttribs_t	= PipelineCompiler::GraphicsPipelineDesc::VertexAttribs_t;
		using SpecValues_t		= GraphicsPipelineDesc::SpecValues_t;
		using PipelineID_t		= MeshPipelineID;
		using PipelineMap_t		= HashMultiMap< HashVal, PipelineID_t >;		// TODO: use flat map


	// variables
	private:
		static constexpr uint	UNDEFINED_SPECIALIZATION = UMax;

		mutable SharedMutex		_pipelineMapGuard;
		mutable PipelineMap_t	_pipelineMap;

		VPipelineLayoutID		_baseLayoutId;
		ShaderModules_t			_shaders;
		
		VRenderPassOutputID		_renderPassOutput;
		EPrimitive				_topology				= Default;
		uint					_maxVertices			= 0;
		uint					_maxIndices				= 0;
		uint3					_defaultTaskGroupSize;
		uint3					_taskSizeSpec			{UNDEFINED_SPECIALIZATION};
		uint3					_defaultMeshGroupSize;
		uint3					_meshSizeSpec			{UNDEFINED_SPECIALIZATION};
		SpecValues_t			_specialization;
		bool					_earlyFragmentTests		= true;
		
		DebugName_t				_debugName;
		
		RWDataRaceCheck			_drCheck;


	// methods
	public:
		VMeshPipelineTemplate () {}
		~VMeshPipelineTemplate ();

		bool Create (VPipelineLayoutID layoutId, VRenderPassOutputID rpOutputId, const PipelineCompiler::MeshPipelineDesc &desc, ArrayView<ShaderModule> modules, StringView dbgName);
		void Destroy (const VResourceManager &);
		
		ND_ VPipelineLayoutID		GetLayoutID ()			const	{ SHAREDLOCK( _drCheck );  return _baseLayoutId; }
		ND_ bool					IsEarlyFragmentTests ()	const	{ SHAREDLOCK( _drCheck );  return _earlyFragmentTests; }
		ND_ StringView				GetDebugName ()			const	{ SHAREDLOCK( _drCheck );  return _debugName; }
	};


}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
