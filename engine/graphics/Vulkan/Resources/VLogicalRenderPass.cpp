// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#ifdef AE_ENABLE_VULKAN

# include "graphics/Vulkan/Resources/VLogicalRenderPass.h"
# include "graphics/Vulkan/Resources/VImage.h"
# include "graphics/Vulkan/Resources/VRenderPass.h"
# include "graphics/Vulkan/Resources/VFramebuffer.h"
# include "graphics/Vulkan/Resources/VRenderPassOutput.h"
# include "graphics/Vulkan/VResourceManager.h"
# include "graphics/Vulkan/VEnumCast.h"
# include "graphics/Private/EnumUtils.h"

namespace AE::Graphics
{

/*
=================================================
	Create
=================================================
*/
	bool VLogicalRenderPass::Create (const VResourceManager &resMngr, const RenderPassDesc &desc)
	{
		const auto	ConvertClearValue = [](const RenderPassDesc::ClearValue_t &cv, OUT VkClearValue &result)
		{
			Visit( cv,
				[&result] (const RGBA32f &src)		{ MemCopy( result.color.float32, src ); },
				[&result] (const RGBA32u &src)		{ MemCopy( result.color.uint32, src ); },
				[&result] (const RGBA32i &src)		{ MemCopy( result.color.int32, src ); },
				[&result] (const DepthStencil &src)	{ result.depthStencil = {src.depth, src.stencil}; },
				[&result] (const NullUnion &)		{ memset( &result, 0, sizeof(result) ); }
			);
		};

		_area		= desc.area;
		_rpOutputId = resMngr.GetRenderPassOutput( desc.passName );

		auto*	rp_output = resMngr.GetResource( _rpOutputId );
		CHECK_ERR( rp_output );

		// copy render targets
		for (auto&[name, src] : desc.renderTargets)
		{
			if ( not src.image )
				continue;

			CHECK_ERR( name.IsDefined() );

			// get image description
			ImageDesc	img_dec;
			if ( src.image.ResourceType() == GfxResourceID::EType::Image )
			{
				VImage const*	img = resMngr.GetResource( VImageID{ src.image.Index(), src.image.Generation() });
				CHECK_ERR( img );
				img_dec = img->Description();
			}
			else
			if ( src.image.ResourceType() == GfxResourceID::EType::VirtualImage )
			{
				VVirtualImage const*	img = resMngr.GetResource( VVirtualImageID{ src.image.Index(), src.image.Generation() });
				CHECK_ERR( img );
				img_dec = img->Description().ToPhysical( uint2(_area.Size()), EVirtualResourceUsage::Unknown );
			}
			else
				RETURN_ERR( "unsupported resource type" );

			// setup color target
			ColorTarget		dst;
			dst.imageId		= src.image;
			dst.loadOp		= VEnumCast( src.loadOp );
			dst.storeOp		= VEnumCast( src.storeOp );
			dst.state		= EResourceState::Unknown;

			if ( src.desc.has_value() ) {
				dst.viewDesc = *src.desc;
				dst.viewDesc.Validate( img_dec );
			}else
				dst.viewDesc = ImageViewDesc{img_dec};

			dst.samples = VEnumCast( img_dec.samples );

			// set attachment index
			{
				auto	iter = rp_output->Get().find( name );
				CHECK_ERR( iter != rp_output->Get().end() );

				dst.index = iter->second.index;

				// compare output format and image format
				using	EType	 = PixelFormatInfo::EType;
				auto&	fmt_info = EPixelFormat_GetInfo( dst.viewDesc.format );

				BEGIN_ENUM_CHECKS();
				switch ( iter->second.type )
				{
					case EFragOutput::Int4 :
						CHECK( AllBits( fmt_info.valueType, EType::Int ));
						break;
					case EFragOutput::UInt4 :
						CHECK( AllBits( fmt_info.valueType, EType::UInt ));
						break;
					case EFragOutput::Float4 :
						CHECK( AnyBits( fmt_info.valueType, EType::SNorm | EType::UNorm | EType::SFloat | EType::UFloat ));
						break;
					case EFragOutput::Unknown :
						break;
				}
				END_ENUM_CHECKS();
			}

			// TODO
			// add resource state flags
			/*if ( (desc.area.left == 0) & (desc.area.right  == int(img_dec.dimension.x)) &
				 (desc.area.top  == 0) & (desc.area.bottom == int(img_dec.dimension.y)) )
			{
				if ( src.loadOp  == EAttachmentLoadOp::Clear or src.loadOp  == EAttachmentLoadOp::Invalidate )
					dst.state |= EResourceState::InvalidateBefore;

				if ( src.storeOp == EAttachmentStoreOp::Invalidate )
					dst.state |= EResourceState::InvalidateAfter;
			}*/

			// add color or depth-stencil render target
			if ( EPixelFormat_HasDepthOrStencil( dst.viewDesc.format ))
			{
				ASSERT( name == RenderTarget_Depth or name == RenderTarget_DepthStencil );
				
				dst.state |= EResourceState::DepthStencilAttachmentReadWrite;	// TODO: add support for other layouts
				dst.state |= (EResourceState::EarlyFragmentTests | EResourceState::LateFragmentTests);
				dst.layout = EResourceState_ToImageLayout( dst.state, 0 );

				_depthStencilTarget = DepthStencilTarget{ dst };
			}
			else
			{
				dst.state |= EResourceState::ColorAttachmentReadWrite;		// TODO: remove 'Read' state if blending disabled and 'loadOp' is 'Clear'
				dst.layout = EResourceState_ToImageLayout( dst.state, 0 );

				_colorTargets.push_back( dst );
			}

			ConvertClearValue( src.clearValue, OUT _clearValues[dst.index] );
		}

		// create viewports and default scissors
		for (auto& src : desc.viewports)
		{
			ASSERT( src.rect.left >= float(desc.area.left) and src.rect.right  <= float(desc.area.right)  );
			ASSERT( src.rect.top  >= float(desc.area.top)  and src.rect.bottom <= float(desc.area.bottom) );

			VkViewport		dst;
			dst.x			= src.rect.left;
			dst.y			= src.rect.top;
			dst.width		= src.rect.Width();
			dst.height		= src.rect.Height();
			dst.minDepth	= src.minDepth;
			dst.maxDepth	= src.maxDepth;
			_viewports.push_back( dst );

			// scissor
			VkRect2D		rect;
			rect.offset.x		= RoundToInt( src.rect.left );
			rect.offset.y		= RoundToInt( src.rect.top );
			rect.extent.width	= RoundToInt( src.rect.Width() );
			rect.extent.height	= RoundToInt( src.rect.Height() );
			_defaultScissors.push_back( rect );
		}

		// create default viewport
		if ( desc.viewports.empty() )
		{
			VkViewport		dst;
			dst.x			= float(desc.area.left);
			dst.y			= float(desc.area.top);
			dst.width		= float(desc.area.Width());
			dst.height		= float(desc.area.Height());
			dst.minDepth	= 0.0f;
			dst.maxDepth	= 1.0f;
			_viewports.push_back( dst );

			VkRect2D		rect;
			rect.offset.x		= desc.area.left;
			rect.offset.y		= desc.area.top;
			rect.extent.width	= desc.area.Width();
			rect.extent.height	= desc.area.Height();
			_defaultScissors.push_back( rect );
		}

		return true;
	}

/*
=================================================
	SetRenderPass
=================================================
*/
	bool  VLogicalRenderPass::SetRenderPass (const VResourceManager &, RenderPassID rp, uint subpass, uint subpassCount)
	{
		_renderPassId	= rp;
		_subpassIndex	= subpass;
		_isLastSubpass	= (subpass+1 == subpassCount);

		// TODO
		return true;
	}
	
/*
=================================================
	SetFramebuffer
=================================================
*/
	bool  VLogicalRenderPass::SetFramebuffer (VFramebufferID fb)
	{
		_framebufferId	= fb;

		// TODO
		return true;
	}


}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
