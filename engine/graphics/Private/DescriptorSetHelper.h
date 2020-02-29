// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "graphics/Public/DescriptorSet.h"
#include "pipeline_compiler/Public/PipelinePack.h"

namespace AE::Graphics
{
	
	//
	// Descriptor Set Helper
	//
	class DescriptorSetHelper
	{
	public:
		using DynamicDataPtr	= DescriptorSet::DynamicDataPtr;
		using Uniform			= PipelineCompiler::DescriptorSetLayoutDesc::Uniform;
		using Uniforms_t		= Tuple< uint, UniformName const*, Uniform const* >;

		ND_ static DynamicDataPtr  CreateDynamicData (const Uniforms_t &uniforms, uint bufferDynamicOffsetCount);
		
		ND_ static DynamicDataPtr  CloneDynamicData (const DescriptorSet &);
		ND_ static DynamicDataPtr  RemoveDynamicData (INOUT DescriptorSet &);

		static bool Initialize (OUT DescriptorSet &, DescriptorSetLayoutID layoutId, uint index, const DynamicDataPtr &);
	};

}	// AE::Graphics
