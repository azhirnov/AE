// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef AE_ENABLE_VULKAN

# include "graphics/Public/ResourceEnums.h"
# include "graphics/Vulkan/VCommon.h"
# include "pipeline_compiler/Public/PipelinePack.h"

namespace AE::Graphics
{

	//
	// Render Pass Output
	//

	class VRenderPassOutput final
	{
	// types
	public:
		using FragOutput = PipelineCompiler::RenderPassInfo::FragmentOutput;
		using Output_t	 = FixedMap< RenderTargetName, FragOutput, GraphicsConfig::MaxColorBuffers >;


	// variables
	private:
		Output_t			_fragOutput;

		RWDataRaceCheck		_drCheck;


	// methods
	public:
		VRenderPassOutput () {}
		~VRenderPassOutput () {}


		bool  Create (const Output_t &fragOutput)
		{
			EXLOCK( _drCheck );
			CHECK_ERR( _fragOutput.empty() );

			_fragOutput = fragOutput;
			return true;
		}


		void Destroy (const VResourceManager &)
		{}


		ND_ bool  operator == (const VRenderPassOutput &rhs) const
		{
			SHAREDLOCK( _drCheck );
			SHAREDLOCK( rhs._drCheck );

			return _fragOutput == rhs._fragOutput;
		}


		ND_ Output_t const&		Get ()	const	{ SHAREDLOCK( _drCheck );  return _fragOutput; }
	};


}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
