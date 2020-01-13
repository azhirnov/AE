// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef AE_ENABLE_VULKAN

# include "graphics/Public/RenderPassDesc.h"
# include "graphics/Public/EResourceState.h"
# include "graphics/Vulkan/VCommon.h"

namespace AE::Graphics
{

	//
	// Vulkan Logical Render Pass
	//

	class VLogicalRenderPass final
	{
	// types
	public:
		struct ColorTarget
		{
			uint					index			= Default;
			VImageID				imageId;
			ImageViewDesc			viewDesc;
			VkSampleCountFlagBits	samples			= VK_SAMPLE_COUNT_FLAG_BITS_MAX_ENUM;
			VkAttachmentLoadOp		loadOp			= VK_ATTACHMENT_LOAD_OP_MAX_ENUM;
			VkAttachmentStoreOp		storeOp			= VK_ATTACHMENT_STORE_OP_MAX_ENUM;
			EResourceState			state			= Default;
			VkImageLayout			layout			= VK_IMAGE_LAYOUT_UNDEFINED;
			//HashVal				_imageHash;		// used for fast render target comparison

			ColorTarget () {}
		};

		struct DepthStencilTarget : ColorTarget
		{
			DepthStencilTarget () {}
			DepthStencilTarget (const ColorTarget &ct) : ColorTarget{ct} {}

			ND_ bool IsDefined () const		{ return imageId.IsValid(); }
		};
		
		using VkClearValues_t	= StaticArray< VkClearValue, GraphicsConfig::MaxAttachments >;
		using ColorTargets_t	= FixedMap< RenderTargetName, ColorTarget, GraphicsConfig::MaxColorBuffers >;
		using Viewports_t		= FixedArray< VkViewport, GraphicsConfig::MaxViewports >;
		using Scissors_t		= FixedArray< VkRect2D, GraphicsConfig::MaxViewports >;
		

	// variables
	private:
		ColorTargets_t			_colorTargets;
		DepthStencilTarget		_depthStencilTarget;
		VkClearValues_t			_clearValues;

		Viewports_t				_viewports;
		Scissors_t				_defaultScissors;
		
		RectI					_area;

		VRenderPassOutputID		_rpOutputId;

		uint					_subpassIndex	= UMax;
		RenderPassID			_renderPassId;
		VFramebufferID			_framebufferId;


	// methods
	public:
		VLogicalRenderPass () {}
		VLogicalRenderPass (VLogicalRenderPass &&) = delete;
		~VLogicalRenderPass () {}

		bool Create (const VResourceManager &resMngr, const RenderPassDesc &);
		bool SetRenderPass (const VResourceManager &resMngr, RenderPassID rp, uint subpass, VFramebufferID fb);
		
		ND_ ColorTargets_t const&		GetColorTargets ()			const	{ return _colorTargets; }
		ND_ DepthStencilTarget const&	GetDepthStencilTarget ()	const	{ return _depthStencilTarget; }
		ND_ ArrayView< VkClearValue >	GetClearValues ()			const	{ return _clearValues; }
		
		ND_ RectI const&				GetArea ()					const	{ return _area; }
		ND_ VRenderPassOutputID			GetRenderPassOutput ()		const	{ return _rpOutputId; }
		
		ND_ ArrayView<VkViewport>		GetViewports ()				const	{ return _viewports; }
		ND_ ArrayView<VkRect2D>			GetScissors ()				const	{ return _defaultScissors; }

		ND_ uint						GetSubpassIndex ()			const	{ return _subpassIndex; }
		ND_ RenderPassID				GetRenderPass ()			const	{ return _renderPassId; }
		ND_ VFramebufferID				GetFramebuffer ()			const	{ return _framebufferId; }
	};


}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
