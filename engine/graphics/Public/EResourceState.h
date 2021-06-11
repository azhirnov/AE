// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'
/*
	EResourceState contains information about pipeline stage,
	memory access mask, data invalidation and other.
*/

#pragma once

#include "graphics/Public/Common.h"

namespace AE::Graphics
{
	
	//
	// Resource State
	//
	enum class EResourceState : uint
	{
		Unknown = 0,

		// memory access
		_Access_ShaderStorage,				// uniform buffer, storage buffer, image storage
		_Access_Uniform,					// uniform buffer only
		_Access_ShaderSample,				// texture only
		_Access_InputAttachment,			// same as ShaderRead
		_Access_Transfer,					// copy buffer/image
		_Access_ColorAttachment,			// color render target
		_Access_DepthStencilAttachment,		// depth/stencil write/test
		_Access_Host,						// resource mapping
		_Access_Present,					// image only
		_Access_IndirectBuffer,
		_Access_IndexBuffer,
		_Access_VertexBuffer,
		_Access_ConditionalRendering,
		//_Access_CommandProcess,
		_Access_ShadingRateImage,
		_Access_BuildRayTracingAS,			// build/update acceleration structure for ray tracing
		_Access_RTASBuildingBuffer,			// vertex, index, ..., scratch buffer that used when build/update acceleration structure
		_AccessLast,
		_AccessMask				= (1 << 7) - 1,

		// shader stages
		_VertexShader			= 1 << 8,
		_TessControlShader		= 1 << 9,
		_TessEvaluationShader	= 1 << 10,
		_GeometryShader			= 1 << 11,
		_FragmentShader			= 1 << 12,
		_ComputeShader			= 1 << 13,
		_MeshTaskShader			= 1 << 14,
		_MeshShader				= 1 << 15,
		_RayGenShader			= 1 << 16,
		_RayAnyHitShader		= 1 << 17,
		_RayClosestHitShader	= 1 << 18,
		_RayMissShader			= 1 << 19,
		_RayIntersectionShader	= 1 << 20,
		_RayCallableShader		= 1 << 21,
		_RayTracingShaders		= _RayGenShader | _RayAnyHitShader | _RayClosestHitShader | _RayMissShader | _RayIntersectionShader | _RayCallableShader,
		_AllGraphics			= _VertexShader | _TessControlShader | _TessEvaluationShader |
								  _GeometryShader | _FragmentShader | _MeshTaskShader | _MeshShader,
		_ShaderMask				= _AllGraphics | _ComputeShader | _RayTracingShaders,

		// flags
		// unused range: 21..27

		// for ColorAttachment, DepthStencilAttachment
		//InvalidateBefore		= 1 << 26,				// discard image content before executing command
		//InvalidateAfter			= 1 << 27,				// discard image content after command execution

		// for DepthStencilAttachment
		EarlyFragmentTests		= 1 << 28,
		LateFragmentTests		= 1 << 29,

		// read/write access
		_Read					= 1u << 30,
		_Write					= 1u << 31,
		_ReadWrite				= _Read | _Write,

			
		// default states
		_StateMask						= _AccessMask | _ReadWrite,

		ShaderRead						= _Access_ShaderStorage | _Read,
		ShaderWrite						= _Access_ShaderStorage | _Write,
		ShaderReadWrite					= _Access_ShaderStorage | _ReadWrite,

		UniformRead						= _Access_Uniform | _Read,

		ShaderSample					= _Access_ShaderSample | _Read,
		InputAttachment					= _Access_InputAttachment | _Read,

		TransferSrc						= _Access_Transfer | _Read,
		TransferDst						= _Access_Transfer | _Write,
			
		ColorAttachmentRead				= _Access_ColorAttachment | _Read,
		ColorAttachmentWrite			= _Access_ColorAttachment | _Write,
		ColorAttachmentReadWrite		= _Access_ColorAttachment | _ReadWrite,
			
		DepthStencilAttachmentRead		= _Access_DepthStencilAttachment | _Read,
		DepthStencilAttachmentWrite		= _Access_DepthStencilAttachment | _Write,
		DepthStencilAttachmentReadWrite	= _Access_DepthStencilAttachment | _ReadWrite,

		HostRead						= _Access_Host | _Read,
		HostWrite						= _Access_Host | _Write,
		HostReadWrite					= _Access_Host | _ReadWrite,
			
		PresentImage					= _Access_Present | _Read,

		IndirectBuffer					= _Access_IndirectBuffer | _Read,
		IndexBuffer						= _Access_IndexBuffer | _Read,
		VertexBuffer					= _Access_VertexBuffer | _Read,
		
		BuildRayTracingStructRead		= _Access_BuildRayTracingAS | _Read,
		BuildRayTracingStructWrite		= _Access_BuildRayTracingAS | _Write,
		BuildRayTracingStructReadWrite	= _Access_BuildRayTracingAS | _Read | _Write,
		
		RTASBuildingBufferRead			= _Access_RTASBuildingBuffer | _Read,
		RTASBuildingBufferReadWrite		= _Access_RTASBuildingBuffer | _Read | _Write,

		RayTracingShaderRead			= ShaderRead | _RayTracingShaders,

		ShadingRateImageRead			= _Access_ShadingRateImage | _Read,
	};

	AE_BIT_OPERATORS( EResourceState );
	
	STATIC_ASSERT( EResourceState::_AccessLast <= EResourceState::_AccessMask );

}	// AE::Graphics
