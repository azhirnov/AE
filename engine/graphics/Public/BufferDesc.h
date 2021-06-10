// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "graphics/Public/ResourceEnums.h"
#include "graphics/Public/Queue.h"

namespace AE::Graphics
{

	//
	// Buffer description
	//

	struct BufferDesc
	{
	// variables
		Bytes			size;
		EBufferUsage	usage		= Default;
		EQueueMask		queues		= Default;
		EMemoryType		memType		= EMemoryType::DeviceLocal;

	// methods
		BufferDesc () {}

		BufferDesc (Bytes			size,
					EBufferUsage	usage,
					EQueueMask		queues	= Default,
					EMemoryType		memType	= EMemoryType::DeviceLocal) :
			size{size}, usage{usage}, queues{queues}, memType{memType} {}
	};



	//
	// Virtual Buffer description
	//
	/*
	struct VirtualBufferDesc
	{
	// variables
		Bytes			size;
		
	// methods
		VirtualBufferDesc () {}
		explicit VirtualBufferDesc (Bytes size) : size{size} {}

		ND_ BufferDesc  ToPhysical (EVirtualResourceUsage usage) const;
	};
	*/


	//
	// Buffer View description
	//

	struct BufferViewDesc
	{
	// variables
		EPixelFormat		format	= Default;
		Bytes				offset;
		Bytes				size	{ ~0_b };

	// methods
		BufferViewDesc () {}

		BufferViewDesc (EPixelFormat	format,
						Bytes			offset,
						Bytes			size) :
			format{format}, offset{offset}, size{size} {}

		void Validate (const BufferDesc &desc);
		
		ND_ bool operator == (const BufferViewDesc &rhs) const;
	};


}	// AE::Graphics


namespace std
{
	template <>
	struct hash< AE::Graphics::BufferViewDesc > {
		ND_ size_t  operator () (const AE::Graphics::BufferViewDesc &value) const;
	};

}	// std
