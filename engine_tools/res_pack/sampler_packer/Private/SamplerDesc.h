// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "graphics/Public/Common.h"
#include "graphics/Public/RenderStateEnums.h"
#include "graphics/Public/IDs.h"

#ifdef AE_BUILD_SAMPLER_PACKER
#  include "scripting/Impl/ScriptEngine.h"
#  include "scripting/Impl/ScriptTypes.h"
#  define SCRIPTING( ... )	__VA_ARGS__
#else
#  define SCRIPTING( ... )
#endif

#include "serializing/ISerializable.h"

namespace AE::SamplerPacker
{
	using namespace AE::Graphics;
	
	enum class SamplerUID : uint { Unknown = ~0u };


	enum class EFilter : uint
	{
		Nearest,
		Linear,

		Unknown	= ~0u,
	};


	enum class EMipmapFilter : uint
	{
		Nearest,
		Linear,
		
		Unknown	= ~0u,
	};


	enum class EAddressMode : uint
	{
		Repeat,
		MirrorRepeat,
		ClampToEdge,
		ClampToBorder,
		MirrorClampToEdge,
		
		Unknown	= ~0u,
	};


	enum class EBorderColor : uint
	{
		FloatTransparentBlack,
		FloatOpaqueBlack,
		FloatOpaqueWhite,

		IntTransparentBlack,
		IntOpaqueBlack,
		IntOpaqueWhite,
		
		Unknown	= ~0u,
	};


	
	//
	// Sampler description
	//

	class SamplerDesc final :
		SCRIPTING( public Scripting::AngelScriptHelper::SimpleRefCounter, )
		public Serializing::ISerializable
	{
	// types
	public:
		using Self			= SamplerDesc;
		using AddressMode3	= Vec< EAddressMode, 3 >;


	// variables
	public:
		String					_name;
		EFilter					magFilter				= EFilter::Nearest;
		EFilter					minFilter				= EFilter::Nearest;
		EMipmapFilter			mipmapMode				= EMipmapFilter::Nearest;
		AddressMode3			addressMode				= { EAddressMode::Repeat, EAddressMode::Repeat, EAddressMode::Repeat };
		float					mipLodBias				= 0.0f;
		Optional<float>			maxAnisotropy;
		Optional<ECompareOp>	compareOp;
		float					minLod					= -1000.0f;
		float					maxLod					= 1000.0f;
		EBorderColor			borderColor				= EBorderColor::FloatTransparentBlack;
		bool					unnormalizedCoordinates	= false;


	// methods
	public:
		SamplerDesc ();
		SamplerDesc (const SamplerDesc &);
		
		void  SetName (const String &name);
		void  SetFilter (EFilter mag, EFilter min, EMipmapFilter mipmap)		{ magFilter = mag;  minFilter = min;  mipmapMode = mipmap; }
		void  SetAddressModeV (EAddressMode uvw)								{ addressMode = {uvw, uvw, uvw}; }
		void  SetAddressMode (EAddressMode u, EAddressMode v, EAddressMode w)	{ addressMode = {u, v, w}; }
		void  SetMipLodBias (float value)										{ mipLodBias = value; }
		void  SetLodRange (float min, float max)								{ minLod = min;  maxLod = max; }
		void  SetAnisotropy (float value)										{ maxAnisotropy = value; }
		void  SetCompareOp (ECompareOp value)									{ compareOp = value; }
		void  SetBorderColor (EBorderColor value)								{ borderColor = value; }
		void  SetNormCoordinates (bool value)									{ unnormalizedCoordinates = not value; }

		bool  Validate ();

		SCRIPTING(
			static bool Bind (const Scripting::ScriptEnginePtr &se);
		)
		
		// ISerializable
		bool Serialize (Serializing::Serializer &) const override;
		bool Deserialize (Serializing::Deserializer const&) override;
	};
	
	
#ifdef AE_BUILD_SAMPLER_PACKER
	using SamplerDescPtr = Scripting::AngelScriptHelper::SharedPtr< SamplerDesc >;


	//
	// Sampler Storage
	//

	struct SamplerStorage
	{
	// types
		using SamplerRefs_t = Array< SamplerDescPtr >;


	// variables
		SamplerRefs_t				_samplerRefs;
		NamedID_HashCollisionCheck	_hashCollisionCheck;


	// methods
		SamplerStorage () {}

		bool Serialize (Serializing::Serializer &ser);

		ND_ static SamplerStorage*  Instance ();
		static void SetInstance (SamplerStorage *);
	};

#endif
	
	static constexpr uint	SamplerStorage_Version = 1;


}	// AE::SamplerPacker

#undef SCRIPTING
