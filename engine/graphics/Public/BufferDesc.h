// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "graphics/Public/ResourceEnums.h"
#include "graphics/Public/Queue.h"

namespace AE::Graphics
{

	//
	// Buffer Description
	//

	struct BufferDesc
	{
	// variables
		BytesU			size;
		EBufferUsage	usage		= Default;
		EQueueMask		queues		= Default;
		EMemoryType		memType		= Default;

	// methods
		BufferDesc () {}

		BufferDesc (BytesU			size,
					EBufferUsage	usage,
					EQueueMask		queues	= Default,
					EMemoryType		memType	= Default) :
			size{size}, usage{usage}, queues{queues}, memType{memType} {}
	};



	//
	// Virtual Buffer Description
	//

	struct VirtualBufferDesc
	{
	// variables
		BytesU			size;
		
	// methods
		VirtualBufferDesc () {}
		explicit VirtualBufferDesc (BytesU size) : size{size} {}
	};


}	// AE::Graphics
