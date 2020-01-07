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

		bool Create (VComputePipelineTemplateID templId, const ComputePipelineDesc &desc, VkPipeline ppln, VkPipelineLayout layout);
		void Destroy (VResourceManager &);

		ND_ ComputePipelineDesc const&	Description ()	const	{ SHAREDLOCK( _drCheck );  return _desc; }
		ND_ VkPipeline					Handle ()		const	{ SHAREDLOCK( _drCheck );  return _handle; }
		ND_ VkPipelineLayout			Layout ()		const	{ SHAREDLOCK( _drCheck );  return _layout; }
	};


	
	//
	// Vulkan Compute Pipeline Template
	//

	class VComputePipelineTemplate final
	{
		friend class VResourceManager;
		
	// types
	public:
		using SpecValues_t		= ComputePipelineDesc::SpecValues_t;
		using PipelineMap_t		= HashMultiMap< HashVal, ComputePipelineID >;		// TODO: use flat map


	// variables
	private:
		static constexpr uint	UNDEFINED_SPECIALIZATION = UMax;

		mutable SharedMutex		_pipelineMapGuard;
		mutable PipelineMap_t	_pipelineMap;

		VPipelineLayoutID		_baseLayoutId;
		VkShaderModule			_shader					= VK_NULL_HANDLE;
		
		uint3					_defaultLocalGroupSize;
		uint3					_localSizeSpec			{UNDEFINED_SPECIALIZATION};
		SpecValues_t			_specialization;

		DebugName_t				_debugName;
		
		RWDataRaceCheck			_drCheck;


	// methods
	public:
		VComputePipelineTemplate () {}
		~VComputePipelineTemplate ();

		bool Create (VPipelineLayoutID layoutId, const PipelineCompiler::ComputePipelineDesc &desc, VkShaderModule module, StringView dbgName);
		void Destroy (VResourceManager &);
		
		ND_ VPipelineLayoutID		GetLayoutID ()			const	{ SHAREDLOCK( _drCheck );  return _baseLayoutId; }

		ND_ StringView				GetDebugName ()			const	{ SHAREDLOCK( _drCheck );  return _debugName; }
	};


}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN



#if 0

#include "VPipelineLayout.h"

namespace AE::Graphics
{

	//
	// Compute Pipeline
	//

	class VComputePipeline final
	{
		friend class VPipelineCache;
		
	// types
	private:
		struct PipelineInstance
		{
		// variables
			HashVal					_hash;
			RawPipelineLayoutID		layoutId;		// strong reference
			uint3					localGroupSize;
			VkPipelineCreateFlags	flags		= 0;
			uint					debugMode	= 0;

		// methods
			PipelineInstance () {}

			void UpdateHash ();

			ND_ bool  operator == (const PipelineInstance &rhs) const;
		};

		struct PipelineInstanceHash {
			ND_ size_t	operator () (const PipelineInstance &value) const	{ return size_t(value._hash); }
		};

		struct ShaderModule
		{
			PipelineDescription::VkShaderPtr	module;
			EShaderDebugMode					debugMode	= Default;
		};

		using Instances_t			= HashMap< PipelineInstance, VkPipeline, PipelineInstanceHash >;
		using VkShaderPtr			= PipelineDescription::VkShaderPtr;
		using ShaderModules_t		= FixedArray< ShaderModule, 4 >;


	// variables
	private:
		mutable std::shared_mutex	_instanceGuard;
		mutable Instances_t			_instances;

		PipelineLayoutID			_baseLayoutId;
		ShaderModules_t				_shaders;

		uint3						_defaultLocalGroupSize;
		uint3						_localSizeSpec;

		DebugName_t					_debugName;
		
		RWDataRaceCheck				_drCheck;


	// methods
	public:
		VComputePipeline ();
		VComputePipeline (VComputePipeline &&) = default;
		~VComputePipeline ();
		
		bool Create (const ComputePipelineDesc &desc, RawPipelineLayoutID layoutId, StringView dbgName);
		void Destroy (VResourceManager &);

		ND_ RawPipelineLayoutID		GetLayoutID ()		const	{ SHAREDLOCK( _drCheck );  return _baseLayoutId.Get(); }
		
		ND_ StringView				GetDebugName ()		const	{ SHAREDLOCK( _drCheck );  return _debugName; }
	};

	
/*
=================================================
	PipelineInstance::operator ==
=================================================
*/
	inline bool VComputePipeline::PipelineInstance::operator == (const PipelineInstance &rhs) const
	{
		return	layoutId			== rhs.layoutId				and
				localGroupSize.x	== rhs.localGroupSize.x		and
				localGroupSize.y	== rhs.localGroupSize.y		and
				localGroupSize.z	== rhs.localGroupSize.z		and
				flags				== rhs.flags				and
				debugMode			== rhs.debugMode;
	}


}	// AE::Graphics

#endif
