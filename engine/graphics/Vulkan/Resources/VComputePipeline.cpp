// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#ifdef AE_ENABLE_VULKAN

# include "graphics/Vulkan/Resources/VComputePipeline.h"
# include "graphics/Vulkan/VResourceManager.h"

namespace AE::Graphics
{
	
/*
=================================================
	destructor
=================================================
*/
	VComputePipeline::~VComputePipeline ()
	{
		EXLOCK( _drCheck );
		CHECK( not _handle );
	}
	
/*
=================================================
	Create
=================================================
*/
	bool VComputePipeline::Create (VComputePipelineTemplateID templId, const ComputePipelineDesc &desc, VkPipeline ppln, VkPipelineLayout layout)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( not _handle and not _layout );

		_templ	= templId;
		_desc	= desc;
		_handle	= ppln;
		_layout	= layout;

		return true;
	}
	
/*
=================================================
	Destroy
=================================================
*/
	void VComputePipeline::Destroy (VResourceManager &resMngr)
	{
		EXLOCK( _drCheck );

		if ( _handle )
		{
			auto&	dev = resMngr.GetDevice();
			dev.vkDestroyPipeline( dev.GetVkDevice(), _handle, null );
		}

		_handle		= VK_NULL_HANDLE;
		_layout		= VK_NULL_HANDLE;
		_templ		= Default;
		//_desc		= Default;
	}
//-----------------------------------------------------------------------------
	

/*
=================================================
	destructor
=================================================
*/
	VComputePipelineTemplate::~VComputePipelineTemplate ()
	{
		EXLOCK( _drCheck );
		CHECK( not _baseLayoutId );
	}
	
/*
=================================================
	Create
=================================================
*/
	bool VComputePipelineTemplate::Create (VPipelineLayoutID layoutId, const PipelineCompiler::ComputePipelineDesc &desc, VkShaderModule module, StringView dbgName)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( not _baseLayoutId );
		
		_baseLayoutId			= layoutId;
		_shader					= module;
		_defaultLocalGroupSize	= desc.defaultLocalGroupSize;
		_localSizeSpec			= desc.localSizeSpec;
		_specialization			= desc.specialization;
		_debugName				= dbgName;

		return true;
	}
	
/*
=================================================
	Destroy
=================================================
*/
	void VComputePipelineTemplate::Destroy (VResourceManager &)
	{
		EXLOCK( _drCheck );

		_baseLayoutId	= Default;
		_shader			= VK_NULL_HANDLE;
	}

}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN


#if 0

#include "VComputePipeline.h"
#include "Shared/EnumUtils.h"
#include "VResourceManager.h"
#include "VDevice.h"

namespace AE::Graphics
{

/*
=================================================
	PipelineInstance::UpdateHash
=================================================
*/
	void VComputePipeline::PipelineInstance::UpdateHash ()
	{
#	if FG_FAST_HASH
		_hash	= FGC::HashOf( &_hash, sizeof(*this) - sizeof(_hash) );
#	else
		_hash	= HashOf( layoutId )	+ HashOf( localGroupSize ) +
				  HashOf( flags )		+ HashOf( debugMode );
#	endif
	}
//-----------------------------------------------------------------------------



/*
=================================================
	constructor
=================================================
*/
	VComputePipeline::VComputePipeline () :
		_localSizeSpec{ ComputePipelineDesc::UNDEFINED_SPECIALIZATION }
	{
	}

/*
=================================================
	destructor
=================================================
*/
	VComputePipeline::~VComputePipeline ()
	{
	}
	
/*
=================================================
	Create
=================================================
*/
	bool VComputePipeline::Create (const ComputePipelineDesc &desc, RawPipelineLayoutID layoutId, StringView dbgName)
	{
		EXLOCK( _drCheck );
		
		for (auto& sh : desc._shader.data)
		{
			auto*	vk_shader = UnionGetIf< PipelineDescription::VkShaderPtr >( &sh.second );
			CHECK_ERR( vk_shader );

			_shaders.push_back(ShaderModule{ *vk_shader, EShaderDebugMode_From(sh.first) });
		}
		CHECK_ERR( _shaders.size() );

		_baseLayoutId			= PipelineLayoutID{ layoutId };
		_defaultLocalGroupSize	= desc._defaultLocalGroupSize;
		_localSizeSpec			= desc._localSizeSpec;
		_debugName				= dbgName;
		
		return true;
	}
	
/*
=================================================
	Destroy
=================================================
*/
	void VComputePipeline::Destroy (VResourceManager &resMngr)
	{
		EXLOCK( _drCheck );

		auto&	dev = resMngr.GetDevice();

		for (auto& ppln : _instances) {
			dev.vkDestroyPipeline( dev.GetVkDevice(), ppln.second, null );
			resMngr.ReleaseResource( const_cast<PipelineInstance &>(ppln.first).layoutId );
		}
		
		if ( _baseLayoutId ) {
			resMngr.ReleaseResource( _baseLayoutId.Release() );
		}

		_instances.clear();
		_debugName.clear();
		_shaders.clear();

		_baseLayoutId			= Default;
		_defaultLocalGroupSize	= Default;
		_localSizeSpec			= uint3{ ComputePipelineDesc::UNDEFINED_SPECIALIZATION };
	}


}	// AE::Graphics

#endif
