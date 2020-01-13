// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef AE_ENABLE_VULKAN

# include "graphics/Public/ImageDesc.h"
# include "graphics/Vulkan/VCommon.h"

namespace AE::Graphics
{

	//
	// Vulkan Framebuffer
	//

	class VFramebuffer final
	{
	// types
	private:
		using Attachments_t	= FixedArray< Pair<VImageID, ImageViewDesc>, GraphicsConfig::MaxAttachments >;

		
	// variables
	private:
		HashVal					_hash;
		VkFramebuffer			_framebuffer	= VK_NULL_HANDLE;
		UniqueID<RenderPassID>	_renderPassId;

		uint2					_dimension;
		ImageLayer				_layers;
		Attachments_t			_attachments;
		
		DebugName_t				_debugName;
		
		RWDataRaceCheck			_drCheck;

		
	// methods
	public:
		VFramebuffer () {}
		VFramebuffer (ArrayView<Pair<VImageID, ImageViewDesc>> attachments, UniqueID<RenderPassID> rp, uint2 dim, uint layers);
		~VFramebuffer ();
		
		bool Create (VResourceManager &, StringView dbgName);
		void Destroy (VResourceManager &);
		
		ND_ bool IsAllResourcesAlive (const VResourceManager &) const;

		ND_ bool operator == (const VFramebuffer &rhs) const;

		ND_ VkFramebuffer		Handle ()			const	{ SHAREDLOCK( _drCheck );  return _framebuffer; }
		ND_ RenderPassID		GetRenderPassID ()	const	{ SHAREDLOCK( _drCheck );  return _renderPassId; }
		ND_ uint2 const&		Dimension ()		const	{ SHAREDLOCK( _drCheck );  return _dimension; }
		ND_ uint				Layers ()			const	{ SHAREDLOCK( _drCheck );  return _layers.Get(); }
		ND_ HashVal				GetHash ()			const	{ SHAREDLOCK( _drCheck );  return _hash; }
	};


}	// AE::Graphics


namespace std
{
	template <>
	struct hash< AE::Graphics::VFramebuffer > {
		ND_ size_t  operator () (const AE::Graphics::VFramebuffer &value) const {
			return size_t(value.GetHash());
		}
	};

}	// std

#endif	// AE_ENABLE_VULKAN
