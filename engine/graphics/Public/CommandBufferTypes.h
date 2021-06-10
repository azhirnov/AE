// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "graphics/Public/Queue.h"
#include "graphics/Public/ImageDesc.h"
#include "graphics/Public/BufferDesc.h"
#include "graphics/Public/PipelineDesc.h"
#include "graphics/Public/RenderPassDesc.h"
#include "graphics/Public/IDs.h"
#include "graphics/Public/EResourceState.h"
#include "graphics/Public/ResourceEnums.h"
#include "graphics/Public/RenderStateEnums.h"
#include "graphics/Public/ShaderEnums.h"
#include "graphics/Public/VertexEnums.h"
#include "graphics/Public/BufferView.h"
#include "graphics/Public/ImageView.h"

#include "threading/TaskSystem/TaskScheduler.h"
#include "threading/TaskSystem/Promise.h"

namespace AE::Graphics
{
	using AE::Threading::Promise;


	struct ImageSubresourceRange
	{
		EImageAspect	aspectMask		= Default;
		MipmapLevel		baseMipLevel;
		uint			levelCount		= 1;
		ImageLayer		baseArrayLayer;
		uint			layerCount		= 1;
	};


	struct ImageSubresourceLayers
	{
		EImageAspect	aspectMask		= Default;
		MipmapLevel		mipLevel;
		ImageLayer		baseArrayLayer;
		uint			layerCount		= 1;
	};


	using ClearColor_t	= Union< RGBA32f, RGBA32i, RGBA32u >;


	struct BufferCopy
	{
		Bytes			srcOffset;
		Bytes			dstOffset;
		Bytes			size;
	};
		

	struct ImageCopy
	{
		ImageSubresourceLayers	srcSubresource;
		uint3					srcOffset;
		ImageSubresourceLayers	dstSubresource;
		uint3					dstOffset;
		uint3					extent;
	};


	struct BufferImageCopy
	{
		Bytes					bufferOffset;
		uint					bufferRowLength;
		uint					bufferImageHeight;
		ImageSubresourceLayers	imageSubresource;
		uint3					imageOffset;
		uint3					imageExtent;
	};
	

	struct ImageBlit
	{
		ImageSubresourceLayers	srcSubresource;
		uint3					srcOffset0;
		uint3					srcOffset1;
		ImageSubresourceLayers	dstSubresource;
		uint3					dstOffset0;
		uint3					dstOffset1;
	};


	struct ImageResolve
	{
		ImageSubresourceLayers	srcSubresource;
		uint3					srcOffset;
		ImageSubresourceLayers	dstSubresource;
		uint3					dstOffset;
		uint3					extent;
	};
	

	struct BufferStream
	{
		BufferID		buffer;
		Bytes			offset;
		Bytes			size;
		Bytes			_pos;
	};


	struct ImageStream
	{
		ImageID			image;
		uint3			offset;
		uint3			size;
		MipmapLevel		mipLevel;
		ImageLayer		arrayLayer;
		EImageAspect	aspectMask;
		uint2			_posYZ;
	};


	enum class ECommandBufferType : ubyte
	{
		Primary_OneTimeSubmit		= 0,
		Secondary_RenderCommands	= 1,
		//Secondary_OneTimeSubmit	= 2,
		_Count,
		Unknown						= 0xFF,
	};


	enum class EStagingHeapType : ubyte
	{
		Static		= 1 << 0,
		Dynamic		= 1 << 1,
	};


	enum class EBarrierPlacement
	{
		Manual,
		PerResource,
		PerRange,			// avoid barriers between different ranges of the same resource
		CommandReordering,	// command buffer can reorder commands to optimize synchronizations
	};


	enum class ESubmitMode
	{
		// Submit batch immediately when all render tasks has been complete.
		Immediately,

		// Allows RG to accumulate batches to minimize CPU overhead.
		Deferred,
	};



	//
	// GPU to CPU Sync interface
	//

	class IFence : public EnableRC< IFence >
	{
	// interface
	public:
		ND_ virtual bool  Wait (nanoseconds timeout) = 0;
		ND_ virtual bool  IsComplete () = 0;
	};

	using FencePtr = RC< IFence >;
	


}	// AE::Graphics

