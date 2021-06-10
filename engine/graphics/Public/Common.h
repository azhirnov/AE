// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Algorithms/ArrayUtils.h"
#include "stl/Math/BitMath.h"
#include "stl/Math/Math.h"
#include "stl/Math/Vec.h"
#include "stl/Math/Color.h"
#include "stl/Math/Rectangle.h"
#include "stl/Utils/Noncopyable.h"
#include "stl/Utils/RefCounter.h"
#include "stl/Containers/Union.h"
#include "stl/Containers/FixedMap.h"
#include "stl/Containers/FixedString.h"
#include "stl/Containers/FixedTupleArray.h"
#include "stl/CompileTime/Math.h"
#include "stl/Stream/Stream.h"


namespace AE::Graphics
{
	using namespace AE::STL;
	using namespace AE::Math;


	struct GraphicsConfig
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
			//static constexpr bool	EnableShaderDebugging	= false;
			static constexpr uint	MaxDescriptorSets		= 4;

			static constexpr bool	UseTimelineSemaphore	= false;

		#else
		
			// buffer
			static constexpr uint	MaxVertexBuffers		= 8;
			static constexpr uint	MaxVertexAttribs		= 16;

			// render pass
			static constexpr uint	MaxColorBuffers			= 8;
			static constexpr uint	MaxViewports			= 16;
			static constexpr uint	MaxRenderPassSubpasses	= 8;
			
			// pipeline
			//static constexpr bool	EnableShaderDebugging	= true;
			static constexpr uint	MaxDescriptorSets		= 8;
			
			static constexpr bool	UseTimelineSemaphore	= false;
		#endif

			static constexpr uint	MaxPushConstants		= 8;
			static constexpr uint	MaxSpecConstants		= 8;

			static constexpr uint	MaxAttachments			= MaxColorBuffers + 1;	// color + depth_stencil

			static constexpr uint	MaxFrames				= 4;
			static constexpr uint	MaxCmdBuffersPerPool	= 16;
			static constexpr uint	MaxCmdPoolsPerQueue		= 8;	// == max render threads

			static constexpr uint	MaxCmdBufPerBatch		= 32;

			static constexpr Bytes	MinBufferBlockSize		= 64_b;	// for stream uploading
	};


	struct DeviceLimits
	{
		static constexpr uint	MinUniformBufferOffsetAlignment			= 256;		// nvidia - 64/256, amd - 16,   intel - 64,   mali -  16,  adreno - 64
		static constexpr uint	MinStorageBufferOffsetAlignment			= 256;		// nvidia - 16,     amd -  4,   intel - 64,   mali - 256,  adreno - 64
		static constexpr uint	MinTexelBufferOffsetAlignment			= 256;		// nvidia - 16,     amd -  4,   intel - 64,   mali - 256,  adreno - 64
		static constexpr uint	MinMemoryMapAlignment					= 64;		// nvidia - 64,     amd - 64,   intel - 64,   mali -  64,  adreno - 64
		static constexpr uint	MinNonCoherentAtomSize					= 128;		// nvidia - 64,     amd - 128,  intel -  1,   mali - 64,   adreno -  1
		static constexpr uint	MaxUniformBufferRange					= 65536;	// nvidia - 64k,    amd - inf,  intel - inf,  mali - 64k,  adreno - 64k
		static constexpr uint	MaxDescriptorSetUniformBuffersDynamic	= 8;		// nvidia - 15,     amd -  8,   intel - 16,   mali -  8,   adreno - 32
		static constexpr uint	MaxDescriptorSetStorageBuffersDynamic	= 4;		// nvidia - 16,     amd -  8,   intel - 16,   mali -  4,   adreno - 16
		static constexpr uint	MaxDescriptorSetInputAttachments		= 4;
		static constexpr uint	MaxColorAttachments						= 4;		// nvidia -  8,     amd -  8,   intel -  8,   mali -  4,   adreno -  8
		static constexpr uint	MaxBoundDescriptorSets					= 4;		// nvidia - 32,     amd - 32,   intel -  8,   mali -  4,   adreno -  4
		static constexpr uint	MaxVertexInputAttributes				= 16;		// nvidia - 32,     amd - 64,   intel - 32,   mali - 16,   adreno - 32
		static constexpr uint	MinOptimalBufferCopyOffsetAlignment		= 64;		// nvidia -  1,     amd -  1,   intel - 64,   mali - 64,   adreno - 64
		static constexpr uint	MinOptimalBufferCopyRowPitchAlignment	= 64;		// nvidia -  1,     amd -  1,   intel - 64,   mali - 64,   adreno - 64
		static constexpr uint	MaxPushConstantsSize					= 128;		// nvidia - 256,    amd - 128,  intel - 256,  mali - 128,  adreno - 128
		static constexpr uint	MinComputeWorkgroupInvocations			= 256;		// max - 1024
		static constexpr uint	MaxFramebufferLayers					= 256;
	};


	struct GraphicsCreateInfo
	{
		uint	maxFrames	= 2;

		using SizePerQueue_t = StaticArray< Bytes, 3 >;
		struct {
			// static staging buffers allocated at engine start
			SizePerQueue_t	writeStaticSize		= {};
			SizePerQueue_t	readStaticSize		= {};

			// dynamic buffers will be allocated when needed and will be released after,
			// but total size can be limited
			Bytes			maxWriteDynamicSize	= 256_Mb;
			Bytes			maxReadDynamicSize	= 256_Mb;

			// total size of staging memory is:
			//   (writeStaticSize + readStaticSize) * maxFrames + (maxWriteDynamicSize + maxReadDynamicSize)
		}	staging;
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
