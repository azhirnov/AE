// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef AE_ENABLE_VULKAN

# include "stl/Containers/StructView.h"
# include "graphics/Public/CommandBufferTypes.h"
# include "graphics/Vulkan/VCommon.h"

namespace AE::Graphics
{

	//
	// Local Resources Barriers Helper
	//

	template <typename BaseType>
	class VLocalResBarrierHelper final : public BaseType
	{
		STATIC_ASSERT( BaseType::HasAutoSync );

	// types
	private:
		using LocalImage	= typename BaseType::LocalImage;
		using LocalBuffer	= typename BaseType::LocalBuffer;

	// variables
	private:


	// methods
	public:
		explicit VLocalResBarrierHelper (const VCommandBatch& batch) : BaseType{batch} {}

		void  AddImage (const LocalImage *img, EResourceState state, VkImageLayout layout);
		void  AddImage (const LocalImage *img, EResourceState state, VkImageLayout layout, StructView<ImageSubresourceLayers> subresLayers);
		void  AddImage (const LocalImage *img, EResourceState state, VkImageLayout layout, StructView<ImageSubresourceRange> subres);

		void  AddBuffer (const LocalBuffer *buf, EResourceState state);
		void  AddBuffer (const LocalBuffer *buf, EResourceState state, Bytes offset, Bytes size);
		void  AddBuffer (const LocalBuffer *buf, EResourceState state, ArrayView<BufferImageCopy> copy);
	};

	
/*
=================================================
	_AddBuffer
=================================================
*/
	template <typename BaseType>
	inline void  VLocalResBarrierHelper<BaseType>::AddBuffer (const LocalBuffer *buf, EResourceState state)
	{
		if ( not buf->IsMutable() )
			return;

		buf->AddPendingState( state, _cmdCounter );

		_pendingBarriers.buffers.insert( buf );
	}


}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
