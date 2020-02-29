// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "Private/SamplerDesc.h"

#include "serializing/ObjectFactory.h"

#include "scripting/Bindings/CoreBindings.h"
#include "scripting/Impl/ClassBinder.h"
#include "scripting/Impl/EnumBinder.h"
#include "scripting/Bindings/CoreBindings.h"

#ifndef AE_BUILD_SAMPLER_PACKER
#  error AE_BUILD_SAMPLER_PACKER required
#endif

namespace AE::Scripting
{
	AE_DECL_SCRIPT_TYPE(   AE::SamplerPacker::ECompareOp,		"ECompareOp"	);
	AE_DECL_SCRIPT_TYPE(   AE::SamplerPacker::EFilter,			"EFilter"		);
	AE_DECL_SCRIPT_TYPE(   AE::SamplerPacker::EMipmapFilter,	"EMipmapFilter"	);
	AE_DECL_SCRIPT_TYPE(   AE::SamplerPacker::EAddressMode,		"EAddressMode"	);
	AE_DECL_SCRIPT_TYPE(   AE::SamplerPacker::EBorderColor,		"EBorderColor"	);
	AE_DECL_SCRIPT_OBJ_RC( AE::SamplerPacker::SamplerDesc,		"Sampler"		);

}	// AE::Scripting

namespace AE::SamplerPacker
{
	
/*
=================================================
	constructor
=================================================
*/
	SamplerDesc::SamplerDesc ()
	{
		SamplerStorage::Instance()->_samplerRefs.push_back( SamplerDescPtr{this} );
	}
	
	SamplerDesc::SamplerDesc (const SamplerDesc &other) :
		_name{ other._name },
		magFilter{ other.magFilter },
		minFilter{ other.minFilter },
		mipmapMode{ other.mipmapMode },
		addressMode{ other.addressMode },
		mipLodBias{ other.mipLodBias },
		maxAnisotropy{ other.maxAnisotropy },
		compareOp{ other.compareOp },
		minLod{ other.minLod },
		maxLod{ other.maxLod },
		borderColor{ other.borderColor },
		unnormalizedCoordinates{ other.unnormalizedCoordinates }
	{
	}

/*
=================================================
	SetName
=================================================
*/
	void SamplerDesc::SetName (const String &name)
	{
		_name = name;
		
		SamplerStorage::Instance()->_hashCollisionCheck.Add( SamplerName{_name} );
	}

/*
=================================================
	Bind
=================================================
*/
	bool SamplerDesc::Bind (const Scripting::ScriptEnginePtr &se)
	{
		using namespace AE::Scripting;

		CoreBindings::BindString( se );

		// bind ECompareOp
		{
			EnumBinder<ECompareOp>	binder{ se };
			CHECK_ERR( binder.Create() );
			CHECK_ERR( binder.AddValue( "Never",	ECompareOp::Never ));
			CHECK_ERR( binder.AddValue( "Less",		ECompareOp::Less ));
			CHECK_ERR( binder.AddValue( "Equal",	ECompareOp::Equal ));
			CHECK_ERR( binder.AddValue( "LEqual",	ECompareOp::LEqual ));
			CHECK_ERR( binder.AddValue( "Greater",	ECompareOp::Greater ));
			CHECK_ERR( binder.AddValue( "NotEqual",	ECompareOp::NotEqual ));
			CHECK_ERR( binder.AddValue( "GEqual",	ECompareOp::GEqual ));
			CHECK_ERR( binder.AddValue( "Always",	ECompareOp::Always ));
		}

		// bind EFilter
		{
			EnumBinder<EFilter>		binder{ se };
			CHECK_ERR( binder.Create() );
			CHECK_ERR( binder.AddValue( "Nearest",	EFilter::Nearest ));
			CHECK_ERR( binder.AddValue( "Linear",	EFilter::Linear ));
		}
		
		// bind EMipmapFilter
		{
			EnumBinder<EMipmapFilter>		binder{ se };
			CHECK_ERR( binder.Create() );
			CHECK_ERR( binder.AddValue( "Nearest",	EMipmapFilter::Nearest ));
			CHECK_ERR( binder.AddValue( "Linear",	EMipmapFilter::Linear ));
		}
		
		// bind EAddressMode
		{
			EnumBinder<EAddressMode>		binder{ se };
			CHECK_ERR( binder.Create() );
			CHECK_ERR( binder.AddValue( "Repeat",				EAddressMode::Repeat ));
			CHECK_ERR( binder.AddValue( "MirrorRepeat",			EAddressMode::MirrorRepeat ));
			CHECK_ERR( binder.AddValue( "ClampToEdge",			EAddressMode::ClampToEdge ));
			CHECK_ERR( binder.AddValue( "Clamp",				EAddressMode::ClampToEdge ));
			CHECK_ERR( binder.AddValue( "ClampToBorder",		EAddressMode::ClampToBorder ));
			CHECK_ERR( binder.AddValue( "MirrorClampToEdge",	EAddressMode::MirrorClampToEdge ));
		}
		
		// bind EBorderColor
		{
			EnumBinder<EBorderColor>		binder{ se };
			CHECK_ERR( binder.Create() );
			CHECK_ERR( binder.AddValue( "FloatTransparentBlack",	EBorderColor::FloatTransparentBlack ));
			CHECK_ERR( binder.AddValue( "FloatOpaqueBlack",			EBorderColor::FloatOpaqueBlack ));
			CHECK_ERR( binder.AddValue( "FloatOpaqueWhite",			EBorderColor::FloatOpaqueWhite ));
			CHECK_ERR( binder.AddValue( "IntTransparentBlack",		EBorderColor::IntTransparentBlack ));
			CHECK_ERR( binder.AddValue( "IntOpaqueBlack",			EBorderColor::IntOpaqueBlack ));
			CHECK_ERR( binder.AddValue( "IntOpaqueWhite",			EBorderColor::IntOpaqueWhite ));
		}

		// bind sampler
		{
			ClassBinder<SamplerDesc>	binder{ se };
			CHECK_ERR( binder.CreateRef() );
			CHECK_ERR( binder.AddMethod( &SamplerDesc::SetName,				"SetName"			));
			CHECK_ERR( binder.AddMethod( &SamplerDesc::SetFilter,			"SetFilter"			));
			CHECK_ERR( binder.AddMethod( &SamplerDesc::SetAddressModeV,		"SetAddressMode"	));
			CHECK_ERR( binder.AddMethod( &SamplerDesc::SetAddressMode,		"SetAddressMode"	));
			CHECK_ERR( binder.AddMethod( &SamplerDesc::SetMipLodBias,		"SetMipLodBias"		));
			CHECK_ERR( binder.AddMethod( &SamplerDesc::SetLodRange,			"SetLodRange"		));
			CHECK_ERR( binder.AddMethod( &SamplerDesc::SetAnisotropy,		"SetAnisotropy"		));
			CHECK_ERR( binder.AddMethod( &SamplerDesc::SetCompareOp,		"SetCompareOp"		));
			CHECK_ERR( binder.AddMethod( &SamplerDesc::SetBorderColor,		"SetBorderColor"	));
			CHECK_ERR( binder.AddMethod( &SamplerDesc::SetNormCoordinates,	"SetNormCoordinates"));
		}

		return true;
	}
	
/*
=================================================
	Serialize
=================================================
*/
	bool SamplerDesc::Serialize (Serializing::Serializer &ser) const
	{
		bool	result = true;
		result &= ser( magFilter, minFilter, mipmapMode );
		result &= ser( addressMode );
		result &= ser( mipLodBias, maxAnisotropy, compareOp );
		result &= ser( minLod, maxLod );
		result &= ser( borderColor, unnormalizedCoordinates	);
		return result;
	}
	
/*
=================================================
	Deserialize
=================================================
*/
	bool SamplerDesc::Deserialize (Serializing::Deserializer const &)
	{
		return false;
	}
	
/*
=================================================
	Validate
=================================================
*/
	bool SamplerDesc::Validate ()
	{
		bool	result = true;

		if ( unnormalizedCoordinates )
		{
			if ( minFilter != magFilter )
			{
				AE_LOGI( "Sampler '"s <<  _name << "': min & mag filter for unnormalized coordinates must equal" );
				magFilter	= minFilter;
				result		= false;
			}

			if ( mipmapMode != EMipmapFilter::Nearest )
			{
				AE_LOGI( "Sampler '"s <<  _name << "': mipmap filter for unnormalized coordinates must be 'nearest'" );
				mipmapMode	= EMipmapFilter::Nearest;
				result		= false;
			}

			if ( minLod != 0.0f or maxLod != 0.0f )
			{
				//AE_LOGI( "Sampler '"s <<  _name << "': min & max LOD for unnormalized coordinates must be zero" );
				minLod	= maxLod = 0.0f;
				result	= false;
			}

			if ( addressMode.x != EAddressMode::ClampToEdge and addressMode.x != EAddressMode::ClampToBorder )
			{
				AE_LOGI( "Sampler '"s <<  _name << "': U-address mode for unnormalized coordinates must be 'clamp'" );
				addressMode.x	= EAddressMode::ClampToEdge;
				result			= false;
			}

			if ( addressMode.y != EAddressMode::ClampToEdge and addressMode.y != EAddressMode::ClampToBorder )
			{
				AE_LOGI( "Sampler '"s <<  _name << "': V-address mode for unnormalized coordinates must be 'clamp'" );
				addressMode.y	= EAddressMode::ClampToEdge;
				result			= false;
			}

			if ( maxAnisotropy.has_value() )
			{
				AE_LOGI( "Sampler '"s <<  _name << "': anisotropy filter for unnormalized coordinates is not supported" );
				maxAnisotropy	= {};
				result			= false;
			}

			if ( compareOp.has_value() )
			{
				AE_LOGI( "Sampler '"s <<  _name << "': compare mode for unnormalized coordinates is not supported" );
				compareOp	= {};
				result		= false;
			}
		}

		if ( maxLod < minLod )
		{
			AE_LOGI( "Sampler '"s <<  _name << "': min LOD must be less than or equal to max LOD" );
			maxLod	= minLod;
			result	= false;
		}
		
		//if ( (minFilter == EFilter::Cubic or magFilter == EFilter::Cubic) and maxAnisotropy.has_value() )
		//{
		//	AE_LOGI( "Sampler '"s <<  _name << "': anisotropy filter is not supported for cubic filter" );
		//	maxAnisotropy	= {};
		//	result			= false;
		//}
		
		if ( addressMode.x != EAddressMode::ClampToBorder and
			 addressMode.y != EAddressMode::ClampToBorder and
			 addressMode.z != EAddressMode::ClampToBorder )
		{
			// reset border color, because it is unused
			borderColor = EBorderColor::FloatTransparentBlack;
		}

		return result;
	}
//-----------------------------------------------------------------------------
	
	
/*
=================================================
	Serialize
=================================================
*/
	bool SamplerStorage::Serialize (Serializing::Serializer &ser)
	{
		CHECK_ERR( _samplerRefs.size() );
		CHECK_ERR( not _hashCollisionCheck.HasCollisions() );
		
		using SamplerMap_t		= HashMultiMap< size_t, SamplerUID >;
		using SamplerArr_t		= Array< SamplerDesc >;
		using SamplerNameMap_t	= HashMap< SamplerName, SamplerUID >;

		SamplerMap_t		unique_samplers;
		SamplerArr_t		sampler_arr;
		SamplerNameMap_t	sampler_names;

		const auto	SamplerDescHash = [] (const SamplerDesc &desc)
		{
			HashVal	result;
			result << HashOf( desc.magFilter ) << HashOf( desc.minFilter ) << HashOf( desc.mipmapMode );
			result << HashOf( desc.addressMode );
			result << HashOf( desc.mipLodBias ) << HashOf( desc.minLod ) << HashOf( desc.maxLod );
			result << HashOf( desc.maxAnisotropy ) << HashOf( desc.compareOp );
			result << HashOf( desc.borderColor ) << HashOf( desc.unnormalizedCoordinates );
			return size_t(result);
		};

		const auto	SamplerDescEqual = [] (const SamplerDesc &lhs, const SamplerDesc &rhs)
		{
			return	lhs.magFilter		== rhs.magFilter			and
					lhs.minFilter		== rhs.minFilter			and
					lhs.mipmapMode		== rhs.mipmapMode			and
					All( lhs.addressMode== rhs.addressMode )		and
					Equals( lhs.mipLodBias,	rhs.mipLodBias )		and
					Equals( lhs.minLod,		rhs.minLod )			and
					Equals( lhs.maxLod,		rhs.maxLod )			and
					Equals( lhs.maxAnisotropy, rhs.maxAnisotropy )	and
					Equals( lhs.compareOp,	rhs.compareOp )			and
					lhs.borderColor		== rhs.borderColor			and
					lhs.unnormalizedCoordinates == rhs.unnormalizedCoordinates;
		};

		const auto	AddSampler = [&] (const SamplerDesc &desc)
		{
			const size_t	hash	= SamplerDescHash( desc );
			auto			iter	= unique_samplers.find( hash );

			for (; iter != unique_samplers.end() and iter->first == hash; ++iter)
			{
				auto&	lhs = sampler_arr[ size_t(iter->second) ];
			
				if ( SamplerDescEqual( lhs, desc ))
				{
					CHECK( sampler_names.insert_or_assign( SamplerName{desc._name}, iter->second ).second );
					return;
				}
			}

			auto	uid = SamplerUID( sampler_arr.size() );

			unique_samplers.insert({ hash, uid });
			sampler_arr.push_back( desc );
		
			CHECK( sampler_names.insert_or_assign( SamplerName{desc._name}, uid ).second );
		};

		for (auto& samp : _samplerRefs)
		{
			samp->Validate();
			AddSampler( *samp );
		}

		Array<Pair< SamplerName, SamplerUID >>	sampler_names2;
		for (auto& item : sampler_names) {
			sampler_names2.push_back( item );
		}
		std::sort( sampler_names2.begin(), sampler_names2.end(), [](auto& lhs, auto& rhs) { return lhs.first < rhs.first; });
		

		bool	result = true;
		result &= ser( SamplerStorage_Version );
		result &= ser( sampler_names2 );
		result &= ser( sampler_arr );

		AE_LOGI( "Serialized samplers: "s << ToString(sampler_names2.size()) );

		_samplerRefs.clear();
		return true;
	}

/*
=================================================
	Instance
=================================================
*/
	static SamplerStorage*&  SamplerStorage_Instance ()
	{
		static SamplerStorage *	inst = null;
		return inst;
	}

	SamplerStorage*  SamplerStorage::Instance ()
	{
		return SamplerStorage_Instance();
	}
	
	void SamplerStorage::SetInstance (SamplerStorage *inst)
	{
		SamplerStorage_Instance() = inst;
	}

}	// AE::SamplerPacker
