// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Algorithms/EnumUtils.h"
#include "stl/Algorithms/ArrayUtils.h"
#include "stl/Math/Vec.h"
#include "stl/Types/Noncopyable.h"

namespace AE::Graphics
{
	using namespace AE::STL;

}	// AE::Graphics


// check definitions
#ifdef AE_CPP_DETECT_MISSMATCH

#  ifdef AE_ENABLE_VULKAN
#	pragma detect_mismatch( "AE_ENABLE_VULKAN", "1" )
#  else
#	pragma detect_mismatch( "AE_ENABLE_VULKAN", "0" )
#  endif

#  ifdef AE_ENABLE_OPENGL
#	pragma detect_mismatch( "AE_ENABLE_OPENGL", "1" )
#  else
#	pragma detect_mismatch( "AE_ENABLE_OPENGL", "0" )
#  endif

#endif	// AE_CPP_DETECT_MISSMATCH
