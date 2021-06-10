// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Common.h"

#ifdef AE_BUILD_SAMPLER_PACKER
#	define AE_SP_API	AE_DLL_EXPORT
#else
#	define AE_SP_API	AE_DLL_IMPORT
#endif


namespace AE::SamplerPacker
{
	using uint = uint32_t;


	struct SamplersInfo
	{
		// input pipelines
		const wchar_t **	inSamplers			= null;
		uint				inSamplerCount		= 0;

		// output
		const wchar_t *		outputPackName		= null;
	};


	extern "C" bool AE_SP_API PackSamplers (const SamplersInfo *info);


}	// AE::SamplerPacker
