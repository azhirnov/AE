// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#ifdef AE_ENABLE_VULKAN

# include "graphics/Vulkan/Resources/VSampler.h"
# include "graphics/Vulkan/VEnumCast.h"
# include "graphics/Vulkan/VResourceManager.h"

# include "serializing/ObjectFactory.h"

# include "sampler_packer/Private/SamplerDesc.h"
# include "sampler_packer/Private/SamplerDescDeserializer.cpp"

namespace AE::Graphics
{
	
/*
=================================================
	destructor
=================================================
*/
	VSampler::~VSampler ()
	{
		EXLOCK( _drCheck );
		CHECK( not _sampler );
	}

/*
=================================================
	Create
=================================================
*/
	bool  VSampler::Create (const VDevice &dev, const VkSamplerCreateInfo &info, StringView dbgName)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( not _sampler );

		VK_CHECK( dev.vkCreateSampler( dev.GetVkDevice(), &info, null, OUT &_sampler ));

		if ( dbgName.size() )
			dev.SetObjectName( uint64_t(_sampler), dbgName, VK_OBJECT_TYPE_SAMPLER );

		return true;
	}

/*
=================================================
	Destroy
=================================================
*/
	void  VSampler::Destroy (const VResourceManagerImpl &resMngr)
	{
		EXLOCK( _drCheck );

		auto&	dev = resMngr.GetDevice();

		if ( _sampler )
			dev.vkDestroySampler( dev.GetVkDevice(), _sampler, null );

		_sampler = VK_NULL_HANDLE;
	}
//-----------------------------------------------------------------------------



namespace {
	using namespace AE::SamplerPacker;

/*
=================================================
	Filter
=================================================
*/
	ND_ inline VkFilter  VEnumCast (EFilter value)
	{
		BEGIN_ENUM_CHECKS();
		switch ( value )
		{
			case EFilter::Nearest :	return VK_FILTER_NEAREST;
			case EFilter::Linear :	return VK_FILTER_LINEAR;
			case EFilter::Unknown :	break;
		}
		END_ENUM_CHECKS();
		RETURN_ERR( "unknown filter mode", VK_FILTER_MAX_ENUM );
	}

/*
=================================================
	SamplerMipmapMode
=================================================
*/
	ND_ inline VkSamplerMipmapMode  VEnumCast (EMipmapFilter value)
	{
		BEGIN_ENUM_CHECKS();
		switch ( value )
		{
			case EMipmapFilter::Nearest :	return VK_SAMPLER_MIPMAP_MODE_NEAREST;
			case EMipmapFilter::Linear :	return VK_SAMPLER_MIPMAP_MODE_LINEAR;
			case EMipmapFilter::Unknown :	break;
		}
		END_ENUM_CHECKS();
		RETURN_ERR( "unknown sampler mipmap mode", VK_SAMPLER_MIPMAP_MODE_MAX_ENUM );
	}

/*
=================================================
	SamplerAddressMode
=================================================
*/
	ND_ inline VkSamplerAddressMode  VEnumCast (EAddressMode value)
	{
		BEGIN_ENUM_CHECKS();
		switch ( value )
		{
			case EAddressMode::Repeat :				return VK_SAMPLER_ADDRESS_MODE_REPEAT;
			case EAddressMode::MirrorRepeat :		return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
			case EAddressMode::ClampToEdge :		return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			case EAddressMode::ClampToBorder :		return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
			case EAddressMode::MirrorClampToEdge :	return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
			case EAddressMode::Unknown :			break;
		}
		END_ENUM_CHECKS();
		RETURN_ERR( "unknown sampler address mode", VK_SAMPLER_ADDRESS_MODE_MAX_ENUM );
	}

/*
=================================================
	BorderColor
=================================================
*/
	ND_ inline VkBorderColor  VEnumCast (EBorderColor value)
	{
		BEGIN_ENUM_CHECKS();
		switch ( value )
		{
			case EBorderColor::FloatTransparentBlack :	return VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
			case EBorderColor::FloatOpaqueBlack :		return VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
			case EBorderColor::FloatOpaqueWhite :		return VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
			case EBorderColor::IntTransparentBlack :	return VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
			case EBorderColor::IntOpaqueBlack :			return VK_BORDER_COLOR_INT_OPAQUE_BLACK;
			case EBorderColor::IntOpaqueWhite :			return VK_BORDER_COLOR_INT_OPAQUE_WHITE;
			case EBorderColor::Unknown :				break;
		}
		END_ENUM_CHECKS();
		RETURN_ERR( "unknown border color type", VK_BORDER_COLOR_MAX_ENUM );
	}

/*
=================================================
	CreateSampler
=================================================
*/
	void  CreateSampler (const VDevice &dev, const SamplerDesc &desc, OUT VkSamplerCreateInfo &info)
	{
		info.sType					= VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		info.flags					= 0;
		info.magFilter				= VEnumCast( desc.magFilter );
		info.minFilter				= VEnumCast( desc.minFilter );
		info.mipmapMode				= VEnumCast( desc.mipmapMode );
		info.addressModeU			= VEnumCast( desc.addressMode.x );
		info.addressModeV			= VEnumCast( desc.addressMode.y );
		info.addressModeW			= VEnumCast( desc.addressMode.z );
		info.mipLodBias				= desc.mipLodBias;
		info.anisotropyEnable		= desc.maxAnisotropy.has_value() ? VK_TRUE : VK_FALSE;
		info.maxAnisotropy			= desc.maxAnisotropy.value_or( 0.0f );
		info.compareEnable			= desc.compareOp.has_value() ? VK_TRUE : VK_FALSE;
		info.compareOp				= VEnumCast( desc.compareOp.value_or( ECompareOp::Always ));
		info.minLod					= desc.minLod;
		info.maxLod					= desc.maxLod;
		info.borderColor			= VEnumCast( desc.borderColor );
		info.unnormalizedCoordinates= desc.unnormalizedCoordinates ? VK_TRUE : VK_FALSE;
		
		// validate
		const VkPhysicalDeviceLimits&	limits	= dev.GetProperties().properties.limits;
		const VkPhysicalDeviceFeatures&	feat	= dev.GetProperties().features;
		
		if ( info.mipLodBias > limits.maxSamplerLodBias )
		{
			ASSERT( info.mipLodBias <= limits.maxSamplerLodBias );
			info.mipLodBias = limits.maxSamplerLodBias;
		}
		
		if ( not feat.samplerAnisotropy )
		{
			ASSERT( not info.anisotropyEnable );
			info.anisotropyEnable = VK_FALSE;
		}

		if ( info.anisotropyEnable )
		{
			info.maxAnisotropy = Clamp( info.maxAnisotropy, 1.0f, limits.maxSamplerAnisotropy );
		}

		if ( not dev.GetFeatures().samplerMirrorClamp )
		{
			if ( info.addressModeU == VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE )
			{
				ASSERT( false );
				info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			}

			if ( info.addressModeV == VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE )
			{
				ASSERT( false );
				info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			}

			if ( info.addressModeW == VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE )
			{
				ASSERT( false );
				info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			}
		}
	}
}
//-----------------------------------------------------------------------------



/*
=================================================
	destructor
=================================================
*/
	VSamplerPack::~VSamplerPack ()
	{
		EXLOCK( _drCheck );
		CHECK( _samplers.empty() );
	}
	
/*
=================================================
	Create
=================================================
*/
	bool  VSamplerPack::Create (VResourceManagerImpl &resMngr, const RC<RStream> &stream, INOUT SamplerRefs &refs)
	{
		EXLOCK( _drCheck );
		
		AE::Serializing::Deserializer	des;
		des.stream = stream;

		uint	version	= 0;
		CHECK_ERR( des( OUT version ) and version == SamplerStorage_Version );
		
		Array<Pair< SamplerName, uint >>	samp_names;
		CHECK_ERR( des( OUT samp_names ));

		uint	count = 0;
		CHECK_ERR( des( OUT count ));
		_samplers.resize( count );

		for (uint i = 0; i < count; ++i)
		{
			SamplerDesc	desc;
			CHECK_ERR( desc.Deserialize( des ));

			VkSamplerCreateInfo	info = {};
			CreateSampler( resMngr.GetDevice(), desc, OUT info );

			_samplers[i] = resMngr.CreateSampler( info );
			CHECK_ERR( _samplers[i] );
		}

		EXLOCK( refs.guard );
		for (auto&[name, uid] : samp_names)
		{
			CHECK_ERR( usize(uid) < _samplers.size() );
			CHECK( refs.map.insert_or_assign( name, _samplers[usize(uid)] ).second );
		}

		return true;
	}
	
/*
=================================================
	Destroy
=================================================
*/
	void  VSamplerPack::Destroy (VResourceManagerImpl &resMngr)
	{
		EXLOCK( _drCheck );

		for (auto& id : _samplers) {
			resMngr.ReleaseResource( INOUT id );
		}
		_samplers.clear();
	}


}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
