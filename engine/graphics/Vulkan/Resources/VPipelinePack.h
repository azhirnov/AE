// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef AE_ENABLE_VULKAN

# include "graphics/Public/IDs.h"
# include "stl/Memory/LinearAllocator.h"
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
		// types
			template <typename NameType, typename ValueType>
			struct ItemsTmpl
			{
				mutable SharedMutex				guard;
				HashMap< NameType, ValueType >	map;
			};

			using GPipelineMap_t	= ItemsTmpl< PipelineName, VGraphicsPipelineTemplateID >;
			using MPipelineMap_t	= ItemsTmpl< PipelineName, VMeshPipelineTemplateID >;
			using CPipelineMap_t	= ItemsTmpl< PipelineName, VComputePipelineTemplateID >;
			//using RTPipelineMap_t	= ItemsTmpl< PipelineName, RayTracingPipelineID >;
			using RPNames_t			= ItemsTmpl< RenderPassName, VRenderPassOutputID >;
			
		// variables
			GPipelineMap_t			graphics;
			MPipelineMap_t			mesh;
			CPipelineMap_t			compute;
			//RTPipelineMap_t		rayTracing;
			RPNames_t				renderPassNames;
		};


	private:
		using PipelineLayoutDesc	= PipelineCompiler::PipelineLayoutDesc;
		using PipelineStorage		= PipelineCompiler::PipelineStorage;
		using EMarker				= PipelineStorage::EMarker;
		using SpecConstants_t		= PipelineCompiler::SpirvShaderCode::SpecConstants_t;

		using DSLayouts_t			= Tuple< size_t, UniqueID< DescriptorSetLayoutID >* >;		// TODO: custom allocator
		using PplnLayouts_t			= Tuple< size_t, UniqueID< VPipelineLayoutID >* >;
		using ShaderModules_t		= Tuple< size_t, VkShaderModule*, SpecConstants_t** >;
		using GPipelines_t			= Tuple< size_t, UniqueID< VGraphicsPipelineTemplateID >* >;
		using MPipelines_t			= Tuple< size_t, UniqueID< VMeshPipelineTemplateID >* >;
		using CPipelines_t			= Tuple< size_t, UniqueID< VComputePipelineTemplateID >* >;
		using RPOutputs_t			= Tuple< size_t, UniqueID< VRenderPassOutputID >* >;
		
		using StackAllocator_t		= StackAllocator< UntypedAlignedAllocator, 16, false >;
	

	// variables
	private:
		LinearAllocator<>	_allocator;
		DSLayouts_t			_dsLayouts;
		PplnLayouts_t		_pplnLayouts;
		ShaderModules_t		_shaderModules;
		GPipelines_t		_gpipelines;
		MPipelines_t		_mpipelines;
		CPipelines_t		_cpipelines;
		RPOutputs_t			_renderPassOutputs;

		RWDataRaceCheck		_drCheck;


	// methods
	public:
		VPipelinePack () {}
		~VPipelinePack ();

		bool Create (VResourceManager &resMngr, const SharedPtr<RStream> &stream, INOUT PipelineRefs &refs);
		void Destroy (VResourceManager &resMngr);

	private:
		bool _LoadDescrSetLayouts (VResourceManager &resMngr, Serializing::Deserializer &, StackAllocator_t &);
		bool _LoadPipelineLayouts (VResourceManager &resMngr, Serializing::Deserializer &);
		bool _LoadRenderPasses (VResourceManager &resMngr, Serializing::Deserializer &, INOUT PipelineRefs &refs);
		bool _LoadSpirvShaders (const VDevice &dev, Serializing::Deserializer &, StackAllocator_t &);
		bool _LoadGraphicsPipelines (VResourceManager &resMngr, Serializing::Deserializer &, StackAllocator_t &allocator);
		bool _LoadMeshPipelines (VResourceManager &resMngr, Serializing::Deserializer &, StackAllocator_t &allocator);
		bool _LoadComputePipelines (VResourceManager &resMngr, Serializing::Deserializer &);
		bool _LoadRayTracingPipelines (VResourceManager &resMngr, Serializing::Deserializer &);
		bool _LoadPipelineNames (Serializing::Deserializer &des, INOUT PipelineRefs &refs);
	};


}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
