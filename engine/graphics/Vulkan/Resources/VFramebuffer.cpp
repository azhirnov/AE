// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#ifdef AE_ENABLE_VULKAN

# include "graphics/Vulkan/Resources/VFramebuffer.h"
# include "graphics/Vulkan/VResourceManager.h"

namespace AE::Graphics
{
	
/*
=================================================
	destructor
=================================================
*/
	VFramebuffer::~VFramebuffer ()
	{
		CHECK( _framebuffer == VK_NULL_HANDLE );
	}
	
/*
=================================================
	IsAllResourcesAlive
=================================================
*/
	bool VFramebuffer::IsAllResourcesAlive (const VResourceManager &resMngr) const
	{
		SHAREDLOCK( _drCheck );

		for (auto& attach : _attachments)
		{
			if ( not resMngr.IsAlive( attach.first ))
				return false;
		}
		return true;
	}

/*
=================================================
	operator ==
=================================================
*/
	bool VFramebuffer::operator == (const VFramebuffer &rhs) const
	{
		SHAREDLOCK( _drCheck );
		SHAREDLOCK( rhs._drCheck );

		if ( _hash != rhs._hash )
			return false;

		return	All( _dimension	== rhs._dimension )	and
				_layers			== rhs._layers		and
				_attachments	== rhs._attachments	and
				_renderPassId	== rhs._renderPassId;
	}

/*
=================================================
	constructor
=================================================
*/
	VFramebuffer::VFramebuffer (ArrayView<Pair<VImageID, ImageViewDesc>> attachments, UniqueID<RenderPassID> rp, uint2 dim, uint layers) :
		_renderPassId{ std::move(rp) },
		_dimension{ dim },
		_layers{ ImageLayer{layers} },
		_attachments{ attachments }
	{
		EXLOCK( _drCheck );
		ASSERT( not _attachments.empty() );
		ASSERT( _renderPassId );

		// calc hash
		_hash =  HashOf( _attachments );
		_hash << HashOf( *_renderPassId );
		_hash << HashOf( _dimension );
		_hash << HashOf( _layers );
	}

/*
=================================================
	Create
=================================================
*/
	bool VFramebuffer::Create (VResourceManager &resMngr, StringView dbgName)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( not _framebuffer );

		FixedArray< VkImageView, GraphicsConfig::MaxAttachments >	image_views;
		VDevice const &												dev			= resMngr.GetDevice();
		VRenderPass const *											ren_pass	= resMngr.GetResource( _renderPassId );
		CHECK_ERR( ren_pass );

		for (auto& rt : _attachments)
		{
			auto*		image = resMngr.GetResource( rt.first );
			CHECK_ERR( image );

			VkImageView	view = image->GetView( dev, false, INOUT rt.second );
			CHECK_ERR( view );

			image_views.push_back( view );
		}

		// create framebuffer
		VkFramebufferCreateInfo		fb_info	= {};
		
		fb_info.sType			= VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		fb_info.renderPass		= ren_pass->Handle();
		fb_info.attachmentCount	= uint(image_views.size());
		fb_info.pAttachments	= image_views.data();
		fb_info.width			= _dimension.x;
		fb_info.height			= _dimension.y;
		fb_info.layers			= _layers.Get();
		
		VK_CHECK( dev.vkCreateFramebuffer( dev.GetVkDevice(), &fb_info, null, OUT &_framebuffer ));
		
		_debugName = dbgName;
		return true;
	}
	
/*
=================================================
	Destroy
=================================================
*/
	void VFramebuffer::Destroy (VResourceManager &resMngr)
	{
		EXLOCK( _drCheck );

		if ( _framebuffer ) {
			auto&	dev = resMngr.GetDevice();
			dev.vkDestroyFramebuffer( dev.GetVkDevice(), _framebuffer, null );
		}

		if ( _renderPassId ) {
			resMngr.ReleaseResource( INOUT _renderPassId );
		}

		_framebuffer	= VK_NULL_HANDLE;
		_hash			= Default;
		_renderPassId	= Default;
		_dimension		= Default;
		_layers			= Default;

		_attachments.clear();
		_debugName.clear();
	}


}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
