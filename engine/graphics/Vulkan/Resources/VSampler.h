// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef AE_ENABLE_VULKAN

# include "graphics/Vulkan/VCommon.h"

namespace AE::Graphics
{

	//
	// Vulkan Sampler immutable data
	//

	class VSampler final
	{
	// variables
	private:
		VkSampler			_sampler	= VK_NULL_HANDLE;

		RWDataRaceCheck		_drCheck;


	// methods
	public:
		VSampler () {}
		~VSampler ();

		bool Create (const VDevice &dev, const VkSamplerCreateInfo &info, StringView dbgName);
		void Destroy (const VResourceManager &);

		ND_ VkSampler	Handle ()	const	{ SHAREDLOCK( _drCheck );  return _sampler; }
	};



	//
	// Vulkan Sampler Pack
	//

	class VSamplerPack final
	{
	// types
	public:
		struct SamplerRefs
		{
			mutable SharedMutex					guard;
			HashMap< SamplerName, VSamplerID >	map;
		};
	private:
		using Samplers_t	= Array< UniqueID< VSamplerID >>;


	// variables
	private:
		Samplers_t			_samplers;
		RWDataRaceCheck		_drCheck;


	// methods
	public:
		VSamplerPack () {}
		~VSamplerPack ();
		
		bool Create (VResourceManager &resMngr, const SharedPtr<RStream> &stream, INOUT SamplerRefs &refs);
		void Destroy (VResourceManager &resMngr);
	};
	

}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
