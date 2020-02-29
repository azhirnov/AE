// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Common.h"

#ifdef AE_BUILD_PIPELINE_COMPILER
#	define AE_PC_API	AE_DLL_EXPORT
#else
#	define AE_PC_API	AE_DLL_IMPORT
#endif


namespace AE::PipelineCompiler
{
	using uint = uint32_t;


	struct PipelinesInfo
	{
		// input pipelines
		const wchar_t **	pipelineFolders		= null;
		uint				pipelineFolderCount	= 0;
		const wchar_t **	inPipelines			= null;
		uint				inPipelineCount		= 0;

		// input shaders
		const wchar_t **	shaderFolders		= null;
		uint				shaderFolderCount	= 0;

		// include directories
		const wchar_t **	includeDirs			= null;
		uint				includeDirCount		= 0;

		// output
		const wchar_t *		outputPackName		= null;
	};


	extern "C" bool AE_PC_API CompilePipelines (const PipelinesInfo *info);


}	// AE::PipelineCompiler
