// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "graphics/Public/Common.h"
#include "stl/Types/HandleTmpl.h"
#include "stl/Types/NamedID.h"

namespace AE::Graphics
{
	using RenderPassID			= HandleTmpl< uint16_t, uint16_t, __COUNTER__ >;
	using MemoryID				= HandleTmpl< uint16_t, uint16_t, __COUNTER__ >;

	using GraphicsPipelineID	= HandleTmpl< uint16_t, uint16_t, __COUNTER__ >;
	using MeshPipelineID		= HandleTmpl< uint16_t, uint16_t, __COUNTER__ >;
	using ComputePipelineID		= HandleTmpl< uint16_t, uint16_t, __COUNTER__ >;
	using RayTracingPipelineID	= HandleTmpl< uint16_t, uint16_t, __COUNTER__ >;
	using DescriptorSetID		= HandleTmpl< uint16_t, uint16_t, __COUNTER__ >;
	using PipelinePackID		= HandleTmpl< uint16_t, uint16_t, __COUNTER__ >;


	using UniformName			= NamedID< 32, __COUNTER__, AE_OPTIMIZE_IDS >;
	using PushConstantName		= NamedID< 32, __COUNTER__, AE_OPTIMIZE_IDS >;
	using SpecializationName	= NamedID< 32, __COUNTER__, AE_OPTIMIZE_IDS >;
	using DescriptorSetName		= NamedID< 32, __COUNTER__, AE_OPTIMIZE_IDS >;
	using PipelineName			= NamedID< 64, __COUNTER__, AE_OPTIMIZE_IDS >;
	
	using VertexName			= NamedID< 32, __COUNTER__, AE_OPTIMIZE_IDS >;
	using VertexBufferName		= NamedID< 32, __COUNTER__, AE_OPTIMIZE_IDS >;

	using SamplerName			= NamedID< 64, __COUNTER__, AE_OPTIMIZE_IDS >;
	using RenderTargetName		= NamedID< 32, __COUNTER__, AE_OPTIMIZE_IDS >;


	
	//
	// Resource ID
	//

	struct GfxResourceID
	{
	// types
	public:
		enum class EType
		{
			Unknown		= 0,	// for execution dependency
			Buffer,
			Image,
			RayTracingGeometry,	// BLAS
			RayTracingScene,	// TLAS
			
			VirtualBuffer,
			VirtualImage,
			
			_Count,
			_VirtualBegin = VirtualBuffer,
		};

		using Self			= GfxResourceID;
		using Index_t		= uint16_t;
		using Generation_t	= uint16_t;
		using Value_t		= BitSizeToUInt< sizeof(Index_t) + sizeof(Generation_t) >;


	// variables
	private:
		Value_t		_value	= UMax;
		
		static constexpr Value_t	_IndexSize	= 15;
		static constexpr Value_t	_GenSize	= 14;
		static constexpr Value_t	_ResSize	= 3;

		static constexpr Value_t	_IndexMask	= (1u << _IndexSize) - 1;
		static constexpr Value_t	_GenOffset	= _IndexSize;
		static constexpr Value_t	_GenMask	= (1u << _GenSize) - 1;
		static constexpr Value_t	_ResOffset	= _IndexSize + _GenSize;
		static constexpr Value_t	_ResMask	= (1u << _ResSize) - 1;

		STATIC_ASSERT( sizeof(_value) >= (sizeof(Index_t) + sizeof(Generation_t)) );
		STATIC_ASSERT( (_IndexSize + _GenSize + _ResSize) == sizeof(_value)*8 );
		STATIC_ASSERT( uint(EType::_Count) <= _ResMask );


	// methods
	public:
		constexpr GfxResourceID () {}

		constexpr GfxResourceID (const Self &other) :
			_value{other._value}
			DEBUG_ONLY(, _class{other._class}) {}

		constexpr GfxResourceID (Value_t index, Value_t gen, EType type) :
			_value{ (index & _IndexMask) | ((gen & _GenMask) << _GenOffset) | ((Value_t(type) & _ResMask) << _ResOffset) } {}
		
		ND_ constexpr bool			IsValid ()						const	{ return _value != UMax; }
		ND_ constexpr Index_t		Index ()						const	{ return _value & _IndexMask; }
		ND_ constexpr Generation_t	Generation ()					const	{ return (_value >> _GenOffset) & _GenMask; }
		ND_ constexpr EType			ResourceType ()					const	{ return EType(_value >> _ResOffset); }
		ND_ constexpr bool			IsVirtual ()					const	{ return ResourceType() >= EType::_VirtualBegin; }
		ND_ HashVal					GetHash ()						const	{ return HashOf(_value); }
		ND_ constexpr Value_t		Data ()							const	{ return _value; }

		ND_ constexpr bool			operator == (const Self &rhs)	const	{ return _value == rhs._value; }
		ND_ constexpr bool			operator != (const Self &rhs)	const	{ return not (*this == rhs); }
		
		ND_ explicit constexpr		operator bool ()				const	{ return IsValid(); }
		

	// debugging
	public:
		DEBUG_ONLY(
			class IDbgBaseClass;
			IDbgBaseClass*	_class = null;
		)
	};

	// add this to output dependency to indicate that pass writes to non-virtual resource and must not be skiped.
	static constexpr GfxResourceID	GfxResourceID_External {UMax, UMax, GfxResourceID::EType::Unknown};


}	// AE::Graphics
