// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#ifdef AE_ENABLE_VULKAN

# include "graphics/Vulkan/VResourceManager.h"
# include "graphics/Vulkan/VEnumCast.h"
# include "stl/Memory/LinearAllocator.h"

namespace AE::Graphics
{
namespace {
/*
=================================================
	Replace
----
	destroy previous resource instance and construct new instance
=================================================
*/
	template <typename ResType, typename ...Args>
	inline void Replace (INOUT VResourceBase<ResType> &target, Args&& ...args)
	{
		target.Data().~ResType();
		new (&target.Data()) ResType{ std::forward<Args &&>(args)... };
	}
}
//-----------------------------------------------------------------------------

	

/*
=================================================
	CreateDependency
=================================================
*/
	GfxResourceID  VResourceManager::CreateDependency (StringView dbgName)
	{
		return Default;
	}
	
/*
=================================================
	_CreateMemory
=================================================
*/
	bool  VResourceManager::_CreateMemory (OUT MemoryID &id, OUT VResourceBase<VMemoryObj>* &memPtr, EMemoryType type, StringView dbgName)
	{
		CHECK_ERR( _Assign( OUT id ));

		auto&	data = _GetResourcePool( id )[ id.Index() ];
		Replace( data );

		if ( not data.Create( type, dbgName ) )
		{
			_Unassign( id );
			RETURN_ERR( "failed when creating memory object" );
		}
		
		memPtr = &data;
		return true;
	}

/*
=================================================
	CreateImage
=================================================
*/
	GfxResourceID  VResourceManager::CreateImage (const ImageDesc &desc, EResourceState defaultState, StringView dbgName)
	{
		MemoryID					mem_id;
		VResourceBase<VMemoryObj>*	mem_obj	= null;
		CHECK_ERR( _CreateMemory( OUT mem_id, OUT mem_obj, desc.memType, dbgName ));

		VImageID	id;
		CHECK_ERR( _Assign( OUT id ));

		auto&	data = _GetResourcePool( id )[ id.Index() ];
		Replace( data );

		if ( not data.Create( *this, desc, mem_id, mem_obj->Data(), _GetQueueFamilies( desc.queues ), defaultState, dbgName ))
		{
			ReleaseResource( mem_id );
			_Unassign( id );
			RETURN_ERR( "failed when creating image" );
		}
		
		mem_obj->AddRef();
		data.AddRef();

		return GfxResourceID{ id.Index(), id.Generation(), GfxResourceID::EType::Image };
	}
	
/*
=================================================
	CreateBuffer
=================================================
*/
	GfxResourceID  VResourceManager::CreateBuffer (const BufferDesc &desc, StringView dbgName)
	{
		MemoryID					mem_id;
		VResourceBase<VMemoryObj>*	mem_obj	= null;
		CHECK_ERR( _CreateMemory( OUT mem_id, OUT mem_obj, desc.memType, dbgName ));

		VBufferID	id;
		CHECK_ERR( _Assign( OUT id ));

		auto&	data = _GetResourcePool( id )[ id.Index() ];
		Replace( data );
		
		if ( not data.Create( *this, desc, mem_id, mem_obj->Data(), _GetQueueFamilies( desc.queues ), dbgName ))
		{
			ReleaseResource( mem_id );
			_Unassign( id );
			RETURN_ERR( "failed when creating buffer" );
		}
		
		mem_obj->AddRef();
		data.AddRef();

		return GfxResourceID{ id.Index(), id.Generation(), GfxResourceID::EType::Buffer };
	}
	
/*
=================================================
	_CreateCachedResource
=================================================
*
	template <typename ID, typename FnInitialize, typename FnCreate>
	inline ID  VResourceManager::_CreateCachedResource (StringView errorStr, FnInitialize&& fnInit, FnCreate&& fnCreate)
	{
		ID	id;
		CHECK_ERR( _Assign( OUT id ));

		auto&	pool	= _GetResourcePool( id );
		auto&	data	= pool[ id.Index() ];
		fnInit( INOUT data );
		
		// search in cache
		Index_t	temp_id		= pool.Find( &data );
		bool	is_created	= false;

		if ( temp_id == UMax )
		{
			// create new
			if ( not fnCreate( data ) )
			{
				_Unassign( id );
				RETURN_ERR( errorStr );
			}

			data.AddRef();
			is_created = true;
			
			// try to add to cache
			temp_id = pool.AddToCache( id.Index() ).first;
		}

		if ( temp_id == id.Index() )
			return id;

		// use already cached resource
		auto&	temp = pool[ temp_id ];
		temp.AddRef();

		if ( is_created )
			data.Destroy( *this );
		
		_Unassign( id );

		return ID( temp_id, temp.GetInstanceID() );
	}

/*
=================================================
	Create***
=================================================
*
	SamplerID  VResourceManager::CreateSampler (const SamplerDesc &desc, StringView dbgName)
	{
		return _CreateCachedResource<SamplerID>( "failed when creating sampler",
										[&] (auto& data) { return Replace( data, _device, desc ); },
										[&] (auto& data) { return data.Create( _device, dbgName ); });
	}
	
/*
=================================================
	GetDescriptorSetLayout
=================================================
*
	DescriptorSetLayoutID  VResourceManager::GetDescriptorSetLayout (EShaderDebugMode debugMode, EShaderStages debuggableShaders)
	{
		const uint	key  = (uint(debuggableShaders) & 0xFFFFFF) | (uint(debugMode) << 24);
		auto		iter = _debugDSLayoutsCache.find( key );

		if ( iter != _debugDSLayoutsCache.end() )
			return iter->second;

		PipelineDescription::UniformMap_t	uniforms;
		PipelineDescription::Uniform		sb_uniform;
		PipelineDescription::StorageBuffer	sb_desc;

		sb_desc.state				= EResourceState_FromShaders( debuggableShaders ) | EResourceState::ShaderReadWrite | EResourceState::_BufferDynamicOffset;
		sb_desc.arrayStride			= SizeOf<uint>;
		sb_desc.staticSize			= GetDebugShaderStorageSize( debuggableShaders ) + SizeOf<uint>;	// per shader data + position
		sb_desc.dynamicOffsetIndex	= 0;

		sb_uniform.index			= BindingIndex{ UMax, 0 };
		sb_uniform.stageFlags		= debuggableShaders;
		sb_uniform.data				= sb_desc;
		sb_uniform.arraySize		= 1;

		uniforms.insert({ UniformID{"dbg_ShaderTrace"}, sb_uniform });

		auto	layout = CreateDescriptorSetLayout( MakeShared<const PipelineDescription::UniformMap_t>( std::move(uniforms) ));
		CHECK_ERR( layout );

		_debugDSLayoutsCache.insert({ key, layout });
		return layout;
	}

/*
=================================================
	_GetQueueFamilies
=================================================
*/
	EQueueFamilyMask  VResourceManager::_GetQueueFamilies (EQueueMask mask) const
	{
		EQueueFamilyMask	result = Default;

		for (auto& q : _device.GetQueues())
		{
			if ( EnumEq( mask, EQueueMask(0) | q.type ))
				result |= q.familyIndex;
		}

		return result;
	}
	
/*
=================================================
	LoadSamplerPack
=================================================
*/
	bool  VResourceManager::LoadSamplerPack (const SharedPtr<RStream> &stream)
	{
		VSamplerPackID	id;
		CHECK_ERR( _Assign( OUT id ));

		auto&	data = _GetResourcePool( id )[ id.Index() ];
		Replace( data );
		
		if ( not data.Create( *this, stream, INOUT _samplerRefs ))
		{
			_Unassign( id );
			RETURN_ERR( "failed when loading sampler pack" );
		}
		
		data.AddRef();
		return true;
	}
	
/*
=================================================
	CreateSampler
=================================================
*/
	VSamplerID  VResourceManager::CreateSampler (const VkSamplerCreateInfo &info, StringView dbgName)
	{
		VSamplerID	id;
		CHECK_ERR( _Assign( OUT id ));

		auto&	data = _GetResourcePool( id )[ id.Index() ];
		Replace( data );
		
		if ( not data.Create( GetDevice(), info, dbgName ))
		{
			_Unassign( id );
			RETURN_ERR( "failed when creating sampler" );
		}
		
		data.AddRef();
		return id;
	}
	
/*
=================================================
	GetSampler
=================================================
*/
	VSamplerID  VResourceManager::GetSampler (const SamplerName &name) const
	{
		SHAREDLOCK( _samplerRefs.guard );

		auto	iter = _samplerRefs.map.find( name );
		CHECK_ERR( iter != _samplerRefs.map.end() );

		return iter->second;
	}
	
/*
=================================================
	GetVkSampler
=================================================
*/
	VkSampler  VResourceManager::GetVkSampler (const SamplerName &name) const
	{
		auto*	res = GetResource( GetSampler( name ));

		if ( not res )
			res = GetResource( _defaultSampler );

		return res->Handle();
	}

/*
=================================================
	_CreateDefaultSampler
=================================================
*/
	bool  VResourceManager::_CreateDefaultSampler ()
	{
		CHECK_ERR( not _defaultSampler );

		VkSamplerCreateInfo		info = {};
		info.sType					= VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		info.flags					= 0;
		info.magFilter				= VK_FILTER_NEAREST;
		info.minFilter				= VK_FILTER_NEAREST;
		info.mipmapMode				= VK_SAMPLER_MIPMAP_MODE_NEAREST;
		info.addressModeU			= VK_SAMPLER_ADDRESS_MODE_REPEAT;
		info.addressModeV			= VK_SAMPLER_ADDRESS_MODE_REPEAT;
		info.addressModeW			= VK_SAMPLER_ADDRESS_MODE_REPEAT;
		info.mipLodBias				= 0.0f;
		info.anisotropyEnable		= VK_FALSE;
		info.maxAnisotropy			= 0.0f;
		info.compareEnable			= VK_FALSE;
		info.compareOp				= VK_COMPARE_OP_NEVER;
		info.minLod					= 0.0f;
		info.maxLod					= 0.0f;
		info.borderColor			= VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
		info.unnormalizedCoordinates= VK_FALSE;
		
		_defaultSampler = CreateSampler( info );
		CHECK_ERR( _defaultSampler );

		return true;
	}
//-----------------------------------------------------------------------------



namespace {
/*
=================================================
	SetShaderStages
=================================================
*/
	void SetShaderStages (OUT VkPipelineShaderStageCreateInfo const*			&outStages,
						  OUT uint												&outCount,
						  ArrayView<VGraphicsPipelineTemplate::ShaderModule>	shaders,
						  LinearAllocator<>										&allocator)
	{
		auto*	stages	= allocator.Alloc<VkPipelineShaderStageCreateInfo>( 5 );
		outStages		= stages;
		outCount		= 0;

		for (auto& sh : shaders)
		{
			ASSERT( sh.module );

			VkPipelineShaderStageCreateInfo&	info = stages[outCount++];
			info.sType		= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			info.pNext		= null;
			info.flags		= 0;
			info.module		= sh.module;
			info.pName		= "main";
			info.stage		= sh.stage;
			info.pSpecializationInfo = null;
		}
	}

/*
=================================================
	SetDynamicState
=================================================
*/
	void SetDynamicState (OUT VkPipelineDynamicStateCreateInfo	&outState,
						  EPipelineDynamicState					inState,
						  LinearAllocator<>						&allocator)
	{
		constexpr uint	max_size = CT_IntLog2<uint(EPipelineDynamicState::_Last)> + 1;

		auto*	states = allocator.Alloc<VkDynamicState>( max_size );

		outState.sType				= VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		outState.pNext				= null;
		outState.flags				= 0;
		outState.dynamicStateCount	= 0;
		outState.pDynamicStates		= states;

		for (EPipelineDynamicState t = EPipelineDynamicState(1 << 0);
			 t < EPipelineDynamicState::_Last;
			 t = EPipelineDynamicState(uint(t) << 1))
		{
			if ( not EnumEq( inState, t ))
				continue;

			*(states++) = VEnumCast( t );
			++outState.dynamicStateCount;
		}

		ASSERT( outState.dynamicStateCount <= max_size );
	}
	
/*
=================================================
	SetMultisampleState
=================================================
*/
	void SetMultisampleState (OUT VkPipelineMultisampleStateCreateInfo	&outState,
							  const RenderState::MultisampleState		&inState)
	{
		outState.sType					= VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		outState.pNext					= null;
		outState.flags					= 0;
		outState.rasterizationSamples	= VEnumCast( inState.samples );
		outState.sampleShadingEnable	= inState.sampleShading;
		outState.minSampleShading		= inState.minSampleShading;
		outState.pSampleMask			= inState.samples.IsEnabled() ? inState.sampleMask.data() : null;
		outState.alphaToCoverageEnable	= inState.alphaToCoverage;
		outState.alphaToOneEnable		= inState.alphaToOne;
	}
	
/*
=================================================
	SetTessellationState
=================================================
*/
	void SetTessellationState (OUT VkPipelineTessellationStateCreateInfo &outState,
							   uint patchSize)
	{
		outState.sType				= VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
		outState.pNext				= null;
		outState.flags				= 0;
		outState.patchControlPoints	= patchSize;
	}
	
/*
=================================================
	SetStencilOpState
=================================================
*/
	void SetStencilOpState (OUT VkStencilOpState				&outState,
							const RenderState::StencilFaceState	&inState)
	{
		outState.failOp			= VEnumCast( inState.failOp );
		outState.passOp			= VEnumCast( inState.passOp );
		outState.depthFailOp	= VEnumCast( inState.depthFailOp );
		outState.compareOp		= VEnumCast( inState.compareOp );
		outState.compareMask	= inState.compareMask;
		outState.writeMask		= inState.writeMask;
		outState.reference		= inState.reference;
	}

/*
=================================================
	SetDepthStencilState
=================================================
*/
	void SetDepthStencilState (OUT VkPipelineDepthStencilStateCreateInfo	&outState,
							   const RenderState::DepthBufferState			&depth,
							   const RenderState::StencilBufferState		&stencil)
	{
		outState.sType					= VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		outState.pNext					= null;
		outState.flags					= 0;

		// depth
		outState.depthTestEnable		= depth.test;
		outState.depthWriteEnable		= depth.write;
		outState.depthCompareOp			= VEnumCast( depth.compareOp );
		outState.depthBoundsTestEnable	= depth.boundsEnabled;
		outState.minDepthBounds			= depth.bounds.x;
		outState.maxDepthBounds			= depth.bounds.y;
		
		// stencil
		outState.stencilTestEnable		= stencil.enabled;
		SetStencilOpState( OUT outState.front, stencil.front );
		SetStencilOpState( OUT outState.back,  stencil.back  );
	}
	
/*
=================================================
	SetRasterizationState
=================================================
*/
	void SetRasterizationState (OUT VkPipelineRasterizationStateCreateInfo	&outState,
								const RenderState::RasterizationState		&inState)
	{
		outState.sType						= VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		outState.pNext						= null;
		outState.flags						= 0;
		outState.polygonMode				= VEnumCast( inState.polygonMode );
		outState.lineWidth					= inState.lineWidth;
		outState.depthBiasConstantFactor	= inState.depthBiasConstFactor;
		outState.depthBiasClamp				= inState.depthBiasClamp;
		outState.depthBiasSlopeFactor		= inState.depthBiasSlopeFactor;
		outState.depthBiasEnable			= inState.depthBias;
		outState.depthClampEnable			= inState.depthClamp;
		outState.rasterizerDiscardEnable	= inState.rasterizerDiscard;
		outState.frontFace					= inState.frontFaceCCW ? VK_FRONT_FACE_COUNTER_CLOCKWISE : VK_FRONT_FACE_CLOCKWISE;
		outState.cullMode					= VEnumCast( inState.cullMode );
	}

/*
=================================================
	SetupPipelineInputAssemblyState
=================================================
*/
	void SetupPipelineInputAssemblyState (OUT VkPipelineInputAssemblyStateCreateInfo	&outState,
										  const RenderState::InputAssemblyState			&inState)
	{
		outState.sType					= VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		outState.pNext					= null;
		outState.flags					= 0;
		outState.topology				= VEnumCast( inState.topology );
		outState.primitiveRestartEnable	= inState.primitiveRestart;
	}

/*
=================================================
	SetVertexInputState
=================================================
*/
	void SetVertexInputState (OUT VkPipelineVertexInputStateCreateInfo	&outState,
							  const VertexInputState					&inState,
							  LinearAllocator<>							&allocator)
	{
		auto*	input_attrib	= allocator.Alloc<VkVertexInputAttributeDescription>( inState.Vertices().size() );
		auto*	buffer_binding	= allocator.Alloc<VkVertexInputBindingDescription>( inState.BufferBindings().size() );

		outState.sType	= VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		outState.pNext	= null;
		outState.flags	= 0;

		outState.pVertexAttributeDescriptions		= input_attrib;
		outState.vertexAttributeDescriptionCount	= uint(inState.Vertices().size());
		outState.pVertexBindingDescriptions			= buffer_binding;
		outState.vertexBindingDescriptionCount		= uint(inState.BufferBindings().size());

		for (auto& src : inState.Vertices())
		{
			ASSERT( src.second.index != UMax );

			VkVertexInputAttributeDescription&	dst = *(input_attrib++);
			dst.binding		= src.second.bufferBinding;
			dst.format		= VEnumCast( src.second.type );
			dst.location	= src.second.index;
			dst.offset		= uint(src.second.offset);
		}

		for (auto& src : inState.BufferBindings())
		{
			VkVertexInputBindingDescription&	dst = *(buffer_binding++);
			dst.binding		= src.second.index;
			dst.inputRate	= VEnumCast( src.second.rate );
			dst.stride		= uint(src.second.stride);
		}

		// if pipeline has attributes then buffer binding must be defined too
		CHECK( inState.Vertices().empty() == inState.BufferBindings().empty() );
	}

/*
=================================================
	SetViewportState
=================================================
*/
	void SetViewportState (OUT VkPipelineViewportStateCreateInfo	&outState,
						   const uint2								&viewportSize,
						   const uint								viewportCount,
						   EPipelineDynamicState					dynamicStates,
							  LinearAllocator<>						&allocator)
	{
		auto*	viewports	= allocator.Alloc<VkViewport>( viewportCount );
		auto*	scissors	= allocator.Alloc<VkRect2D>( viewportCount );

		for (uint i = 0; i < viewportCount; ++i)
		{
			viewports[i] = VkViewport{ 0, 0, float(viewportSize.x), float(viewportSize.y), 0.0f, 1.0f };
			scissors[i]	 = VkRect2D{ VkOffset2D{ 0, 0 }, VkExtent2D{ viewportSize.x, viewportSize.y } };
		}

		outState.sType			= VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		outState.pNext			= null;
		outState.flags			= 0;
		outState.pViewports		= EnumEq( dynamicStates, EPipelineDynamicState::Viewport ) ? null : viewports;
		outState.viewportCount	= viewportCount;
		outState.pScissors		= EnumEq( dynamicStates, EPipelineDynamicState::Scissor ) ? null : scissors;
		outState.scissorCount	= viewportCount;
	}

/*
=================================================
	SetColorBlendAttachmentState
=================================================
*
	void SetColorBlendAttachmentState (OUT VkPipelineColorBlendAttachmentState &outState,
									   const RenderState::ColorBuffer &inState,
									   const bool logicOpEnabled)
	{
		outState.blendEnable			= logicOpEnabled ? false : inState.blend;
		outState.srcColorBlendFactor	= VEnumCast( inState.srcBlendFactor.color );
		outState.srcAlphaBlendFactor	= VEnumCast( inState.srcBlendFactor.alpha );
		outState.dstColorBlendFactor	= VEnumCast( inState.dstBlendFactor.color );
		outState.dstAlphaBlendFactor	= VEnumCast( inState.dstBlendFactor.alpha );
		outState.colorBlendOp			= VEnumCast( inState.blendOp.color );
		outState.alphaBlendOp			= VEnumCast( inState.blendOp.alpha );
		outState.colorWriteMask			= (inState.colorMask.x ? VK_COLOR_COMPONENT_R_BIT : 0) |
										  (inState.colorMask.y ? VK_COLOR_COMPONENT_G_BIT : 0) |
										  (inState.colorMask.z ? VK_COLOR_COMPONENT_B_BIT : 0) |
										  (inState.colorMask.w ? VK_COLOR_COMPONENT_A_BIT : 0);
	}

/*
=================================================
	SetColorBlendState
=================================================
*
	void SetColorBlendState (OUT VkPipelineColorBlendStateCreateInfo	&outState,
							 OUT ColorAttachments_t						&attachments,
							 const RenderState::ColorBuffersState		&inState,
							 const VRenderPass							&renderPass,
							 const uint									subpassIndex)
	{
		ASSERT( subpassIndex < renderPass.GetCreateInfo().subpassCount );

		const bool					logic_op_enabled	= ( inState.logicOp != ELogicOp::None );
		const VkSubpassDescription&	subpass				= renderPass.GetCreateInfo().pSubpasses[ subpassIndex ];

		for (size_t i = 0; i < subpass.colorAttachmentCount; ++i)
		{
			VkPipelineColorBlendAttachmentState		color_state = {};
			color_state.colorWriteMask	= VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
										  VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
			attachments.push_back( color_state );
		}

		for (size_t i = 0, cnt = Min(inState.buffers.size(), attachments.size()); i < cnt; ++i)
		{
			SetColorBlendAttachmentState( OUT attachments[i], inState.buffers[i], logic_op_enabled );
		}

		outState.sType				= VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		outState.pNext				= null;
		outState.flags				= 0;
		outState.attachmentCount	= uint(attachments.size());
		outState.pAttachments		= attachments.empty() ? null : attachments.data();
		outState.logicOpEnable		= logic_op_enabled;
		outState.logicOp			= logic_op_enabled ? VEnumCast( inState.logicOp ) : VK_LOGIC_OP_CLEAR;

		outState.blendConstants[0] = inState.blendColor.r;
		outState.blendConstants[1] = inState.blendColor.g;
		outState.blendConstants[2] = inState.blendColor.b;
		outState.blendConstants[3] = inState.blendColor.a;
	}

/*
=================================================
	ValidateRenderState
=================================================
*/
	void ValidateRenderState (const VDevice &dev, INOUT RenderState &renderState, INOUT EPipelineDynamicState &dynamicStates)
	{
		if ( renderState.rasterization.rasterizerDiscard )
		{
			renderState.color	= Default;
			renderState.depth	= Default;
			renderState.stencil	= Default;
			dynamicStates		= dynamicStates & ~(EPipelineDynamicState::RasterizerMask);
		}

		// reset to default state if dynamic state enabled.
		// it is needed for correct hash calculation.
		for (EPipelineDynamicState t = EPipelineDynamicState(1 << 0);
			 t < EPipelineDynamicState::_Last;
			 t = EPipelineDynamicState(uint(t) << 1)) 
		{
			if ( not EnumEq( dynamicStates, t ) )
				continue;

			BEGIN_ENUM_CHECKS();
			switch ( t )
			{
				case EPipelineDynamicState::Viewport :
					break;
				case EPipelineDynamicState::Scissor :
					break;

				//case EPipelineDynamicState::LineWidth :
				//	renderState.rasterization.lineWidth = 1.0f;
				//	break;

				//case EPipelineDynamicState::DepthBias :
				//	ASSERT( renderState.rasterization.depthBias );
				//	renderState.rasterization.depthBiasConstFactor	= 0.0f;
				//	renderState.rasterization.depthBiasClamp		= 0.0f;
				//	renderState.rasterization.depthBiasSlopeFactor	= 0.0f;
				//	break;

				//case EPipelineDynamicState::BlendConstants :
				//	renderState.color.blendColor = RGBA32f{ 1.0f };
				//	break;

				//case EPipelineDynamicState::DepthBounds :
				//	ASSERT( renderState.depth.boundsEnabled );
				//	renderState.depth.bounds = { 0.0f, 1.0f };
				//	break;

				case EPipelineDynamicState::StencilCompareMask :
					ASSERT( renderState.stencil.enabled ); 
					renderState.stencil.front.compareMask = UMax;
					renderState.stencil.back.compareMask  = UMax;
					break;

				case EPipelineDynamicState::StencilWriteMask :
					ASSERT( renderState.stencil.enabled ); 
					renderState.stencil.front.writeMask = UMax;
					renderState.stencil.back.writeMask  = UMax;
					break;

				case EPipelineDynamicState::StencilReference :
					ASSERT( renderState.stencil.enabled ); 
					renderState.stencil.front.reference = 0;
					renderState.stencil.back.reference  = 0;
					break;

				case EPipelineDynamicState::ShadingRatePalette :
					// do nothing
					break;

				case EPipelineDynamicState::Unknown :
				case EPipelineDynamicState::Default :
				case EPipelineDynamicState::_Last :
				case EPipelineDynamicState::All :
					break;	// to shutup warnings

				default :
					ASSERT(false);	// not supported
					break;
			}
			END_ENUM_CHECKS();
		}

		// validate color buffer states
		{
			const bool	dual_src_blend	= dev.GetDeviceFeatures().dualSrcBlend;

			const auto	IsDualSrcBlendFactor = [] (EBlendFactor value) {
				switch ( value ) {
					case EBlendFactor::Src1Color :
					case EBlendFactor::OneMinusSrc1Color :
					case EBlendFactor::Src1Alpha :
					case EBlendFactor::OneMinusSrc1Alpha :
						return true;
				}
				return false;
			};

			for (auto& cb : renderState.color.buffers)
			{
				if ( not cb.blend )
				{	
					cb.srcBlendFactor =	 { EBlendFactor::One,	EBlendFactor::One };
					cb.dstBlendFactor	= { EBlendFactor::Zero,	EBlendFactor::Zero };
					cb.blendOp			= { EBlendOp::Add,		EBlendOp::Add };
				}
				else
				if ( not dual_src_blend )
				{
					ASSERT( not IsDualSrcBlendFactor( cb.srcBlendFactor.color ) );
					ASSERT( not IsDualSrcBlendFactor( cb.srcBlendFactor.alpha ) );
					ASSERT( not IsDualSrcBlendFactor( cb.dstBlendFactor.color ) );
					ASSERT( not IsDualSrcBlendFactor( cb.dstBlendFactor.alpha ) );
				}
			}
		}

		// validate depth states
		{
			if ( not renderState.depth.test )
				renderState.depth.compareOp = ECompareOp::LEqual;

			//if ( not renderState.depth.write )

			if ( not renderState.depth.boundsEnabled )
				renderState.depth.bounds = { 0.0f, 1.0f };
		}

		// validate stencil states
		{
			if ( not renderState.stencil.enabled )
			{
				renderState.stencil = Default;
			}
		}
	}
}

/*
=================================================
	GetGraphicsPipeline
=================================================
*/
	GraphicsPipelineID  VResourceManager::GetGraphicsPipeline (const PipelineName &name, const GraphicsPipelineDesc &inDesc)
	{
		VGraphicsPipelineTemplateID			templ_id;
		GraphicsPipelineDesc				desc		= inDesc;
		HashVal								desc_hash;
		VGraphicsPipelineTemplate const*	ppln_templ	= null;

		// find pipeline template
		{
			auto	iter = _pipelineRefs.graphics.find( name );
			CHECK_ERR( iter != _pipelineRefs.graphics.end() );

			templ_id = iter->second;

			ppln_templ = GetResource( templ_id );
			CHECK_ERR( ppln_templ );
		}

		// validate
		{
			if ( ppln_templ->_patchControlPoints )
				desc.renderState.inputAssembly.topology = EPrimitive::Patch;

			desc.vertexInput.ApplyAttribs( ppln_templ->GetVertexAttribs() );
			ValidateRenderState( _device, INOUT desc.renderState, INOUT desc.dynamicState );
			
			// check topology
			CHECK_ERR(	uint(desc.renderState.inputAssembly.topology) < ppln_templ->_supportedTopology.size() and
						ppln_templ->_supportedTopology[uint(desc.renderState.inputAssembly.topology)] );

			desc_hash = desc.CalcHash();
		}

		// find in existing pipelines
		{
			SHAREDLOCK( ppln_templ->_pipelineMapGuard );

			for (auto iter = ppln_templ->_pipelineMap.find( desc_hash );
				 iter != ppln_templ->_pipelineMap.end() and iter->first == desc_hash;
				 ++iter)
			{
				auto*	rhs = GetResource( iter->second );

				if ( rhs and desc == rhs->Description() )
					return iter->second;
			}
		}

		// create new pipeline
		VkPipeline			ppln	= VK_NULL_HANDLE;
		VkPipelineLayout	layout	= VK_NULL_HANDLE;
		{
			VkGraphicsPipelineCreateInfo			pipeline_info		= {};
			VkPipelineInputAssemblyStateCreateInfo	input_assembly_info	= {};
			VkPipelineColorBlendStateCreateInfo		blend_info			= {};
			VkPipelineDepthStencilStateCreateInfo	depth_stencil_info	= {};
			VkPipelineMultisampleStateCreateInfo	multisample_info	= {};
			VkPipelineRasterizationStateCreateInfo	rasterization_info	= {};
			VkPipelineTessellationStateCreateInfo	tessellation_info	= {};
			VkPipelineDynamicStateCreateInfo		dynamic_state_info	= {};
			VkPipelineVertexInputStateCreateInfo	vertex_input_info	= {};
			VkPipelineViewportStateCreateInfo		viewport_info		= {};
			LinearAllocator<>						allocator;			allocator.SetBlockSize( 16_Kb );

			SetShaderStages( OUT pipeline_info.pStages, OUT pipeline_info.stageCount, ppln_templ->_shaders, allocator );
			SetDynamicState( OUT dynamic_state_info, desc.dynamicState, allocator );
			SetMultisampleState( OUT multisample_info, desc.renderState.multisample );
			SetTessellationState( OUT tessellation_info, ppln_templ->_patchControlPoints );
			SetDepthStencilState( OUT depth_stencil_info, desc.renderState.depth, desc.renderState.stencil );
			SetRasterizationState( OUT rasterization_info, desc.renderState.rasterization );
			SetupPipelineInputAssemblyState( OUT input_assembly_info, desc.renderState.inputAssembly );
			SetVertexInputState( OUT vertex_input_info, desc.vertexInput, allocator );
			//SetViewportState( OUT viewport_info, uint2(1024, 1024), desc.viewportCount, desc.dynamicState );
			//SetColorBlendState( OUT blend_info, OUT _tempAttachments, inst.renderState.color, *render_pass, inst.subpassIndex );	// TODO

			//pipeline_info.pColorBlendState	= &blend_info;
			//pipeline_info.renderPass			= render_pass->Handle();

			pipeline_info.sType					= VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			pipeline_info.basePipelineIndex		= -1;
			pipeline_info.basePipelineHandle	= VK_NULL_HANDLE;
			pipeline_info.pDepthStencilState	= &depth_stencil_info;
			pipeline_info.pDynamicState			= &dynamic_state_info;
			pipeline_info.pInputAssemblyState	= &input_assembly_info;
			pipeline_info.pMultisampleState		= &multisample_info;
			pipeline_info.pRasterizationState	= &rasterization_info;
			pipeline_info.pTessellationState	= (ppln_templ->_patchControlPoints > 0 ? &tessellation_info : null);
			pipeline_info.pVertexInputState		= &vertex_input_info;
			pipeline_info.layout				= layout;
			pipeline_info.subpass				= desc.subpassIndex;

			if ( not rasterization_info.rasterizerDiscardEnable )
			{
				pipeline_info.pViewportState		= &viewport_info;
			}else{
				pipeline_info.pViewportState		= null;
				pipeline_info.pMultisampleState		= null;
				pipeline_info.pDepthStencilState	= null;
				pipeline_info.pColorBlendState		= null;
			}

			VK_CHECK( _device.vkCreateGraphicsPipelines( _device.GetVkDevice(), VK_NULL_HANDLE, 1, &pipeline_info, null, OUT &ppln ));
		}

		// create pipeline wrapper
		GraphicsPipelineID	id;
		CHECK_ERR( _Assign( OUT id ));

		auto&	data = _GetResourcePool( id )[ id.Index() ];
		Replace( data );
		
		if ( not data.Create( templ_id, desc, ppln, layout ))
		{
			_Unassign( id );
			RETURN_ERR( "failed to create graphics pipeline" );
		}
		
		data.AddRef();
		return id;
	}
	
/*
=================================================
	GetMeshPipeline
=================================================
*
	MeshPipelineID  VResourceManager::GetMeshPipeline (const PipelineName &name)
	{
		auto	iter = _pipelineRefs.mesh.find( name );
		return iter != _pipelineRefs.mesh.end() ? iter->second : Default;
	}
	
/*
=================================================
	GetComputePipeline
=================================================
*
	ComputePipelineID  VResourceManager::GetComputePipeline (const PipelineName &name)
	{
		auto	iter = _pipelineRefs.compute.find( name );
		return iter != _pipelineRefs.compute.end() ? iter->second : Default;
	}
	
/*
=================================================
	GetRayTracingPipeline
=================================================
*
	RayTracingPipelineID  VResourceManager::GetRayTracingPipeline (const PipelineName &name)
	{
		auto	iter = _pipelineRefs.rayTracing.find( name );
		return iter != _pipelineRefs.rayTracing.end() ? iter->second : Default;
	}

/*
=================================================
	LoadPipelinePack
=================================================
*/
	PipelinePackID  VResourceManager::LoadPipelinePack (const SharedPtr<RStream> &stream)
	{
		PipelinePackID	id;
		CHECK_ERR( _Assign( OUT id ));

		auto&	data = _GetResourcePool( id )[ id.Index() ];
		Replace( data );
		
		if ( not data.Create( *this, stream, INOUT _pipelineRefs ))
		{
			_Unassign( id );
			RETURN_ERR( "failed when loading pipeline pack" );
		}
		
		data.AddRef();
		return id;
	}
	
/*
=================================================
	ReloadPipelinePack
=================================================
*/
	bool  VResourceManager::ReloadPipelinePack (const SharedPtr<RStream> &stream, PipelinePackID id)
	{
		// TODO
		return false;
	}
	
/*
=================================================
	CreateDescriptorSetLayout
=================================================
*/
	VDescriptorSetLayoutID  VResourceManager::CreateDescriptorSetLayout (const VDescriptorSetLayout::Uniforms_t &uniforms, ArrayView<VkSampler> samplerStorage, StringView dbgName)
	{
		VDescriptorSetLayoutID	id;
		CHECK_ERR( _Assign( OUT id ));

		auto&	data = _GetResourcePool( id )[ id.Index() ];
		Replace( data );
		
		if ( not data.Create( GetDevice(), uniforms, samplerStorage, dbgName ))
		{
			_Unassign( id );
			RETURN_ERR( "failed when creating descriptor set layout" );
		}
		
		data.AddRef();
		return id;
	}
	
/*
=================================================
	_CreateEmptyDescriptorSetLayout
=================================================
*/
	bool  VResourceManager::_CreateEmptyDescriptorSetLayout ()
	{
		VDescriptorSetLayoutID	id;
		CHECK_ERR( _Assign( OUT id ));

		auto&	data = _GetResourcePool( id )[ id.Index() ];
		Replace( data );
		
		if ( not data.Create( GetDevice(), "EmptyDSLayout" ))
		{
			_Unassign( id );
			RETURN_ERR( "failed when creating empty descriptor set layout" );
		}
		
		data.AddRef();

		_emptyDSLayout = id;
		return true;
	}

/*
=================================================
	CreatePipelineLayout
=================================================
*/
	VPipelineLayoutID  VResourceManager::CreatePipelineLayout (const VPipelineLayout::DescriptorSets_t &descSetLayouts, const VPipelineLayout::PushConstants_t &pusConstants, StringView dbgName)
	{
		VPipelineLayoutID	id;
		CHECK_ERR( _Assign( OUT id ));

		auto&	data = _GetResourcePool( id )[ id.Index() ];
		Replace( data );
		
		auto*	empty_ds = GetResource( _emptyDSLayout );
		CHECK_ERR( empty_ds );

		if ( not data.Create( GetDevice(), descSetLayouts, pusConstants, empty_ds->Handle(), dbgName ))
		{
			_Unassign( id );
			RETURN_ERR( "failed when creating pipeline layout" );
		}
		
		data.AddRef();
		return id;
	}
	
/*
=================================================
	CreateGPTemplate
=================================================
*/
	VGraphicsPipelineTemplateID  VResourceManager::CreateGPTemplate (VPipelineLayoutID layoutId, const PipelineCompiler::GraphicsPipelineDesc &desc,
																	 ArrayView<ShaderModule> modules, StringView dbgName)
	{
		VGraphicsPipelineTemplateID		id;
		CHECK_ERR( _Assign( OUT id ));

		auto&	data = _GetResourcePool( id )[ id.Index() ];
		Replace( data );
		
		auto*	empty_ds = GetResource( _emptyDSLayout );
		CHECK_ERR( empty_ds );

		if ( not data.Create( layoutId, desc, modules, dbgName ))
		{
			_Unassign( id );
			RETURN_ERR( "failed when creating graphics pipeline template" );
		}
		
		data.AddRef();
		return id;
	}
	
/*
=================================================
	CreateGPTemplate
=================================================
*/
	VMeshPipelineTemplateID  VResourceManager::CreateMPTemplate (VPipelineLayoutID layoutId, const PipelineCompiler::MeshPipelineDesc &desc,
																 ArrayView<ShaderModule> modules, StringView dbgName)
	{
		VMeshPipelineTemplateID		id;
		CHECK_ERR( _Assign( OUT id ));

		auto&	data = _GetResourcePool( id )[ id.Index() ];
		Replace( data );
		
		auto*	empty_ds = GetResource( _emptyDSLayout );
		CHECK_ERR( empty_ds );

		if ( not data.Create( layoutId, desc, modules, dbgName ))
		{
			_Unassign( id );
			RETURN_ERR( "failed when creating mesh pipeline template" );
		}
		
		data.AddRef();
		return id;
	}
	
/*
=================================================
	CreateGPTemplate
=================================================
*/
	VComputePipelineTemplateID  VResourceManager::CreateCPTemplate (VPipelineLayoutID layoutId, const PipelineCompiler::ComputePipelineDesc &desc,
																	VkShaderModule modules, StringView dbgName)
	{
		VComputePipelineTemplateID		id;
		CHECK_ERR( _Assign( OUT id ));

		auto&	data = _GetResourcePool( id )[ id.Index() ];
		Replace( data );
		
		auto*	empty_ds = GetResource( _emptyDSLayout );
		CHECK_ERR( empty_ds );

		if ( not data.Create( layoutId, desc, modules, dbgName ))
		{
			_Unassign( id );
			RETURN_ERR( "failed when creating compute pipeline template" );
		}
		
		data.AddRef();
		return id;
	}
	

}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
