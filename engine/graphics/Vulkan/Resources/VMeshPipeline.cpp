// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#ifdef AE_ENABLE_VULKAN

# include "graphics/Vulkan/Resources/VMeshPipeline.h"
# include "graphics/Vulkan/VResourceManager.h"

namespace AE::Graphics
{

/*
=================================================
	destructor
=================================================
*/
	VMeshPipeline::~VMeshPipeline ()
	{
		EXLOCK( _drCheck );
		CHECK( not _handle );
	}
	
/*
=================================================
	Create
=================================================
*/
	bool VMeshPipeline::Create (VMeshPipelineTemplateID templId, const MeshPipelineDesc &desc, VkPipeline ppln, VkPipelineLayout layout)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( not _handle and not _layout );
		
		// _desc.renderPassId reference has been acquired by VResourceManager

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
	void VMeshPipeline::Destroy (VResourceManager &resMngr)
	{
		EXLOCK( _drCheck );

		if ( _handle )
		{
			auto&	dev = resMngr.GetDevice();
			dev.vkDestroyPipeline( dev.GetVkDevice(), _handle, null );
		}
		
		{
			UniqueID<RenderPassID>	rp{ _desc.renderPassId };
			resMngr.ReleaseResource( rp );
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
	VMeshPipelineTemplate::~VMeshPipelineTemplate ()
	{
		EXLOCK( _drCheck );
		CHECK( not _baseLayoutId );
	}

/*
=================================================
	Create
=================================================
*/
	bool  VMeshPipelineTemplate::Create (VPipelineLayoutID layoutId, VRenderPassOutputID rpOutputId, const PipelineCompiler::MeshPipelineDesc &desc, ArrayView<ShaderModule> modules, StringView dbgName)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( not _baseLayoutId );
		
		_baseLayoutId			= layoutId;
		_shaders				= modules;
		_renderPassOutput		= rpOutputId;

		_topology				= desc.topology;
		_maxVertices			= desc.maxVertices;
		_maxIndices				= desc.maxIndices;
		_defaultTaskGroupSize	= desc.defaultTaskGroupSize;
		_taskSizeSpec			= desc.taskSizeSpec;
		_defaultMeshGroupSize	= desc.defaultMeshGroupSize;
		_meshSizeSpec			= desc.meshSizeSpec;
		_specialization			= desc.specialization;
		_earlyFragmentTests		= desc.earlyFragmentTests;

		_debugName				= dbgName;

		return true;
	}

/*
=================================================
	Destroy
=================================================
*/
	void  VMeshPipelineTemplate::Destroy (const VResourceManager &)
	{
		EXLOCK( _drCheck );

		_baseLayoutId		= Default;
		_renderPassOutput	= Default;
		_shaders.clear();
	}

}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
