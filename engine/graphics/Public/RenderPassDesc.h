// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "graphics/Public/IDs.h"
#include "graphics/Public/ResourceEnums.h"
#include "graphics/Public/ImageDesc.h"

namespace AE::Graphics
{

	//
	// Render Pass description
	//
	
	struct RenderPassDesc
	{
	// types
		using ClearValue_t = Union< NullUnion, RGBA32f, RGBA32u, RGBA32i, DepthStencil >;
		
		struct RT
		{
			GfxResourceID				image;		// may be image module in initial state (created by CreateRenderTarget or other)
			Optional< ImageViewDesc >	desc;		// may be used to specialize level, layer, different format, layout, ...
			ClearValue_t				clearValue;	// default is black color
			EAttachmentLoadOp			loadOp		= EAttachmentLoadOp::Load;
			EAttachmentStoreOp			storeOp		= EAttachmentStoreOp::Store;
		};

		struct Viewport
		{
			RectF		rect;
			float		minDepth	= 0.0f;
			float		maxDepth	= 1.0f;
		};

		using Targets_t		= FixedMap< RenderTargetName, RT, GraphicsConfig::MaxAttachments >;
		using Viewports_t	= FixedArray< Viewport, GraphicsConfig::MaxViewports >;


	// variables
		Targets_t		renderTargets;
		Viewports_t		viewports;
		RectI			area;
		RenderPassName	passName;


	// methods
		RenderPassDesc () {}

		explicit RenderPassDesc (const RenderPassName &name, const RectI &rect) : area{rect}, passName{name}
		{
			ASSERT( area.IsValid() );
			ASSERT( passName.IsDefined() );
		}

		explicit RenderPassDesc (const RenderPassName &name, const int2 &size) : RenderPassDesc{ name, RectI{int2(), size} }
		{}


		// render target
		RenderPassDesc&  AddTarget (RenderTargetName id, GfxResourceID image);
		RenderPassDesc&  AddTarget (RenderTargetName id, GfxResourceID image, EAttachmentLoadOp loadOp, EAttachmentStoreOp storeOp);
		
		template <typename ClearVal>
		RenderPassDesc&  AddTarget (RenderTargetName id, GfxResourceID image, const ClearVal &clearValue, EAttachmentStoreOp storeOp);
		RenderPassDesc&  AddTarget (RenderTargetName id, GfxResourceID image, const ImageViewDesc &desc, EAttachmentLoadOp loadOp, EAttachmentStoreOp storeOp);

		template <typename ClearVal>
		RenderPassDesc&  AddTarget (RenderTargetName id, GfxResourceID image, const ImageViewDesc &desc, const ClearVal &clearValue, EAttachmentStoreOp storeOp);


		// viewport
		template <typename T>
		RenderPassDesc&  AddViewport (const Rectangle<T> &rect, float minDepth = 0.0f, float maxDepth = 1.0f);
		
		template <typename T>
		RenderPassDesc&  AddViewport (const Vec<T,2> &size, float minDepth = 0.0f, float maxDepth = 1.0f);
	};

	

	inline RenderPassDesc&  RenderPassDesc::AddTarget (RenderTargetName id, GfxResourceID image)
	{
		return AddTarget( id, image, EAttachmentLoadOp::Load, EAttachmentStoreOp::Store );
	}


	inline RenderPassDesc&  RenderPassDesc::AddTarget (RenderTargetName id, GfxResourceID image, EAttachmentLoadOp loadOp, EAttachmentStoreOp storeOp)
	{
		ASSERT( image and image.ResourceType() == GfxResourceID::EType::Image );
		ASSERT( loadOp != EAttachmentLoadOp::Clear );	// clear value is not defined

		renderTargets.insert_or_assign( id, RT{ image, {}, ClearValue_t{}, loadOp, storeOp });
		return *this;
	}
	

	template <typename ClearVal>
	inline RenderPassDesc&  RenderPassDesc::AddTarget (RenderTargetName id, GfxResourceID image, const ClearVal &clearValue, EAttachmentStoreOp storeOp)
	{
		ASSERT( image and image.ResourceType() == GfxResourceID::EType::Image );

		renderTargets.insert_or_assign( id, RT{ image, {}, clearValue, EAttachmentLoadOp::Clear, storeOp });
		return *this;
	}
	

	inline RenderPassDesc&  RenderPassDesc::AddTarget (RenderTargetName id, GfxResourceID image, const ImageViewDesc &desc, EAttachmentLoadOp loadOp, EAttachmentStoreOp storeOp)
	{
		ASSERT( image and image.ResourceType() == GfxResourceID::EType::Image );
		ASSERT( loadOp != EAttachmentLoadOp::Clear );	// clear value is not defined

		renderTargets.insert_or_assign( id, RT{ image, desc, ClearValue_t{}, loadOp, storeOp });
		return *this;
	}


	template <typename ClearVal>
	inline RenderPassDesc&  RenderPassDesc::AddTarget (RenderTargetName id, GfxResourceID image, const ImageViewDesc &desc, const ClearVal &clearValue, EAttachmentStoreOp storeOp)
	{
		ASSERT( image and image.ResourceType() == GfxResourceID::EType::Image );

		renderTargets.insert_or_assign( id, RT{ image, desc, clearValue, EAttachmentLoadOp::Clear, storeOp });
		return *this;
	}


	template <typename T>
	inline RenderPassDesc&  RenderPassDesc::AddViewport (const Rectangle<T> &rect, float minDepth, float maxDepth)
	{
		ASSERT( rect.IsValid() );
		viewports.push_back({ RectF{rect}, minDepth, maxDepth, palette });
		return *this;
	}


	template <typename T>
	inline RenderPassDesc&  RenderPassDesc::AddViewport (const Vec<T,2> &size, float minDepth, float maxDepth)
	{
		viewports.push_back({ RectF{float2(), float2(size)}, minDepth, maxDepth });
		return *this;
	}


}	// AE::Graphics
