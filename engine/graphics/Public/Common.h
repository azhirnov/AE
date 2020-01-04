// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Algorithms/EnumUtils.h"
#include "stl/Algorithms/ArrayUtils.h"
#include "stl/Math/Math.h"
#include "stl/Math/Vec.h"
#include "stl/Math/Color.h"
#include "stl/Math/Rectangle.h"
#include "stl/Types/Noncopyable.h"
#include "stl/Containers/Union.h"
#include "stl/Containers/FixedMap.h"
#include "stl/Containers/FixedString.h"
#include "stl/CompileTime/Math.h"
#include "stl/Stream/Stream.h"

namespace AE::Graphics
{
	using namespace AE::STL;
	using namespace AE::Math;


	struct Graphics_Config
	{
		#ifdef PLATFORM_ANDROID
			// buffer
			static constexpr uint	MaxVertexBuffers		= 8;
			static constexpr uint	MaxVertexAttribs		= 16;

			// render pass
			static constexpr uint	MaxColorBuffers			= 4;
			static constexpr uint	MaxViewports			= 1;
			static constexpr uint	MaxRenderPassSubpasses	= 8;
		
			// pipeline
			static constexpr bool	EnableShaderDebugging	= false;
			static constexpr uint	MaxDescriptorSets		= 4;

		#else
		
			// buffer
			static constexpr uint	MaxVertexBuffers		= 8;
			static constexpr uint	MaxVertexAttribs		= 16;

			// render pass
			static constexpr uint	MaxColorBuffers			= 8;
			static constexpr uint	MaxViewports			= 16;
			static constexpr uint	MaxRenderPassSubpasses	= 8;
			
			// pipeline
			static constexpr bool	EnableShaderDebugging	= true;
			static constexpr uint	MaxDescriptorSets		= 8;
		#endif

			static constexpr uint	MaxPushConstants		= 8;
			static constexpr uint	MaxSpecConstants		= 8;
	};

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
