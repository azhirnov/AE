// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "platform/Public/Common.h"

namespace AE::App
{

	enum class EGraphicsApi : uint
	{
		Vulkan			= 1 << 0,
		OpenGL			= 1 << 1,
		OpenGLES_3		= 1 << 2,
		OpenGLES_2		= 1 << 3,
	};
	AE_BIT_OPERATORS( EGraphicsApi );


}	// AE::App
