// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#ifdef AE_ENABLE_VULKAN
# include "stl/Algorithms/StringUtils.h"
# include "Test_VulkanRenderGraph.h"

namespace
{

	struct DeviceFnTable
	{
#		define VKLOADER_STAGE_FNPOINTER
#		 include "vulkan_loader/fn_vulkan_dev.h"
#		undef  VKLOADER_STAGE_FNPOINTER
	};
	STATIC_ASSERT( sizeof(DeviceFnTable) == sizeof(VulkanDeviceFnTable) );
	


	//
	// Vulkan Logger
	//
	struct VulkanLogger : public VulkanDeviceFn
	{
	// types
	public:
		struct ImageData
		{
			VkImage					image	= VK_NULL_HANDLE;
			String					name	= {};
			VkImageCreateInfo		info	= {};
		};
		using ImageMap_t = HashMap< VkImage, ImageData >;


		struct ImageViewData
		{
			VkImageView				view	= VK_NULL_HANDLE;
			String					name	= {};
			ImageData const*		image	= null;
			VkImageViewCreateInfo	info	= {};
		};
		using ImageViewMap_t = HashMap< VkImageView, ImageViewData >;


		struct BufferData
		{
			VkBuffer				buffer	= VK_NULL_HANDLE;
			String					name	= {};
			VkBufferCreateInfo		info	= {};
		};
		using BufferMap_t = HashMap< VkBuffer, BufferData >;


		struct BufferViewData
		{
			VkBufferView			view	= VK_NULL_HANDLE;
			String					name	= {};
			BufferData const*		buffer	= null;
			VkBufferViewCreateInfo	info	= {};
		};
		using BufferViewMap_t = HashMap< VkBufferView, BufferViewData >;
		

		struct FramebufferData
		{
			VkFramebuffer			fb		= VK_NULL_HANDLE;
			String					name;
			VkFramebufferCreateInfo	info	= {};
			Array<VkImageView>		attachments;
		};
		using FramebufferMap_t = HashMap< VkFramebuffer, FramebufferData >;


		struct RenderPassData
		{
			VkRenderPass					rp     = VK_NULL_HANDLE;
			String							name;
			VkRenderPassCreateInfo			info   = {};
			Array<VkAttachmentDescription>	attachments;
			Array<VkSubpassDescription>		subpasses;
			Array<VkSubpassDependency>		dependencies;
			Array<VkAttachmentReference>	references;
			Array<uint>						preserve;
		};
		using RenderPassMap_t = HashMap< VkRenderPass, RenderPassData >;


		struct PipelineData
		{
			enum class EType
			{
				Unknown,
				Graphics,
				Compute,
				Mesh,
				RayTracing,
			};

			VkPipeline	pipeline	= VK_NULL_HANDLE;
			EType		type		= EType::Unknown;
			String		name;
		};
		using PipelineMap_t = HashMap< VkPipeline, PipelineData >;


		struct CommandBufferData
		{
			enum class EState
			{
				Initial,
				Recording,
				Pending,
			};

			VkCommandBuffer		cmdBuffer		= VK_NULL_HANDLE;
			EState				state			= EState::Initial;
			VkRenderPass		currentRP		= VK_NULL_HANDLE;
			VkFramebuffer		currentFB		= VK_NULL_HANDLE;
			uint				subpassIndex	= 0;
			String				log;
			String				name;
		};
		using CommandBufferMap_t = HashMap< VkCommandBuffer, CommandBufferData >;
		

		struct ResourceStatistic
		{
			usize	bufferCount			= 0;
			usize	imageCount			= 0;
			usize	bufferViewCount		= 0;
			usize	imageViewCount		= 0;
			usize	pipelineCount		= 0;
			usize	framebufferCount	= 0;
			usize	renderPassCount		= 0;
			usize	commandBufferCount	= 0;
		};


	// variables
	public:
		Mutex					guard;
		bool					enableLog	= false;
		String					log;
		CommandBufferMap_t		commandBuffers;
		ImageMap_t				imageMap;
		ImageViewMap_t			imageViewMap;
		BufferMap_t				bufferMap;
		BufferViewMap_t			bufferViewMap;
		FramebufferMap_t		framebufferMap;
		RenderPassMap_t			renderPassMap;
		PipelineMap_t			pipelineMap;
		ResourceStatistic		resourceStat;

	private:
		DeviceFnTable			_originDeviceFnTable;


	// methods
	public:
		void  Initialize (INOUT VulkanDeviceFnTable& fnTable);
		void  Deinitialize (OUT VulkanDeviceFnTable& fnTable);

		ND_ static VulkanLogger&  Get ()
		{
			static std::aligned_storage_t< sizeof(VulkanLogger), alignof(VulkanLogger) >	logger;
			return *Cast<VulkanLogger>( &logger );
		}
	};
	


/*
=================================================
	VkPipelineStageToString
=================================================
*/
	ND_ static String  VkPipelineStageToString (VkPipelineStageFlags stages)
	{
		if ( stages == VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT )
			return "ALL_GRAPHICS";

		if ( stages == VK_PIPELINE_STAGE_ALL_COMMANDS_BIT )
			return "ALL_COMMANDS";

		String	result;
		while ( stages )
		{
			VkAccessFlags	bit = ExtractBit( INOUT stages );

			if ( result.size() )
				result += " | ";

			#define CASE( _name_, _suffix_ ) \
				case VK_PIPELINE_STAGE_##_name_##_BIT##_suffix_ :  result << #_name_;  break

			//BEGIN_ENUM_CHECKS();
			switch ( VkPipelineStageFlagBits(bit) )
			{
				CASE( TOP_OF_PIPE, );
				CASE( DRAW_INDIRECT, );
				CASE( VERTEX_INPUT, );
				CASE( VERTEX_SHADER, );
				CASE( TESSELLATION_CONTROL_SHADER, );
				CASE( TESSELLATION_EVALUATION_SHADER, );
				CASE( GEOMETRY_SHADER, );
				CASE( FRAGMENT_SHADER, );
				CASE( EARLY_FRAGMENT_TESTS, );
				CASE( LATE_FRAGMENT_TESTS, );
				CASE( COLOR_ATTACHMENT_OUTPUT, );
				CASE( COMPUTE_SHADER, );
				CASE( TRANSFER, );
				CASE( BOTTOM_OF_PIPE, );
				CASE( HOST, );
				CASE( ALL_GRAPHICS, );
				CASE( ALL_COMMANDS, );
				CASE( TRANSFORM_FEEDBACK, _EXT);
				CASE( CONDITIONAL_RENDERING, _EXT);
				CASE( SHADING_RATE_IMAGE, _NV);
				CASE( RAY_TRACING_SHADER, _NV);
				CASE( ACCELERATION_STRUCTURE_BUILD, _NV);
				CASE( TASK_SHADER, _NV);
				CASE( MESH_SHADER, _NV);
				CASE( FRAGMENT_DENSITY_PROCESS, _EXT);
				case VK_PIPELINE_STAGE_FLAG_BITS_MAX_ENUM:
				default:	break;
			}
			//END_ENUM_CHECKS();
			#undef CASE
		}
		return result.size() ? result : "0";
	}
	
/*
=================================================
	VkAccessMaskToString
=================================================
*/
	ND_ static String  VkAccessMaskToString (VkAccessFlags flags)
	{
		String	result;
		while ( flags != 0 )
		{
			VkAccessFlags	bit = ExtractBit( INOUT flags );

			if ( result.size() )
				result += " | ";

			#define CASE( _name_, _suffix_ ) \
				case VK_ACCESS_##_name_##_BIT##_suffix_ :  result << #_name_;  break

			//BEGIN_ENUM_CHECKS();
			switch ( VkAccessFlagBits(bit) )
			{
				CASE( INDIRECT_COMMAND_READ, );
				CASE( INDEX_READ, );
				CASE( VERTEX_ATTRIBUTE_READ, );
				CASE( UNIFORM_READ, );
				CASE( INPUT_ATTACHMENT_READ, );
				CASE( SHADER_READ, );
				CASE( SHADER_WRITE, );
				CASE( COLOR_ATTACHMENT_READ, );
				CASE( COLOR_ATTACHMENT_WRITE, );
				CASE( DEPTH_STENCIL_ATTACHMENT_READ, );
				CASE( DEPTH_STENCIL_ATTACHMENT_WRITE, );
				CASE( TRANSFER_READ, );
				CASE( TRANSFER_WRITE, );
				CASE( HOST_READ, );
				CASE( HOST_WRITE, );
				CASE( MEMORY_READ, );
				CASE( MEMORY_WRITE, );
				CASE( TRANSFORM_FEEDBACK_WRITE, _EXT);
				CASE( TRANSFORM_FEEDBACK_COUNTER_READ, _EXT);
				CASE( TRANSFORM_FEEDBACK_COUNTER_WRITE, _EXT);
				CASE( CONDITIONAL_RENDERING_READ, _EXT);
				CASE( COLOR_ATTACHMENT_READ_NONCOHERENT, _EXT);
				CASE( SHADING_RATE_IMAGE_READ, _NV);
				CASE( ACCELERATION_STRUCTURE_READ, _NV);
				CASE( ACCELERATION_STRUCTURE_WRITE, _NV);
				CASE( FRAGMENT_DENSITY_MAP_READ, _EXT);
				case VK_ACCESS_FLAG_BITS_MAX_ENUM:
				default:	break;
			}
			//END_ENUM_CHECKS();
			#undef CASE
		}
		return result.size() ? result : "0";
	}
	
/*
=================================================
	VkImageLayoutToString
=================================================
*/
	ND_ static String  VkImageLayoutToString (VkImageLayout layout)
	{
		#define CASE( _name_, _suffix_ ) \
			case VK_IMAGE_LAYOUT_##_name_##_suffix_ : return #_name_

		BEGIN_ENUM_CHECKS();
		switch ( layout )
		{
			CASE( UNDEFINED, );
			CASE( GENERAL, );
			CASE( COLOR_ATTACHMENT_OPTIMAL, );
			CASE( DEPTH_STENCIL_ATTACHMENT_OPTIMAL, );
			CASE( DEPTH_STENCIL_READ_ONLY_OPTIMAL, );
			CASE( SHADER_READ_ONLY_OPTIMAL, );
			CASE( TRANSFER_SRC_OPTIMAL, );
			CASE( TRANSFER_DST_OPTIMAL, );
			CASE( PREINITIALIZED, );
			CASE( DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL, );
			CASE( DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL, );
			CASE( PRESENT_SRC, _KHR);
			CASE( SHARED_PRESENT, _KHR);
			CASE( SHADING_RATE_OPTIMAL, _NV);
			CASE( FRAGMENT_DENSITY_MAP_OPTIMAL, _EXT);
			CASE( DEPTH_ATTACHMENT_OPTIMAL, _KHR);
			CASE( DEPTH_READ_ONLY_OPTIMAL, _KHR);
			CASE( STENCIL_ATTACHMENT_OPTIMAL, _KHR);
			CASE( STENCIL_READ_ONLY_OPTIMAL, _KHR);
			#ifndef VK_VERSION_1_2
			case VK_IMAGE_LAYOUT_RANGE_SIZE:
			#endif
			case VK_IMAGE_LAYOUT_MAX_ENUM:
			default :	return "unknown";
		}
		END_ENUM_CHECKS();
		#undef CASE
	}
	
/*
=================================================
	VkSampleCountToString
=================================================
*/
	ND_ static String  VkSampleCountToString (VkSampleCountFlagBits samples)
	{
		BEGIN_ENUM_CHECKS();
		switch ( samples )
		{
			case VK_SAMPLE_COUNT_1_BIT :	return "1";
			case VK_SAMPLE_COUNT_2_BIT :	return "2";
			case VK_SAMPLE_COUNT_4_BIT :	return "4";
			case VK_SAMPLE_COUNT_8_BIT :	return "8";
			case VK_SAMPLE_COUNT_16_BIT :	return "16";
			case VK_SAMPLE_COUNT_32_BIT :	return "32";
			case VK_SAMPLE_COUNT_64_BIT :	return "64";
			case VK_SAMPLE_COUNT_FLAG_BITS_MAX_ENUM :
			default:						return "unknown";
		}
		END_ENUM_CHECKS();
	}
	
/*
=================================================
	VkAttachmentLoadOpToString
=================================================
*/
	ND_ static String  VkAttachmentLoadOpToString (VkAttachmentLoadOp op)
	{
		#define CASE( _name_, _suffix_ ) \
			case VK_ATTACHMENT_LOAD_OP_##_name_##_suffix_ :  return #_name_

		BEGIN_ENUM_CHECKS();
		switch (op)
		{
			CASE( LOAD, );
			CASE( CLEAR, );
			CASE( DONT_CARE, );
			#ifndef VK_VERSION_1_2
			case VK_ATTACHMENT_LOAD_OP_RANGE_SIZE :
			#endif
			case VK_ATTACHMENT_LOAD_OP_MAX_ENUM :
			default:	return "unknown";
		}
		END_ENUM_CHECKS();
		#undef CASE
	}
	
/*
=================================================
	VkAttachmentStoreOpToString
=================================================
*/
	ND_ static String  VkAttachmentStoreOpToString (VkAttachmentStoreOp op)
	{
		#define CASE( _name_, _suffix_ ) \
			case VK_ATTACHMENT_STORE_OP_##_name_##_suffix_ :  return #_name_

		BEGIN_ENUM_CHECKS();
		switch (op)
		{
			CASE( STORE, );
			CASE( DONT_CARE, );
			
			#ifdef VK_QCOM_render_pass_store_ops
			CASE( NONE, _QCOM );
			#endif
			#ifndef VK_VERSION_1_2
			case VK_ATTACHMENT_STORE_OP_RANGE_SIZE :
			#endif
			case VK_ATTACHMENT_STORE_OP_MAX_ENUM :
			default:	return "unknown";
		}
		END_ENUM_CHECKS();
		#undef CASE
	}
	
/*
=================================================
	VkAttachmentStoreOpToString
=================================================
*/
	ND_ static String  VkImageAspectToString (VkImageAspectFlags flags)
	{
		#define CASE( _name_, _suffix_ ) \
			case VK_IMAGE_ASPECT_##_name_##_suffix_ :  return #_name_

		String	result;
		while ( flags != 0 )
		{
			VkImageAspectFlags	bit = ExtractBit( INOUT flags );

			if ( result.size() )
				result += " | ";
			
			BEGIN_ENUM_CHECKS();
			switch ( VkImageAspectFlagBits(bit) )
			{
				CASE( COLOR, _BIT );
				CASE( DEPTH, _BIT );
				CASE( STENCIL, _BIT );
				CASE( METADATA, _BIT );
				CASE( PLANE_0, _BIT );
				CASE( PLANE_1, _BIT );
				CASE( PLANE_2, _BIT );
				CASE( MEMORY_PLANE_0, _BIT_EXT );
				CASE( MEMORY_PLANE_1, _BIT_EXT );
				CASE( MEMORY_PLANE_2, _BIT_EXT );
				CASE( MEMORY_PLANE_3, _BIT_EXT );
				case VK_IMAGE_ASPECT_FLAG_BITS_MAX_ENUM :
				default:	return "unknown";
			}
			END_ENUM_CHECKS();
		}
		return result;
	}
	
/*
=================================================
	Wrap_vkCreateBuffer
=================================================
*/
	VkResult VKAPI_CALL Wrap_vkCreateBuffer (VkDevice device, const VkBufferCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkBuffer* pBuffer)
	{
		auto&	logger = VulkanLogger::Get();
		EXLOCK( logger.guard );

		VkResult	res	= logger.vkCreateBuffer( device, pCreateInfo, pAllocator, pBuffer );
		auto&		buf	= logger.bufferMap.insert_or_assign( *pBuffer, VulkanLogger::BufferData{} ).first->second;

		buf.buffer	= *pBuffer;
		buf.info	= *pCreateInfo;
		buf.name	= "buffer-" + ToString( logger.resourceStat.bufferCount );

		++logger.resourceStat.bufferCount;
		return res;
	}
	
/*
=================================================
	Wrap_vkDestroyBuffer
=================================================
*/
	void VKAPI_CALL Wrap_vkDestroyBuffer (VkDevice device, VkBuffer buffer, const VkAllocationCallbacks* pAllocator)
	{
		auto&	logger = VulkanLogger::Get();
		EXLOCK( logger.guard );

		logger.vkDestroyBuffer( device, buffer, pAllocator );
		logger.bufferMap.erase( buffer );
	}
	
/*
=================================================
	Wrap_vkCreateBufferView
=================================================
*/
	VkResult VKAPI_CALL Wrap_vkCreateBufferView (VkDevice device, const VkBufferViewCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkBufferView* pView)
	{
		auto&	logger = VulkanLogger::Get();
		EXLOCK( logger.guard );

		VkResult	res		= logger.vkCreateBufferView( device, pCreateInfo, pAllocator, pView );
		auto&		view	= logger.bufferViewMap.insert_or_assign( *pView, VulkanLogger::BufferViewData{} ).first->second;

		view.view	= *pView;
		view.info	= *pCreateInfo;
		view.buffer	= &logger.bufferMap.find( pCreateInfo->buffer )->second;
		view.name	= "buffer-view-" + ToString( logger.resourceStat.bufferViewCount ) + " (" + view.buffer->name + ")";

		++logger.resourceStat.bufferViewCount;
		return res;
	}
	
/*
=================================================
	Wrap_vkDestroyBufferView
=================================================
*/
	void VKAPI_CALL Wrap_vkDestroyBufferView (VkDevice device, VkBufferView bufferView, const VkAllocationCallbacks* pAllocator)
	{
		auto&	logger = VulkanLogger::Get();
		EXLOCK( logger.guard );

		logger.vkDestroyBufferView( device, bufferView, pAllocator );
		logger.bufferViewMap.erase( bufferView );
	}
	
/*
=================================================
	Wrap_vkCreateImage
=================================================
*/
	VkResult VKAPI_CALL Wrap_vkCreateImage (VkDevice device, const VkImageCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkImage* pImage)
	{
		auto&	logger = VulkanLogger::Get();
		EXLOCK( logger.guard );

		VkResult	res	= logger.vkCreateImage( device, pCreateInfo, pAllocator, pImage );
		auto&		img	= logger.imageMap.insert_or_assign( *pImage, VulkanLogger::ImageData{} ).first->second;

		img.image	= *pImage;
		img.info	= *pCreateInfo;
		img.name	= "image-" + ToString( logger.resourceStat.imageCount );

		++logger.resourceStat.imageCount;
		return res;
	}
	
/*
=================================================
	Wrap_vkDestroyImage
=================================================
*/
	void VKAPI_CALL Wrap_vkDestroyImage (VkDevice device, VkImage image, const VkAllocationCallbacks* pAllocator)
	{
		auto&	logger = VulkanLogger::Get();
		EXLOCK( logger.guard );

		logger.vkDestroyImage( device, image, pAllocator );
		logger.imageMap.erase( image );
	}
	
/*
=================================================
	Wrap_vkCreateImageView
=================================================
*/
	VkResult VKAPI_CALL Wrap_vkCreateImageView (VkDevice device, const VkImageViewCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkImageView* pView)
	{
		auto&	logger = VulkanLogger::Get();
		EXLOCK( logger.guard );

		VkResult	res		= logger.vkCreateImageView(device, pCreateInfo, pAllocator, pView);
		auto&		view	= logger.imageViewMap.insert_or_assign(*pView, VulkanLogger::ImageViewData{}).first->second;

		view.view	= *pView;
		view.image	= &logger.imageMap.find( pCreateInfo->image )->second;
		view.info	= *pCreateInfo;
		view.name	= "image-view-" + ToString( logger.resourceStat.imageViewCount ) + " (" + view.image->name + ")";

		++logger.resourceStat.imageViewCount;
		return res;
	}
	
/*
=================================================
	Wrap_vkDestroyImageView
=================================================
*/
	void VKAPI_CALL Wrap_vkDestroyImageView (VkDevice device, VkImageView imageView, const VkAllocationCallbacks* pAllocator)
	{
		auto&	logger = VulkanLogger::Get();
		EXLOCK( logger.guard );

		logger.vkDestroyImageView( device, imageView, pAllocator );
		logger.imageViewMap.erase( imageView );
	}
	
/*
=================================================
	Wrap_vkCreateFramebuffer
=================================================
*/
	VkResult VKAPI_CALL Wrap_vkCreateFramebuffer (VkDevice device, const VkFramebufferCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkFramebuffer* pFramebuffer)
	{
		auto&	logger = VulkanLogger::Get();
		EXLOCK( logger.guard );

		VkResult	res	= logger.vkCreateFramebuffer( device, pCreateInfo, pAllocator, pFramebuffer );
		auto&		fb	= logger.framebufferMap.insert_or_assign( *pFramebuffer, VulkanLogger::FramebufferData{} ).first->second;

		fb.fb	= *pFramebuffer;
		fb.name	= "framebuffer-" + ToString( logger.resourceStat.framebufferCount );
		fb.info	= *pCreateInfo;

		fb.attachments.assign( pCreateInfo->pAttachments, pCreateInfo->pAttachments + pCreateInfo->attachmentCount );
		fb.info.pAttachments = fb.attachments.data();

		++logger.resourceStat.framebufferCount;
		return res;
	}
	
/*
=================================================
	Wrap_vkDestroyFramebuffer
=================================================
*/
	void VKAPI_CALL Wrap_vkDestroyFramebuffer (VkDevice device, VkFramebuffer framebuffer, const VkAllocationCallbacks* pAllocator)
	{
		auto&	logger = VulkanLogger::Get();
		EXLOCK( logger.guard );

		logger.vkDestroyFramebuffer( device, framebuffer, pAllocator );
		logger.framebufferMap.erase( framebuffer );
	}
	
/*
=================================================
	Wrap_vkCreateRenderPass
=================================================
*/
	VkResult VKAPI_CALL Wrap_vkCreateRenderPass (VkDevice device, const VkRenderPassCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkRenderPass* pRenderPass)
	{
		auto&	logger = VulkanLogger::Get();
		EXLOCK( logger.guard );

		VkResult	res	= logger.vkCreateRenderPass( device, pCreateInfo, pAllocator, pRenderPass );
		auto&		rp	= logger.renderPassMap.insert_or_assign( *pRenderPass, VulkanLogger::RenderPassData{} ).first->second;

		rp.rp	= *pRenderPass;
		rp.info	= *pCreateInfo;
		rp.name	= "render-pass-" + ToString( logger.resourceStat.renderPassCount );
		++logger.resourceStat.renderPassCount;

		rp.attachments.assign(pCreateInfo->pAttachments, pCreateInfo->pAttachments + pCreateInfo->attachmentCount);
		rp.dependencies.assign(pCreateInfo->pDependencies, pCreateInfo->pDependencies + pCreateInfo->dependencyCount);
		rp.subpasses.assign(pCreateInfo->pSubpasses, pCreateInfo->pSubpasses + pCreateInfo->subpassCount);
		rp.info.pAttachments	= rp.attachments.data();
		rp.info.pDependencies	= rp.dependencies.data();
		rp.info.pSubpasses		= rp.subpasses.data();

		usize	ref_count		= 0;
		usize	preserve_count	= 0;
		for (auto& sp : rp.subpasses)
		{
			ref_count += sp.colorAttachmentCount + (sp.pResolveAttachments ? sp.colorAttachmentCount : 0) + sp.inputAttachmentCount + (sp.pDepthStencilAttachment != null);
			preserve_count += sp.preserveAttachmentCount;
		}

		rp.references.resize( ref_count );
		ref_count = 0;

		rp.preserve.resize( preserve_count );
		preserve_count = 0;

		for (auto& sp : rp.subpasses)
		{
			std::memcpy( rp.references.data() + ref_count, sp.pInputAttachments, sizeof(*sp.pInputAttachments) * sp.inputAttachmentCount );
			sp.pInputAttachments	 = rp.references.data() + ref_count;
			ref_count				+= sp.inputAttachmentCount;

			std::memcpy( rp.references.data() + ref_count, sp.pColorAttachments, sizeof(*sp.pColorAttachments) * sp.colorAttachmentCount );
			sp.pColorAttachments	 = rp.references.data() + ref_count;
			ref_count				+= sp.colorAttachmentCount;

			if ( sp.pResolveAttachments != null )
			{
				std::memcpy( rp.references.data() + ref_count, sp.pResolveAttachments, sizeof(*sp.pResolveAttachments) * sp.colorAttachmentCount );
				sp.pResolveAttachments	 = rp.references.data() + ref_count;
				ref_count				+= sp.colorAttachmentCount;
			}

			if ( sp.pDepthStencilAttachment != null )
			{
				rp.references[ref_count]	= *sp.pDepthStencilAttachment;
				sp.pDepthStencilAttachment	= &rp.references[ref_count];
				++ref_count;
			}

			std::memcpy( rp.preserve.data() + preserve_count, sp.pPreserveAttachments, sizeof(*sp.pPreserveAttachments) * sp.preserveAttachmentCount );
			sp.pPreserveAttachments	 = rp.preserve.data() + preserve_count;
			preserve_count			+= sp.preserveAttachmentCount;
		}

		return res;
	}
	
/*
=================================================
	Wrap_vkDestroyRenderPass
=================================================
*/
	void VKAPI_CALL Wrap_vkDestroyRenderPass (VkDevice device, VkRenderPass renderPass, const VkAllocationCallbacks* pAllocator)
	{
		auto&	logger = VulkanLogger::Get();
		EXLOCK( logger.guard );

		logger.vkDestroyRenderPass( device, renderPass, pAllocator );
		logger.renderPassMap.erase( renderPass );
	}
	
/*
=================================================
	Wrap_vkCreateGraphicsPipelines
=================================================
*/
	VkResult VKAPI_CALL Wrap_vkCreateGraphicsPipelines (VkDevice device, VkPipelineCache pipelineCache, uint createInfoCount, const VkGraphicsPipelineCreateInfo* pCreateInfos, const VkAllocationCallbacks* pAllocator, VkPipeline* pPipelines)
	{
		auto&	logger = VulkanLogger::Get();
		EXLOCK( logger.guard );

		VkResult res = logger.vkCreateGraphicsPipelines( device, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines );

		for (uint i = 0; i < createInfoCount; ++i)
		{
			auto& ppln		= logger.pipelineMap.insert_or_assign( pPipelines[i], VulkanLogger::PipelineData{} ).first->second;
			ppln.pipeline	= pPipelines[i];
			ppln.type		= VulkanLogger::PipelineData::EType::Graphics;
			ppln.name		= "pipeline-" + ToString( logger.resourceStat.pipelineCount );
			++logger.resourceStat.pipelineCount;
		}

		return res;
	}
	
/*
=================================================
	Wrap_vkCreateComputePipelines
=================================================
*/
	VkResult VKAPI_CALL Wrap_vkCreateComputePipelines (VkDevice device, VkPipelineCache pipelineCache, uint createInfoCount, const VkComputePipelineCreateInfo* pCreateInfos, const VkAllocationCallbacks* pAllocator, VkPipeline* pPipelines)
	{
		auto&	logger = VulkanLogger::Get();
		EXLOCK( logger.guard );

		VkResult res = logger.vkCreateComputePipelines( device, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines );

		for (uint i = 0; i < createInfoCount; ++i)
		{
			auto& ppln		= logger.pipelineMap.insert_or_assign( pPipelines[i], VulkanLogger::PipelineData{} ).first->second;
			ppln.pipeline	= pPipelines[i];
			ppln.type		= VulkanLogger::PipelineData::EType::Compute;
			ppln.name		= "pipeline-" + ToString( logger.resourceStat.pipelineCount );
			++logger.resourceStat.pipelineCount;
		}

		return res;
	}
	
/*
=================================================
	Wrap_vkDestroyPipeline
=================================================
*/
	void VKAPI_CALL Wrap_vkDestroyPipeline (VkDevice device, VkPipeline pipeline, const VkAllocationCallbacks* pAllocator)
	{
		auto&	logger = VulkanLogger::Get();
		EXLOCK( logger.guard );

		logger.vkDestroyPipeline( device, pipeline, pAllocator );
		logger.pipelineMap.erase( pipeline );
	}
	
/*
=================================================
	Wrap_vkAllocateCommandBuffers
=================================================
*/
	VkResult VKAPI_CALL Wrap_vkAllocateCommandBuffers (VkDevice device, const VkCommandBufferAllocateInfo* pAllocateInfo, VkCommandBuffer* pCommandBuffers)
	{
		auto&	logger = VulkanLogger::Get();
		EXLOCK( logger.guard );

		VkResult res = logger.vkAllocateCommandBuffers( device, pAllocateInfo, pCommandBuffers );

		for (uint i = 0; i < pAllocateInfo->commandBufferCount; ++i)
		{
			auto&	cmdbuf		= logger.commandBuffers.insert_or_assign( pCommandBuffers[i], VulkanLogger::CommandBufferData{} ).first->second;
			cmdbuf.cmdBuffer	= pCommandBuffers[i];
			cmdbuf.state		= VulkanLogger::CommandBufferData::EState::Initial;
			cmdbuf.name			= "command-buffer-" + ToString( logger.resourceStat.commandBufferCount );
			++logger.resourceStat.commandBufferCount;
		}
		return res;
	}
	
/*
=================================================
	Wrap_vkBeginCommandBuffer
=================================================
*/
	VkResult VKAPI_CALL Wrap_vkBeginCommandBuffer (VkCommandBuffer commandBuffer, const VkCommandBufferBeginInfo* pBeginInfo)
	{
		auto&	logger = VulkanLogger::Get();
		EXLOCK( logger.guard );

		auto	iter = logger.commandBuffers.find( commandBuffer );
		if ( iter == logger.commandBuffers.end() )
			return VK_RESULT_MAX_ENUM;

		auto&	cmdbuf = iter->second;
		ASSERT( cmdbuf.cmdBuffer == commandBuffer );

		cmdbuf.state = VulkanLogger::CommandBufferData::EState::Recording;
		cmdbuf.log.clear();

		return logger.vkBeginCommandBuffer( commandBuffer, pBeginInfo );
	}
	
/*
=================================================
	Wrap_vkEndCommandBuffer
=================================================
*/
	VkResult VKAPI_CALL Wrap_vkEndCommandBuffer (VkCommandBuffer commandBuffer)
	{
		auto&	logger = VulkanLogger::Get();
		EXLOCK( logger.guard );
		
		auto	iter = logger.commandBuffers.find( commandBuffer );
		if ( iter == logger.commandBuffers.end() )
			return VK_RESULT_MAX_ENUM;

		auto&	cmdbuf = iter->second;
		ASSERT( cmdbuf.cmdBuffer == commandBuffer );

		cmdbuf.state = VulkanLogger::CommandBufferData::EState::Pending;

		return logger.vkEndCommandBuffer( commandBuffer );
	}
	
/*
=================================================
	Wrap_vkQueueSubmit
=================================================
*/
	VkResult VKAPI_CALL Wrap_vkQueueSubmit (VkQueue queue, uint submitCount, const VkSubmitInfo* pSubmits, VkFence fence)
	{
		auto&	logger = VulkanLogger::Get();
		EXLOCK( logger.guard );

		if ( logger.enableLog )
		{
			String	log;
			for (uint i = 0; i < submitCount; ++i)
			{
				auto&	batch = pSubmits[i];
				for (uint j = 0; j < batch.commandBufferCount; ++j)
				{
					auto	cmd_iter = logger.commandBuffers.find( batch.pCommandBuffers[j] );
					if ( cmd_iter == logger.commandBuffers.end() )
						return VK_RESULT_MAX_ENUM;

					auto&	cmdbuf = cmd_iter->second;
					cmdbuf.state = VulkanLogger::CommandBufferData::EState::Initial;

					log << "--------------------------------------------------\n";
					log << "name: " << cmdbuf.name << "\n{\n";
					log << cmdbuf.log;
					log << "}\n--------------------------------------------------\n";

					cmdbuf.log.clear();
				}
			}
			logger.log += log;
		}

		return logger.vkQueueSubmit( queue, submitCount, pSubmits, fence );
	}
	
/*
=================================================
	Wrap_vkSetDebugUtilsObjectNameEXT
=================================================
*/
	VkResult VKAPI_CALL Wrap_vkSetDebugUtilsObjectNameEXT (VkDevice device, const VkDebugUtilsObjectNameInfoEXT* pNameInfo)
	{
		auto&	logger = VulkanLogger::Get();
		EXLOCK( logger.guard );

		BEGIN_ENUM_CHECKS();
		switch ( pNameInfo->objectType )
		{
			case VK_OBJECT_TYPE_COMMAND_BUFFER :
			{
				auto	iter = logger.commandBuffers.find( VkCommandBuffer(pNameInfo->objectHandle) );
				if ( iter != logger.commandBuffers.end() )
					iter->second.name = pNameInfo->pObjectName;
				break;
			}

			case VK_OBJECT_TYPE_BUFFER :
			{
				auto	iter = logger.bufferMap.find( VkBuffer(pNameInfo->objectHandle) );
				if ( iter != logger.bufferMap.end() )
					iter->second.name = pNameInfo->pObjectName;
				break;
			}

			case VK_OBJECT_TYPE_BUFFER_VIEW :
			{
				auto	iter = logger.bufferViewMap.find( VkBufferView(pNameInfo->objectHandle) );
				if ( iter != logger.bufferViewMap.end() )
					iter->second.name = pNameInfo->pObjectName;
				break;
			}

			case VK_OBJECT_TYPE_IMAGE :
			{
				auto	iter = logger.imageMap.find( VkImage(pNameInfo->objectHandle) );
				if ( iter != logger.imageMap.end() )
					iter->second.name = pNameInfo->pObjectName;
				break;
			}

			case VK_OBJECT_TYPE_IMAGE_VIEW :
			{
				auto	iter = logger.imageViewMap.find( VkImageView(pNameInfo->objectHandle) );
				if ( iter != logger.imageViewMap.end() )
					iter->second.name = pNameInfo->pObjectName;
				break;
			}

			case VK_OBJECT_TYPE_RENDER_PASS :
			{
				auto	iter = logger.renderPassMap.find( VkRenderPass(pNameInfo->objectHandle) );
				if ( iter != logger.renderPassMap.end() )
					iter->second.name = pNameInfo->pObjectName;
				break;
			}

			case VK_OBJECT_TYPE_FRAMEBUFFER :
			{
				auto	iter = logger.framebufferMap.find( VkFramebuffer(pNameInfo->objectHandle) );
				if ( iter != logger.framebufferMap.end() )
					iter->second.name = pNameInfo->pObjectName;
				break;
			}

			case VK_OBJECT_TYPE_PIPELINE :
			{
				auto	iter = logger.pipelineMap.find( VkPipeline(pNameInfo->objectHandle) );
				if ( iter != logger.pipelineMap.end() )
					iter->second.name = pNameInfo->pObjectName;
				break;
			}

			case VK_OBJECT_TYPE_UNKNOWN :
			case VK_OBJECT_TYPE_DEVICE :
			case VK_OBJECT_TYPE_INSTANCE :
			case VK_OBJECT_TYPE_PHYSICAL_DEVICE :
			case VK_OBJECT_TYPE_QUEUE :
			case VK_OBJECT_TYPE_SEMAPHORE :
			case VK_OBJECT_TYPE_FENCE :
			case VK_OBJECT_TYPE_DEVICE_MEMORY :
			case VK_OBJECT_TYPE_EVENT :
			case VK_OBJECT_TYPE_QUERY_POOL :
			case VK_OBJECT_TYPE_SAMPLER :
			case VK_OBJECT_TYPE_DESCRIPTOR_POOL :
			case VK_OBJECT_TYPE_DESCRIPTOR_SET :
			case VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT :
			case VK_OBJECT_TYPE_SHADER_MODULE :
			case VK_OBJECT_TYPE_PIPELINE_CACHE :
			case VK_OBJECT_TYPE_PIPELINE_LAYOUT :
			case VK_OBJECT_TYPE_COMMAND_POOL :
			case VK_OBJECT_TYPE_SAMPLER_YCBCR_CONVERSION :
			case VK_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE :
			case VK_OBJECT_TYPE_SURFACE_KHR :
			case VK_OBJECT_TYPE_SWAPCHAIN_KHR :
			case VK_OBJECT_TYPE_DISPLAY_KHR :
			case VK_OBJECT_TYPE_DISPLAY_MODE_KHR :
			case VK_OBJECT_TYPE_DEBUG_REPORT_CALLBACK_EXT :
			case VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_NV :
			case VK_OBJECT_TYPE_DEBUG_UTILS_MESSENGER_EXT :
			case VK_OBJECT_TYPE_VALIDATION_CACHE_EXT :
			case VK_OBJECT_TYPE_PERFORMANCE_CONFIGURATION_INTEL :

			#ifdef VK_NV_device_generated_commands
			case VK_OBJECT_TYPE_INDIRECT_COMMANDS_LAYOUT_NV :
			#elif defined(VK_NVX_device_generated_commands)
			case VK_OBJECT_TYPE_OBJECT_TABLE_NVX:
			case VK_OBJECT_TYPE_INDIRECT_COMMANDS_LAYOUT_NVX:
			#endif
			#ifdef VK_EXT_private_data
			case VK_OBJECT_TYPE_PRIVATE_DATA_SLOT_EXT :
			#endif
			#ifdef VK_KHR_acceleration_structure
			case VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR :
			#endif
			#ifdef VK_KHR_deferred_host_operations
			case VK_OBJECT_TYPE_DEFERRED_OPERATION_KHR :
			#endif
			#ifndef VK_VERSION_1_2
			case VK_OBJECT_TYPE_RANGE_SIZE :
			#endif
			case VK_OBJECT_TYPE_MAX_ENUM :
				break;
		}
		END_ENUM_CHECKS();

		return logger.vkSetDebugUtilsObjectNameEXT( device, pNameInfo );
	}

/*
=================================================
	Wrap_vkCmdPipelineBarrier
=================================================
*/
	void VKAPI_CALL  Wrap_vkCmdPipelineBarrier (VkCommandBuffer commandBuffer, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags, uint memoryBarrierCount, const VkMemoryBarrier* pMemoryBarriers, uint bufferMemoryBarrierCount, const VkBufferMemoryBarrier* pBufferMemoryBarriers, uint imageMemoryBarrierCount, const VkImageMemoryBarrier* pImageMemoryBarriers)
	{
		auto&	logger = VulkanLogger::Get();
		EXLOCK( logger.guard );

		auto iter = logger.commandBuffers.find( commandBuffer );
		if ( iter == logger.commandBuffers.end() )
			return;

		auto&	cmdbuf = iter->second;
		ASSERT( cmdbuf.cmdBuffer == commandBuffer );

		logger.vkCmdPipelineBarrier( commandBuffer, srcStageMask, dstStageMask, dependencyFlags, memoryBarrierCount, pMemoryBarriers, bufferMemoryBarrierCount, pBufferMemoryBarriers, imageMemoryBarrierCount, pImageMemoryBarriers );

		if ( not logger.enableLog )
			return;

		auto&	log = cmdbuf.log;

		log << "  PipelineBarrier\n";
		log << "    stage: " << VkPipelineStageToString( srcStageMask ) << " ---> " << VkPipelineStageToString( dstStageMask ) << '\n';

		for (uint i = 0; i < memoryBarrierCount; ++i)
		{
			auto&	barrier = pMemoryBarriers[i];
			log << "    MemoryBarrier:\n";
			log << "      access: " << VkAccessMaskToString( barrier.srcAccessMask ) << " ---> " << VkAccessMaskToString( barrier.dstAccessMask ) << '\n';
		}

		for (uint i = 0; i < bufferMemoryBarrierCount; ++i)
		{
			auto&	barrier = pBufferMemoryBarriers[i];
			log << "    BufferBarrier:\n";

			auto	buf = logger.bufferMap.find( barrier.buffer );

			if ( buf != logger.bufferMap.end() )
				log << "      name:   " << buf->second.name << '\n';

			log << "      access: " << VkAccessMaskToString( barrier.srcAccessMask ) << " ---> " << VkAccessMaskToString( barrier.dstAccessMask ) << '\n';

			if ( not (barrier.offset == 0 and barrier.size == VK_WHOLE_SIZE) )
				log << "      range:  [" << ToString( Bytes{barrier.offset} ) << ", " << (barrier.size == VK_WHOLE_SIZE ? "whole" : ToString( Bytes{barrier.offset + barrier.size} )) << ")\n";
		}

		for (uint i = 0; i < imageMemoryBarrierCount; ++i)
		{
			auto&	barrier = pImageMemoryBarriers[i];
			log << "    ImageBarrier:\n";

			auto	img = logger.imageMap.find( barrier.image );
			if ( img != logger.imageMap.end() )
				log << "      name:    " << img->second.name << "\n";

			log << "      layout:  " << VkImageLayoutToString( barrier.oldLayout );

			if ( barrier.oldLayout != barrier.newLayout )
				log << " ---> " << VkImageLayoutToString( barrier.newLayout );

			log << '\n';
			log << "      access:  " << VkAccessMaskToString( barrier.srcAccessMask ) << " ---> " << VkAccessMaskToString( barrier.dstAccessMask ) << '\n';
			log << "      aspect:  " << VkImageAspectToString( barrier.subresourceRange.aspectMask ) << '\n';

			if ( not (barrier.subresourceRange.baseMipLevel == 0 and barrier.subresourceRange.levelCount == VK_REMAINING_MIP_LEVELS) )
			{
				log << "      mipmaps: [" << ToString( barrier.subresourceRange.baseMipLevel ) << ", " 
					<< (barrier.subresourceRange.levelCount == VK_REMAINING_MIP_LEVELS ? "whole" : ToString( barrier.subresourceRange.baseMipLevel + barrier.subresourceRange.levelCount )) << ")\n";
			}
			if ( not (barrier.subresourceRange.baseArrayLayer == 0 and barrier.subresourceRange.layerCount == VK_REMAINING_ARRAY_LAYERS) )
			{
				log << "      layers:  [" << ToString( barrier.subresourceRange.baseArrayLayer ) << ", "
					<< (barrier.subresourceRange.layerCount == VK_REMAINING_ARRAY_LAYERS ? "whole" : ToString( barrier.subresourceRange.baseArrayLayer + barrier.subresourceRange.layerCount )) << ")\n";
			}
		}

		log << "  ----------\n\n";
	}
	
/*
=================================================
	GetPreviousLayout
=================================================
*/
	static VkImageLayout GetPreviousLayout (const VulkanLogger::FramebufferData& framebuffer, const VulkanLogger::RenderPassData& renderPass, uint subpassIndex, uint attachmentIndex, VkImageLayout currentLasyout)
	{
		struct StackValue
		{
			uint			subpass;
			VkImageLayout	layout;
		};
		StackValue	stack [32];
		uint		stack_size	= 0;

		for (auto& dep : renderPass.dependencies)
		{
			if ( dep.dstSubpass == subpassIndex )
			{
				stack[stack_size].subpass = dep.srcSubpass;

				if ( dep.srcSubpass == VK_SUBPASS_EXTERNAL )
				{
					stack[stack_size].layout = renderPass.attachments[ attachmentIndex ].initialLayout;
					++stack_size;
				}
				else
				{
					auto&	sp		= renderPass.subpasses[ dep.srcSubpass ];
					bool	found	= false;

					for (uint i = 0; !found && i < sp.colorAttachmentCount; ++i)
					{
						if ( sp.pColorAttachments[i].attachment == attachmentIndex )
						{
							stack[stack_size].layout = sp.pColorAttachments[i].layout;
							found = true;
						}
					}
					for (uint i = 0; sp.pResolveAttachments && !found && i < sp.colorAttachmentCount; ++i)
					{
						if ( sp.pResolveAttachments[i].attachment == attachmentIndex )
						{
							stack[stack_size].layout = sp.pResolveAttachments[i].layout;
							found = true;
						}
					}
					if ( sp.pDepthStencilAttachment && !found && sp.pDepthStencilAttachment->attachment == attachmentIndex )
					{
						stack[stack_size].layout = sp.pDepthStencilAttachment->layout;
						found = true;
					}
					for (uint i = 0; !found && sp.inputAttachmentCount; ++i)
					{
						if ( sp.pInputAttachments[i].attachment == attachmentIndex )
						{
							stack[stack_size].layout = sp.pInputAttachments[i].layout;
							found = true;
						}
					}

					if ( found )
						++stack_size;
				}
			}
		}

		if ( stack_size == 0 )
			return currentLasyout;

		if ( stack_size == 1 )
			return stack[0].layout;

		ASSERT( !"TODO" );
		return currentLasyout;
	}
	
/*
=================================================
	Wrap_vkCmdBeginRenderPass
=================================================
*/
	void VKAPI_CALL Wrap_vkCmdBeginRenderPass (VkCommandBuffer commandBuffer, const VkRenderPassBeginInfo* pRenderPassBegin, VkSubpassContents contents)
	{
		auto&	logger = VulkanLogger::Get();
		EXLOCK( logger.guard );

		auto	iter = logger.commandBuffers.find( commandBuffer );
		if ( iter == logger.commandBuffers.end() )
			return;

		auto&	cmdbuf = iter->second;
		ASSERT( cmdbuf.cmdBuffer == commandBuffer );

		logger.vkCmdBeginRenderPass( commandBuffer, pRenderPassBegin, contents );

		cmdbuf.currentRP	= pRenderPassBegin->renderPass;
		cmdbuf.currentFB	= pRenderPassBegin->framebuffer;
		cmdbuf.subpassIndex	= 0;

		if ( not logger.enableLog )
			return;

		auto&	log = cmdbuf.log;
		log << "  BeginRenderPass\n";

		auto	rp_iter = logger.renderPassMap.find( cmdbuf.currentRP );
		if ( rp_iter == logger.renderPassMap.end() )
			return;

		auto	fb_iter = logger.framebufferMap.find( cmdbuf.currentFB );
		if ( fb_iter == logger.framebufferMap.end() )
			return;

		auto&	rp = rp_iter->second;
		auto&	fb = fb_iter->second;

		log << "    renderPass:  " << rp.name << '\n';
		log << "    framebuffer: " << fb.name << '\n';

		auto&	pass = rp.info.pSubpasses[cmdbuf.subpassIndex];

		for (uint i = 0; i < pass.colorAttachmentCount; ++i)
		{
			auto&	ref	= pass.pColorAttachments[i];
			auto&	at	= rp.info.pAttachments[ref.attachment];

			auto	view_iter = logger.imageViewMap.find( fb.attachments[ref.attachment] );
			if ( view_iter == logger.imageViewMap.end() )
				return;

			log << "    color attachment:\n";
			log << "      name:   " << view_iter->second.name << "\n";
			log << "      layout: " << VkImageLayoutToString( at.initialLayout );

			if ( at.initialLayout != ref.layout )
				log << " ---> " << VkImageLayoutToString( ref.layout );

			log << '\n';
			log << "      loadOp: " << VkAttachmentLoadOpToString( at.loadOp ) << "\n";
		}

		if ( pass.pDepthStencilAttachment != null )
		{
			auto&	ref	= *pass.pDepthStencilAttachment;
			auto&	at	= rp.info.pAttachments[ref.attachment];

			auto	view_iter = logger.imageViewMap.find( fb.attachments[ref.attachment] );
			if ( view_iter == logger.imageViewMap.end() )
				return;

			log << "    depth-stencil attachment:\n";
			log << "      name:          " << view_iter->second.name << '\n';
			log << "      layout:        " << VkImageLayoutToString( at.initialLayout );

			if ( at.initialLayout != ref.layout )
				log << " ---> " << VkImageLayoutToString( ref.layout );

			log << '\n';
			log << "      depthLoadOp:   " << VkAttachmentLoadOpToString( at.loadOp ) << '\n';
			log << "      stencilLoadOp: " << VkAttachmentLoadOpToString( at.stencilLoadOp ) << '\n';
		}

		for (auto& dep : rp.dependencies)
		{
			if ( dep.dstSubpass == cmdbuf.subpassIndex )
			{
				log << "    dependency:\n";
				log << "      source: " << (dep.srcSubpass == VK_SUBPASS_EXTERNAL ? "External" : ToString( dep.srcSubpass) ) << '\n';
				log << "      stage:  " << VkPipelineStageToString( dep.srcStageMask ) << " ---> " << VkPipelineStageToString( dep.dstStageMask ) << '\n';
				log << "      access: " << VkAccessMaskToString( dep.srcAccessMask ) << " ---> " << VkAccessMaskToString( dep.dstAccessMask ) << '\n';
			}
		}

		log << "  ----------\n\n";
	}
	
/*
=================================================
	Wrap_vkCmdNextSubpass
=================================================
*/
	void VKAPI_CALL Wrap_vkCmdNextSubpass (VkCommandBuffer commandBuffer, VkSubpassContents contents)
	{
		auto&	logger = VulkanLogger::Get();
		EXLOCK( logger.guard );

		auto	iter = logger.commandBuffers.find( commandBuffer );
		if ( iter == logger.commandBuffers.end() )
			return;

		auto&	cmdbuf = iter->second;
		ASSERT( cmdbuf.cmdBuffer == commandBuffer );

		logger.vkCmdNextSubpass( commandBuffer, contents );
		cmdbuf.subpassIndex++;

		if ( not logger.enableLog )
			return;

		auto&	log = cmdbuf.log;
		log << "  NextSubpass\n";
		log << "    index: " << ToString( cmdbuf.subpassIndex ) << '\n';

		auto	rp_iter = logger.renderPassMap.find( cmdbuf.currentRP );
		if ( rp_iter == logger.renderPassMap.end() )
			return;

		auto	fb_iter = logger.framebufferMap.find( cmdbuf.currentFB );
		if ( fb_iter == logger.framebufferMap.end() )
			return;

		auto&	rp	 = rp_iter->second;
		auto&	fb	 = fb_iter->second;
		auto&	pass = rp.info.pSubpasses[cmdbuf.subpassIndex];

		for (uint i = 0; i < pass.colorAttachmentCount; ++i)
		{
			auto&	ref	= pass.pColorAttachments[i];
			auto&	at	= rp.info.pAttachments[ref.attachment];

			auto	view_iter = logger.imageViewMap.find( fb.attachments[ref.attachment] );
			if ( view_iter == logger.imageViewMap.end() )
				return;

			const VkImageLayout	prev = GetPreviousLayout( fb, rp, cmdbuf.subpassIndex, ref.attachment, ref.layout );

			log << "    color attachment:\n";
			log << "      name:    " << view_iter->second.name << '\n';
			log << "      samples: " << VkSampleCountToString( at.samples ) << '\n';
			log << "      layout:  " << VkImageLayoutToString( prev );

			if ( prev != ref.layout )
				log << " ---> " << VkImageLayoutToString( ref.layout );
			
			log << "\n";
			log << "      loadOp:  " << VkAttachmentLoadOpToString( at.loadOp ) << '\n';
		}

		if ( pass.pResolveAttachments )
		{
			for (uint i = 0; i < pass.colorAttachmentCount; ++i)
			{
				auto&	ref	= pass.pResolveAttachments[i];
				auto&	at	= rp.info.pAttachments[ref.attachment];

				auto	view_iter = logger.imageViewMap.find( fb.attachments[ref.attachment] );
				if ( view_iter == logger.imageViewMap.end() )
					return;

				const VkImageLayout	prev = GetPreviousLayout( fb, rp, cmdbuf.subpassIndex, ref.attachment, ref.layout );

				log << "    resolve attachment:\n";
				log << "      name:    " << view_iter->second.name << '\n';
				log << "      samples: " << VkSampleCountToString( at.samples ) << '\n';
				log << "      layout:  " << VkImageLayoutToString( prev );

				if ( prev != ref.layout )
					log << " ---> " << VkImageLayoutToString( ref.layout );
				
				log << '\n';
			}
		}

		for (uint i = 0; i < pass.inputAttachmentCount; ++i)
		{
			auto&	ref	= pass.pInputAttachments[i];
			auto&	at	= rp.info.pAttachments[ref.attachment];

			auto	view_iter = logger.imageViewMap.find( fb.attachments[ref.attachment] );
			if ( view_iter == logger.imageViewMap.end() )
				return;

			const VkImageLayout	prev = GetPreviousLayout( fb, rp, cmdbuf.subpassIndex, ref.attachment, ref.layout );

			log << "    input attachment:\n";
			log << "      name:    " << view_iter->second.name + '\n';
			log << "      samples: " << VkSampleCountToString( at.samples ) << '\n';
			log << "      layout:  " << VkImageLayoutToString( prev );

			if ( prev != ref.layout )
				log << " ---> " << VkImageLayoutToString( ref.layout );
			
			log << '\n';
		}

		if ( pass.pDepthStencilAttachment != null )
		{
			auto&	ref	= *pass.pDepthStencilAttachment;
			auto&	at	= rp.info.pAttachments[ref.attachment];

			auto	view_iter = logger.imageViewMap.find( fb.attachments[ref.attachment] );
			if ( view_iter == logger.imageViewMap.end() )
				return;

			const VkImageLayout prev = GetPreviousLayout( fb, rp, cmdbuf.subpassIndex, ref.attachment, ref.layout );

			log << "    depth-stencil attachment:\n";
			log << "      name:    " << view_iter->second.name << '\n';
			log << "      samples: " << VkSampleCountToString( at.samples ) << '\n';
			log << "      layout:  " << VkImageLayoutToString( prev );

			if ( prev != ref.layout )
				log << " ---> " << VkImageLayoutToString( ref.layout );
			
			log << '\n';
		}

		for (auto& dep : rp.dependencies)
		{
			if ( dep.dstSubpass == cmdbuf.subpassIndex )
			{
				log << "    dependency:\n";
				log << "      source: " << (dep.srcSubpass == VK_SUBPASS_EXTERNAL ? "External" : ToString(dep.srcSubpass)) << '\n';
				log << "      stage:  " << VkPipelineStageToString( dep.srcStageMask ) << " ---> " << VkPipelineStageToString( dep.dstStageMask ) << "\n";
				log << "      access: " << VkAccessMaskToString( dep.srcAccessMask ) << " ---> " << VkAccessMaskToString( dep.dstAccessMask ) << "\n";
			}
		}

		// TODO: preserve attachments

		log << "  ----------\n\n";
	}
	
/*
=================================================
	Wrap_vkCmdEndRenderPass
=================================================
*/
	ND_ static bool  IsDepthStencilFormat (VkFormat fmt)
	{
		switch ( fmt )
		{
			case VK_FORMAT_D16_UNORM :
			case VK_FORMAT_X8_D24_UNORM_PACK32 :
			case VK_FORMAT_D32_SFLOAT :
			case VK_FORMAT_S8_UINT :
			case VK_FORMAT_D16_UNORM_S8_UINT :
			case VK_FORMAT_D24_UNORM_S8_UINT :
			case VK_FORMAT_D32_SFLOAT_S8_UINT :
				return true;
		}
		return false;
	}
	
/*
=================================================
	Wrap_vkCmdEndRenderPass
=================================================
*/
	void VKAPI_CALL Wrap_vkCmdEndRenderPass (VkCommandBuffer commandBuffer)
	{
		auto&	logger = VulkanLogger::Get();
		EXLOCK( logger.guard );

		auto	iter = logger.commandBuffers.find( commandBuffer );
		if ( iter == logger.commandBuffers.end() )
			return;

		auto&	cmdbuf = iter->second;
		ASSERT( cmdbuf.cmdBuffer == commandBuffer );

		logger.vkCmdEndRenderPass( commandBuffer );

		const auto	old_rp = cmdbuf.currentRP;
		const auto	old_fb = cmdbuf.currentFB;

		cmdbuf.currentRP	= VK_NULL_HANDLE;
		cmdbuf.currentFB	= VK_NULL_HANDLE;
		cmdbuf.subpassIndex	= UMax;

		if ( not logger.enableLog )
			return;

		auto&	log = cmdbuf.log;
		log << "  EndRenderPass\n";

		auto	rp_iter = logger.renderPassMap.find( old_rp );
		if ( rp_iter == logger.renderPassMap.end() )
			return;

		auto	fb_iter = logger.framebufferMap.find( old_fb );
		if ( fb_iter == logger.framebufferMap.end() )
			return;

		auto&	rp = rp_iter->second;
		auto&	fb = fb_iter->second;

		for (usize i = 0; i < rp.info.attachmentCount; ++i)
		{
			auto&	at = rp.info.pAttachments[i];

			auto	view_iter = logger.imageViewMap.find( fb.attachments[i] );
			if ( view_iter == logger.imageViewMap.end() )
				return;

			const VkImageLayout prev = GetPreviousLayout( fb, rp, VK_SUBPASS_EXTERNAL, uint(i), at.finalLayout );

			log << "    attachment:\n";
			log << "      name:    " << view_iter->second.name << '\n';
			log << "      samples: " << VkSampleCountToString( at.samples ) << '\n';
			log << "      layout:  " << VkImageLayoutToString( prev );

			if ( prev != at.finalLayout )
				log << " ---> " << VkImageLayoutToString( at.finalLayout );
			
			log << "\n";

			if ( IsDepthStencilFormat( at.format ))
			{
				log << "      depthStoreOp:   " << VkAttachmentStoreOpToString( at.storeOp ) << '\n';
				log << "      stencilStoreOp: " << VkAttachmentStoreOpToString( at.stencilStoreOp ) << '\n';
			}
			else
			{
				log << "      storeOp: " << VkAttachmentStoreOpToString( at.storeOp ) << '\n';
			}
		}

		for (auto& dep : rp.dependencies)
		{
			if ( dep.dstSubpass == VK_SUBPASS_EXTERNAL )
			{
				log << "    dependency:\n";
				log << "      source: " << (dep.srcSubpass == VK_SUBPASS_EXTERNAL ? "External" : ToString( dep.srcSubpass )) << '\n';
				log << "      stage:  " << VkPipelineStageToString( dep.srcStageMask ) << " ---> " << VkPipelineStageToString( dep.dstStageMask ) << '\n';
				log << "      access: " << VkAccessMaskToString( dep.srcAccessMask ) << " ---> " << VkAccessMaskToString( dep.dstAccessMask ) << '\n';
			}
		}

		log << "  ----------\n\n";
	}
	
/*
=================================================
	Wrap_vkCmdCopyBuffer
=================================================
*/
	void VKAPI_CALL Wrap_vkCmdCopyBuffer (VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer, uint regionCount, const VkBufferCopy* pRegions)
	{
		auto&	logger = VulkanLogger::Get();
		EXLOCK( logger.guard );

		auto	iter = logger.commandBuffers.find( commandBuffer );
		if ( iter == logger.commandBuffers.end() )
			return;

		auto&	cmdbuf = iter->second;
		ASSERT( cmdbuf.cmdBuffer == commandBuffer );

		logger.vkCmdCopyBuffer( commandBuffer, srcBuffer, dstBuffer, regionCount, pRegions );

		if ( not logger.enableLog )
			return;

		auto&	log = cmdbuf.log;
		log << "  CopyBuffer\n";

		auto	src_iter = logger.bufferMap.find( srcBuffer );
		if ( src_iter == logger.bufferMap.end() )
			return;
	
		auto	dst_iter = logger.bufferMap.find( dstBuffer );
		if ( dst_iter == logger.bufferMap.end() )
			return;

		log << "    src: " << src_iter->second.name << '\n';
		log << "    dst: " << dst_iter->second.name << '\n';

		for (uint i = 0; i < regionCount; ++i)
		{
			auto&	reg = pRegions[i];
			log << "      copy [" << ToString( reg.srcOffset ) << ", " << ToString( reg.srcOffset + reg.size )
				<< ") ---> ["     << ToString( reg.dstOffset ) << ", " << ToString( reg.dstOffset + reg.size ) << ")\n";
		}
		log << "  ----------\n\n";
	}
	
/*
=================================================
	SubresourceLayerToString
=================================================
*/
	ND_ static String  SubresourceLayerToString (const VkOffset3D &offset, const VkExtent3D &extent, const VkImageSubresourceLayers &subres)
	{
		return	"{ [("s << ToString( offset.x ) << ", " << ToString( offset.y ) << ", " << ToString( offset.z )
				<< "), (" << ToString( offset.x + extent.width ) << ", " << ToString( offset.y + extent.height ) << ", " << ToString( offset.z + extent.depth )
				<< ")), mip:" << ToString( subres.mipLevel )
				<< ", layers: [" << ToString( subres.baseArrayLayer )
				<< ", " << ToString( subres.baseArrayLayer + subres.layerCount ) << "), "
				<< VkImageAspectToString( subres.aspectMask ) << " }";
	}

/*
=================================================
	Wrap_vkCmdCopyImage
=================================================
*/
	void VKAPI_CALL Wrap_vkCmdCopyImage (VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint regionCount, const VkImageCopy* pRegions)
	{
		auto&	logger = VulkanLogger::Get();
		EXLOCK( logger.guard );

		auto	iter = logger.commandBuffers.find( commandBuffer );
		if ( iter == logger.commandBuffers.end() )
			return;

		auto&	cmdbuf = iter->second;
		ASSERT( cmdbuf.cmdBuffer == commandBuffer );

		logger.vkCmdCopyImage( commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions );

		if ( not logger.enableLog )
			return;

		auto&	log = cmdbuf.log;
		log << "  CopyImage\n";

		auto	src_iter = logger.imageMap.find( srcImage );
		if ( src_iter == logger.imageMap.end() )
			return;
	
		auto	dst_iter = logger.imageMap.find( dstImage );
		if ( dst_iter == logger.imageMap.end() )
			return;

		log << "    src:       " << src_iter->second.name << '\n';
		log << "    srcLayout: " << VkImageLayoutToString( srcImageLayout ) << '\n';
		log << "    dst:       " << dst_iter->second.name << '\n';
		log << "    dstLayout: " << VkImageLayoutToString( dstImageLayout ) << '\n';

		for (uint i = 0; i < regionCount; ++i)
		{
			auto&	reg = pRegions[i];
			log << "      copy " << SubresourceLayerToString( reg.srcOffset, reg.extent, reg.srcSubresource ) << " ---> " << SubresourceLayerToString( reg.dstOffset, reg.extent, reg.dstSubresource ) << '\n';
		}
		log << "  ----------\n\n";
	}
	
/*
=================================================
	Wrap_vkCmdBlitImage
=================================================
*/
	void VKAPI_CALL Wrap_vkCmdBlitImage (VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint regionCount, const VkImageBlit* pRegions, VkFilter filter)
	{
		auto&	logger = VulkanLogger::Get();
		EXLOCK( logger.guard );

		auto	iter = logger.commandBuffers.find( commandBuffer );
		if ( iter == logger.commandBuffers.end() )
			return;

		auto&	cmdbuf = iter->second;
		ASSERT( cmdbuf.cmdBuffer == commandBuffer );
	
		logger.vkCmdBlitImage( commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions, filter );
		
		if ( not logger.enableLog )
			return;

		auto&	log = cmdbuf.log;
		log << "  BlitImage\n";

		auto	src_iter = logger.imageMap.find( srcImage );
		if ( src_iter == logger.imageMap.end() )
			return;
	
		auto	dst_iter = logger.imageMap.find( dstImage );
		if ( dst_iter == logger.imageMap.end( ))
			return;

		log << "    src:       " << src_iter->second.name << '\n';
		log << "    srcLayout: " << VkImageLayoutToString( srcImageLayout ) << '\n';
		log << "    dst:       " << dst_iter->second.name << '\n';
		log << "    dstLayout: " << VkImageLayoutToString( dstImageLayout ) << '\n';
		log << "  ----------\n\n";
	}
	
/*
=================================================
	Wrap_vkCmdCopyBufferToImage
=================================================
*/
	void VKAPI_CALL Wrap_vkCmdCopyBufferToImage (VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkImage dstImage, VkImageLayout dstImageLayout, uint regionCount, const VkBufferImageCopy* pRegions)
	{
		auto&	logger = VulkanLogger::Get();
		EXLOCK( logger.guard );

		auto	iter = logger.commandBuffers.find( commandBuffer );
		if ( iter == logger.commandBuffers.end() )
			return;

		auto&	cmdbuf = iter->second;
		ASSERT( cmdbuf.cmdBuffer == commandBuffer );

		logger.vkCmdCopyBufferToImage(commandBuffer, srcBuffer, dstImage, dstImageLayout, regionCount, pRegions);
		
		if ( not logger.enableLog )
			return;

		auto&	log = cmdbuf.log;
		log << "  CopyBufferToImage\n";

		auto	src_iter = logger.bufferMap.find( srcBuffer );
		if ( src_iter == logger.bufferMap.end() )
			return;
	
		auto	dst_iter = logger.imageMap.find( dstImage );
		if ( dst_iter == logger.imageMap.end() )
			return;

		log << "    src:       " << src_iter->second.name << '\n';
		log << "    dst:       " << dst_iter->second.name << '\n';
		log << "    dstLayout: " << VkImageLayoutToString( dstImageLayout ) << '\n';

		log << "  ----------\n\n";
	}
	
/*
=================================================
	Wrap_vkCmdCopyImageToBuffer
=================================================
*/
	void VKAPI_CALL Wrap_vkCmdCopyImageToBuffer (VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkBuffer dstBuffer, uint regionCount, const VkBufferImageCopy* pRegions)
	{
		auto&	logger = VulkanLogger::Get();
		EXLOCK( logger.guard );

		auto	iter = logger.commandBuffers.find( commandBuffer );
		if ( iter == logger.commandBuffers.end() )
			return;

		auto&	cmdbuf = iter->second;
		ASSERT( cmdbuf.cmdBuffer == commandBuffer );

		logger.vkCmdCopyImageToBuffer( commandBuffer, srcImage, srcImageLayout, dstBuffer, regionCount, pRegions );
		
		if ( not logger.enableLog )
			return;

		auto&	log = cmdbuf.log;
		log << "  CopyImageToBuffer\n";

		auto	src_iter = logger.imageMap.find( srcImage );
		if ( src_iter == logger.imageMap.end() )
			return;
	
		auto	dst_iter = logger.bufferMap.find( dstBuffer );
		if ( dst_iter == logger.bufferMap.end() )
			return;

		log << "    src:       " << src_iter->second.name << '\n';
		log << "    srcLayout: " << VkImageLayoutToString( srcImageLayout ) << '\n';
		log << "    dst:       " << dst_iter->second.name << '\n';
		log << "  ----------\n\n";
	}
	
/*
=================================================
	Wrap_vkCmdUpdateBuffer
=================================================
*/
	void VKAPI_CALL Wrap_vkCmdUpdateBuffer (VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize dataSize, const void* pData)
	{
		auto&	logger = VulkanLogger::Get();
		EXLOCK( logger.guard );

		auto	iter = logger.commandBuffers.find( commandBuffer );
		if ( iter == logger.commandBuffers.end() )
			return;

		auto&	cmdbuf = iter->second;
		ASSERT( cmdbuf.cmdBuffer == commandBuffer );

		logger.vkCmdUpdateBuffer( commandBuffer, dstBuffer, dstOffset, dataSize, pData );
	
		if ( not logger.enableLog )
			return;

		auto&	log = cmdbuf.log;
		log << "  UpdateBuffer\n";

		auto	dst_iter = logger.bufferMap.find( dstBuffer );
		if ( dst_iter == logger.bufferMap.end() )
			return;
	
		log << "    dst:       " << dst_iter->second.name << "\n";
		log << "  ----------\n\n";
	}
	
/*
=================================================
	Wrap_vkCmdFillBuffer
=================================================
*/
	void VKAPI_CALL Wrap_vkCmdFillBuffer (VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize size, uint data)
	{
		auto&	logger = VulkanLogger::Get();
		EXLOCK( logger.guard );

		auto	iter = logger.commandBuffers.find( commandBuffer );
		if ( iter == logger.commandBuffers.end() )
			return;

		auto&	cmdbuf = iter->second;
		ASSERT( cmdbuf.cmdBuffer == commandBuffer );

		logger.vkCmdFillBuffer( commandBuffer, dstBuffer, dstOffset, size, data );
	
		if ( not logger.enableLog )
			return;

		auto&	log = cmdbuf.log;
		log << "  FillBuffer\n";

		auto	dst_iter = logger.bufferMap.find( dstBuffer );
		if ( dst_iter == logger.bufferMap.end() )
			return;
	
		log << "    dst:       " << dst_iter->second.name << '\n';
		log << "  ----------\n\n";
	}
	
/*
=================================================
	Wrap_vkCmdClearColorImage
=================================================
*/
	void VKAPI_CALL Wrap_vkCmdClearColorImage (VkCommandBuffer commandBuffer, VkImage image, VkImageLayout imageLayout, const VkClearColorValue* pColor, uint rangeCount, const VkImageSubresourceRange* pRanges)
	{
		auto&	logger = VulkanLogger::Get();
		EXLOCK( logger.guard );

		auto	iter = logger.commandBuffers.find( commandBuffer );
		if ( iter == logger.commandBuffers.end() )
			return;

		auto&	cmdbuf = iter->second;
		ASSERT( cmdbuf.cmdBuffer == commandBuffer );

		logger.vkCmdClearColorImage( commandBuffer, image, imageLayout, pColor, rangeCount, pRanges );
	
		if ( not logger.enableLog )
			return;

		auto& log = cmdbuf.log;
		log << "  ClearColorImage\n";

		// TODO
	}
	
/*
=================================================
	Wrap_vkCmdClearDepthStencilImage
=================================================
*/
	void VKAPI_CALL Wrap_vkCmdClearDepthStencilImage (VkCommandBuffer commandBuffer, VkImage image, VkImageLayout imageLayout, const VkClearDepthStencilValue* pDepthStencil, uint rangeCount, const VkImageSubresourceRange* pRanges)
	{
		auto&	logger = VulkanLogger::Get();
		EXLOCK( logger.guard );

		// TODO
	}
	
/*
=================================================
	Wrap_vkCmdClearAttachments
=================================================
*/
	void VKAPI_CALL Wrap_vkCmdClearAttachments (VkCommandBuffer commandBuffer, uint attachmentCount, const VkClearAttachment* pAttachments, uint rectCount, const VkClearRect* pRects)
	{
		auto&	logger = VulkanLogger::Get();
		EXLOCK( logger.guard );

		// TODO
	}
	
/*
=================================================
	Wrap_vkCmdResolveImage
=================================================
*/
	void VKAPI_CALL Wrap_vkCmdResolveImage (VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint regionCount, const VkImageResolve* pRegions)
	{
		auto&	logger = VulkanLogger::Get();
		EXLOCK( logger.guard );

		// TODO
	}
	
/*
=================================================
	Initialize
=================================================
*/
	void  VulkanLogger::Initialize (INOUT VulkanDeviceFnTable& fnTable)
	{
		EXLOCK( guard );

		std::memcpy( &_originDeviceFnTable, &fnTable, sizeof(_originDeviceFnTable) );
		VulkanDeviceFn_Init( reinterpret_cast<VulkanDeviceFnTable*>(&_originDeviceFnTable) );

		auto&	table = *reinterpret_cast<DeviceFnTable *>(&fnTable);

		table._var_vkCreateBuffer				= &Wrap_vkCreateBuffer;
		table._var_vkDestroyBuffer				= &Wrap_vkDestroyBuffer;
		table._var_vkCreateBufferView			= &Wrap_vkCreateBufferView;
		table._var_vkDestroyBufferView			= &Wrap_vkDestroyBufferView;
		table._var_vkCreateImage				= &Wrap_vkCreateImage;
		table._var_vkDestroyImage				= &Wrap_vkDestroyImage;
		table._var_vkCreateImageView			= &Wrap_vkCreateImageView;
		table._var_vkDestroyImageView			= &Wrap_vkDestroyImageView;
		table._var_vkCreateFramebuffer			= &Wrap_vkCreateFramebuffer;
		table._var_vkDestroyFramebuffer			= &Wrap_vkDestroyFramebuffer;
		table._var_vkCreateRenderPass			= &Wrap_vkCreateRenderPass;
		table._var_vkDestroyRenderPass			= &Wrap_vkDestroyRenderPass;
		table._var_vkCreateGraphicsPipelines	= &Wrap_vkCreateGraphicsPipelines;
		table._var_vkCreateComputePipelines		= &Wrap_vkCreateComputePipelines;
		table._var_vkDestroyPipeline			= &Wrap_vkDestroyPipeline;
		table._var_vkAllocateCommandBuffers		= &Wrap_vkAllocateCommandBuffers;
		table._var_vkBeginCommandBuffer			= &Wrap_vkBeginCommandBuffer;
		table._var_vkEndCommandBuffer			= &Wrap_vkEndCommandBuffer;
		table._var_vkQueueSubmit				= &Wrap_vkQueueSubmit;
		table._var_vkSetDebugUtilsObjectNameEXT	= &Wrap_vkSetDebugUtilsObjectNameEXT;
		table._var_vkCmdPipelineBarrier			= &Wrap_vkCmdPipelineBarrier;
		table._var_vkCmdBeginRenderPass			= &Wrap_vkCmdBeginRenderPass;
		table._var_vkCmdNextSubpass				= &Wrap_vkCmdNextSubpass;
		table._var_vkCmdEndRenderPass			= &Wrap_vkCmdEndRenderPass;
		table._var_vkCmdCopyBuffer				= &Wrap_vkCmdCopyBuffer;
		table._var_vkCmdCopyImage				= &Wrap_vkCmdCopyImage;
		table._var_vkCmdBlitImage				= &Wrap_vkCmdBlitImage;
		table._var_vkCmdCopyBufferToImage		= &Wrap_vkCmdCopyBufferToImage;
		table._var_vkCmdCopyImageToBuffer		= &Wrap_vkCmdCopyImageToBuffer;
		table._var_vkCmdUpdateBuffer			= &Wrap_vkCmdUpdateBuffer;
		table._var_vkCmdFillBuffer				= &Wrap_vkCmdFillBuffer;
		table._var_vkCmdClearColorImage			= &Wrap_vkCmdClearColorImage;
		table._var_vkCmdClearDepthStencilImage	= &Wrap_vkCmdClearDepthStencilImage;
		table._var_vkCmdClearAttachments		= &Wrap_vkCmdClearAttachments;
		table._var_vkCmdResolveImage			= &Wrap_vkCmdResolveImage;
	}
	
/*
=================================================
	Deinitialize
=================================================
*/
	void  VulkanLogger::Deinitialize (OUT VulkanDeviceFnTable& fnTable)
	{
		EXLOCK( guard );

		std::memcpy( &fnTable, &_originDeviceFnTable, sizeof(_originDeviceFnTable) );
	}

}	// namespace


/*
=================================================
	_AddVulkanHooks
=================================================
*/
void  VRGTest::_AddVulkanHooks ()
{
	auto&	logger = VulkanLogger::Get();
	PlacementNew<VulkanLogger>( &logger );

	logger.Initialize( _vulkan.EditDeviceFnTable() );
}

/*
=================================================
	_RemoveVulkanHooks
=================================================
*/
void  VRGTest::_RemoveVulkanHooks ()
{
	auto&	logger = VulkanLogger::Get();

	logger.Deinitialize( _vulkan.EditDeviceFnTable() );
	logger.~VulkanLogger();
}

/*
=================================================
	_EnableLogging
=================================================
*/
void  VRGTest::_EnableLogging (bool enable)
{
	auto&	logger = VulkanLogger::Get();
	EXLOCK( logger.guard );

	logger.enableLog = enable;
}

/*
=================================================
	_GetLog
=================================================
*/
void  VRGTest::_GetLog (OUT String &log) const
{
	auto&	logger = VulkanLogger::Get();
	EXLOCK( logger.guard );

	log.clear();
	std::swap( log, logger.log );
}


#endif	// AE_ENABLE_VULKAN
