// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef AE_ENABLE_VULKAN

# include "graphics/Public/IDs.h"
# include "stl/Memory/AllocatorFwdDecl.h"
# include "serializing/Common.h"
# include "pipeline_compiler/Public/PipelinePack.h"
# include "graphics/Vulkan/VCommon.h"

namespace AE::Graphics
{

	//
	// Vulkan Pipeline Pack
	//

	class VPipelinePack
	{
	// types
	public:
		struct PipelineRefs
		{
			using GPipelineMap_t	= HashMap< PipelineName, VGraphicsPipelineTemplateID >;
			using MPipelineMap_t	= HashMap< PipelineName, VMeshPipelineTemplateID >;
			using CPipelineMap_t	= HashMap< PipelineName, VComputePipelineTemplateID >;
			//using RTPipelineMap_t	= HashMap< PipelineName, RayTracingPipelineID >;
			using UniquePplnNames_t	= HashSet< PipelineName >;
			
			mutable SharedMutex		guard;
			GPipelineMap_t			graphics;
			MPipelineMap_t			mesh;
			CPipelineMap_t			compute;
			//RTPipelineMap_t		rayTracing;
		};


	private:
		using PipelineLayoutDesc	= PipelineCompiler::PipelineLayoutDesc;
		using PipelineStorage		= PipelineCompiler::PipelineStorage;
		using EMarker				= PipelineStorage::EMarker;

		using DSLayouts_t			= Array< VDescriptorSetLayoutID >;		// TODO: custom allocator
		using PplnLayouts_t			= Array< VPipelineLayoutID >;
		using ShaderModules_t		= Array< VkShaderModule >;
		
		using Allocator_t			= StackAllocator< UntypedAlignedAllocator, 16 >;


	// variables
	private:
		DSLayouts_t			_dsLayouts;
		PplnLayouts_t		_pplnLayouts;
		ShaderModules_t		_shaderModules;
		
		RWDataRaceCheck		_drCheck;


	// methods
	public:
		VPipelinePack () {}
		~VPipelinePack ();

		bool Create (VResourceManager &resMngr, const SharedPtr<RStream> &stream, INOUT PipelineRefs &refs);
		void Destroy (VResourceManager &resMngr);

	private:
		bool _LoadDescrSetLayouts (VResourceManager &resMngr, Serializing::Deserializer &, Allocator_t &);
		bool _LoadPipelineLayouts (VResourceManager &resMngr, Serializing::Deserializer &);
		bool _LoadSpirvShaders (const VDevice &dev, Serializing::Deserializer &, Allocator_t &);
		bool _LoadGraphicsPipelines (VResourceManager &resMngr, Serializing::Deserializer &, Allocator_t &, OUT ArrayView<VGraphicsPipelineTemplateID> &);
		bool _LoadMeshPipelines (VResourceManager &resMngr, Serializing::Deserializer &, Allocator_t &, OUT ArrayView<VMeshPipelineTemplateID> &);
		bool _LoadComputePipelines (VResourceManager &resMngr, Serializing::Deserializer &, Allocator_t &, OUT ArrayView<VComputePipelineTemplateID> &);
		bool _LoadPipelineNames (ArrayView<VGraphicsPipelineTemplateID>, ArrayView<VMeshPipelineTemplateID>,
								 ArrayView<VComputePipelineTemplateID>, Serializing::Deserializer &des, INOUT PipelineRefs &refs);
	};


}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
