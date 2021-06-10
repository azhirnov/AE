// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

# include "graphics/Public/CommandBuffer.h"

namespace AE::Graphics
{

	//
	// Transfer Context Helper
	//

	template <typename BaseType>
	class TransferCtxHelper : public BaseType
	{
		STATIC_ASSERT( BaseType::IsTransferContext );

	// methods
	public:
		explicit TransferCtxHelper (RC<VCommandBatch> batch) : BaseType{batch} {}
		/*
		template <typename T>
		bool  AllocBuffer (Bytes size, Bytes alignment, OUT BufferID &buffer, OUT Bytes &offset, OUT T* &mapped) {
			void*	ptr = null;
			bool	res = AllocBuffer( size, alignment, OUT buffer, OUT offset, OUT ptr );
			mapped = Cast<T>( ptr );
			return res;
		}*/

	};


}	// AE::Graphics
