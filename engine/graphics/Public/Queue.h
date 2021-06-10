// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "graphics/Public/Common.h"

namespace AE::Graphics
{

	enum class EQueueType : ubyte
	{
		Graphics,			// also supports compute and transfer commands
		AsyncCompute,		// separate compute queue
		AsyncTransfer,		// separate transfer queue
		_Count,
		Unknown			= 0xFF,
	};


	enum class EQueueMask : ushort
	{
		Graphics		= 1 << ushort(EQueueType::Graphics),
		AsyncCompute	= 1 << ushort(EQueueType::AsyncCompute),
		AsyncTransfer	= 1 << ushort(EQueueType::AsyncTransfer),
		All				= Graphics | AsyncCompute | AsyncTransfer,
		Unknown			= 0,
	};
	AE_BIT_OPERATORS( EQueueMask );
	

	forceinline constexpr EQueueMask&  operator |= (EQueueMask &lhs, EQueueType rhs)
	{
		ASSERT( uint(rhs) < 16 );
		return lhs = BitCast<EQueueMask>(ushort( uint(lhs) | (1u << (uint(rhs) & 15)) ));
	}

	forceinline constexpr EQueueMask   operator |  (EQueueMask lhs, EQueueType rhs)
	{
		ASSERT( uint(rhs) < 16 );
		return BitCast<EQueueMask>(ushort( uint(lhs) | (1u << (uint(rhs) & 15)) ));
	}

	forceinline constexpr EQueueMask   operator &  (EQueueMask lhs, EQueueType rhs)
	{
		ASSERT( uint(rhs) < 16 );
		return BitCast<EQueueMask>(ushort( uint(lhs) & (1u << (uint(rhs) & 15)) ));
	}


}	// AE::Graphics
