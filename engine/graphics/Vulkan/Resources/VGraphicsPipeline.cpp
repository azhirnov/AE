// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#ifdef AE_ENABLE_VULKAN

# include "graphics/Vulkan/Resources/VGraphicsPipeline.h"
# include "graphics/Vulkan/VResourceManager.h"

namespace AE::Graphics
{

/*
=================================================
	destructor
=================================================
*/
	VGraphicsPipeline::~VGraphicsPipeline ()
	{
		EXLOCK( _drCheck );
		CHECK( not _handle );
	}
	
/*
=================================================
	Create
=================================================
*/
	bool VGraphicsPipeline::Create (VGraphicsPipelineTemplateID templId, const GraphicsPipelineDesc &desc, VkPipeline ppln, VkPipelineLayout layout)
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
	void VGraphicsPipeline::Destroy (VResourceManager &resMngr)
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
	VGraphicsPipelineTemplate::~VGraphicsPipelineTemplate ()
	{
		EXLOCK( _drCheck );
		CHECK( not _baseLayoutId );
	}

/*
=================================================
	Create
=================================================
*/
	bool  VGraphicsPipelineTemplate::Create (VPipelineLayoutID layoutId, const PipelineCompiler::GraphicsPipelineDesc &desc, ArrayView<ShaderModule> modules, StringView dbgName)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( not _baseLayoutId );
		
		_baseLayoutId		= layoutId;
		_shaders			= modules;
		_supportedTopology	= desc.supportedTopology;
		_vertexAttribs		= desc.vertexAttribs;
		_specialization		= desc.specialization;
		_patchControlPoints	= desc.patchControlPoints;
		_earlyFragmentTests	= desc.earlyFragmentTests;
		_debugName			= dbgName;

		return true;
	}

/*
=================================================
	Destroy
=================================================
*/
	void  VGraphicsPipelineTemplate::Destroy (VResourceManager &)
	{
		EXLOCK( _drCheck );

		_baseLayoutId	= Default;
		_shaders.clear();
	}

}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
