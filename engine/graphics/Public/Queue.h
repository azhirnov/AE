// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "graphics/Public/Common.h"

namespace AE::Graphics
{

	enum class EQueueType : uint
	{
		Graphics,			// also supports compute and transfer commands
		AsyncCompute,		// separate compute queue
		AsyncTransfer,		// separate transfer queue
		//Present,			// queue must support present, may be a separate queue
		_Count,
		Unknown			= ~0u,
	};


	enum class EQueueMask : uint
	{
		Graphics		= 1 << uint(EQueueType::Graphics),
		AsyncCompute	= 1 << uint(EQueueType::AsyncCompute),
		AsyncTransfer	= 1 << uint(EQueueType::AsyncTransfer),
		All				= Graphics | AsyncCompute | AsyncTransfer,
		Unknown			= 0,
	};
	AE_BIT_OPERATORS( EQueueMask );
	

	forceinline constexpr EQueueMask&  operator |= (EQueueMask &lhs, EQueueType rhs)
	{
		ASSERT( uint(rhs) < 32 );
		return lhs = BitCast<EQueueMask>( uint(lhs) | (1u << (uint(rhs) & 31)) );
	}

	forceinline constexpr EQueueMask   operator |  (EQueueMask lhs, EQueueType rhs)
	{
		ASSERT( uint(rhs) < 32 );
		return BitCast<EQueueMask>( uint(lhs) | (1u << (uint(rhs) & 31)) );
	}


}	// AE::Graphics
