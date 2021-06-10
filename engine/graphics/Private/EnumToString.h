// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "graphics/Public/ResourceEnums.h"
#include "graphics/Public/EResourceState.h"
#include "graphics/Public/ShaderEnums.h"
#include "graphics/Public/VertexEnums.h"
#include "graphics/Public/Queue.h"

namespace AE::STL
{
	using EQueueType = Graphics::EQueueType;
	
/*
=================================================
	ToString (EQueueType)
=================================================
*/
	ND_ inline StringView  ToString (EQueueType queue)
	{
		BEGIN_ENUM_CHECKS();
		switch ( queue )
		{
			case EQueueType::Graphics :		return "Graphics";
			case EQueueType::AsyncCompute :	return "AsyncCompute";
			case EQueueType::AsyncTransfer:	return "AsyncTransfer";
			case EQueueType::_Count :
			case EQueueType::Unknown :		break;
		}
		END_ENUM_CHECKS();
		RETURN_ERR( "unknown queue type" );
	}


}	// AE::STL
