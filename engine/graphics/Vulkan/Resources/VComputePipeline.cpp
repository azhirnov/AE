// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

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
	void VComputePipeline::Destroy (VResourceManagerImpl &resMngr)
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
	bool VComputePipelineTemplate::Create (VPipelineLayoutID layoutId, const PipelineCompiler::ComputePipelineDesc &desc, const ShaderModule &module, StringView dbgName)
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
	void VComputePipelineTemplate::Destroy (const VResourceManagerImpl &)
	{
		EXLOCK( _drCheck );

		_baseLayoutId	= Default;
		_shader			= Default;
	}

}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
