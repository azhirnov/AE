// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#ifndef AE_BUILD_SAMPLER_PACKER

#include "serializing/ObjectFactory.h"

namespace AE::SamplerPacker
{
	
	SamplerDesc::SamplerDesc ()
	{}
	
	SamplerDesc::SamplerDesc (const SamplerDesc &) = default;

	void SamplerDesc::SetName (const String &)
	{}
	
	bool SamplerDesc::Serialize (Serializing::Serializer &) const
	{
		return false;
	}
	
	bool SamplerDesc::Deserialize (Serializing::Deserializer const &des)
	{
		bool	result = true;
		result &= des( OUT magFilter, OUT minFilter, OUT mipmapMode );
		result &= des( OUT addressMode );
		result &= des( OUT mipLodBias, OUT maxAnisotropy, OUT compareOp );
		result &= des( OUT minLod, OUT maxLod );
		result &= des( OUT borderColor, OUT unnormalizedCoordinates	);
		return result;
	}

}	// AE::SamplerPacker

#endif	// AE_BUILD_SAMPLER_PACKER
