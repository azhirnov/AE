// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "graphics/Public/Common.h"
#include "stl/Types/HandleTmpl.h"
#include "stl/Types/NamedID.h"

namespace AE::Graphics
{
namespace _ae_graphics_hidden_
{
	static constexpr uint	GraphicsIDs_Start	= 0x10000000;
	static constexpr uint	VulkanIDs_Start		= 0x20000000;

	static constexpr uint	NamedIDs_Start		= 0x10000000;

}	// _ae_graphics_hidden_


	using RenderPassID			= HandleTmpl< uint16_t, uint16_t, _ae_graphics_hidden_::GraphicsIDs_Start + 1 >;
	using CmdBatchID			= HandleTmpl< uint16_t, uint16_t, _ae_graphics_hidden_::GraphicsIDs_Start + 2 >;
	using BakedCommandBufferID	= HandleTmpl< uint16_t, uint16_t, _ae_graphics_hidden_::GraphicsIDs_Start + 3 >;

	using GraphicsPipelineID	= HandleTmpl< uint16_t, uint16_t, _ae_graphics_hidden_::GraphicsIDs_Start + 4 >;
	using MeshPipelineID		= HandleTmpl< uint16_t, uint16_t, _ae_graphics_hidden_::GraphicsIDs_Start + 5 >;
	using ComputePipelineID		= HandleTmpl< uint16_t, uint16_t, _ae_graphics_hidden_::GraphicsIDs_Start + 6 >;
	using RayTracingPipelineID	= HandleTmpl< uint16_t, uint16_t, _ae_graphics_hidden_::GraphicsIDs_Start + 7 >;
	using DescriptorSetID		= HandleTmpl< uint16_t, uint16_t, _ae_graphics_hidden_::GraphicsIDs_Start + 8 >;
	using PipelinePackID		= HandleTmpl< uint16_t, uint16_t, _ae_graphics_hidden_::GraphicsIDs_Start + 9 >;
	using DescriptorSetLayoutID	= HandleTmpl< uint16_t, uint16_t, _ae_graphics_hidden_::GraphicsIDs_Start + 10 >;


	using UniformName			= NamedID< 32, _ae_graphics_hidden_::NamedIDs_Start + 1,  AE_OPTIMIZE_IDS >;
	using PushConstantName		= NamedID< 32, _ae_graphics_hidden_::NamedIDs_Start + 2,  AE_OPTIMIZE_IDS >;
	using SpecializationName	= NamedID< 32, _ae_graphics_hidden_::NamedIDs_Start + 3,  AE_OPTIMIZE_IDS >;
	using DescriptorSetName		= NamedID< 32, _ae_graphics_hidden_::NamedIDs_Start + 4,  AE_OPTIMIZE_IDS >;
	using PipelineName			= NamedID< 64, _ae_graphics_hidden_::NamedIDs_Start + 5,  AE_OPTIMIZE_IDS >;
	
	using VertexName			= NamedID< 32, _ae_graphics_hidden_::NamedIDs_Start + 6,  AE_OPTIMIZE_IDS >;
	using VertexBufferName		= NamedID< 32, _ae_graphics_hidden_::NamedIDs_Start + 7,  AE_OPTIMIZE_IDS >;

	using SamplerName			= NamedID< 64, _ae_graphics_hidden_::NamedIDs_Start + 8,  AE_OPTIMIZE_IDS >;
	using RenderTargetName		= NamedID< 32, _ae_graphics_hidden_::NamedIDs_Start + 9,  AE_OPTIMIZE_IDS >;
	using RenderPassName		= NamedID< 32, _ae_graphics_hidden_::NamedIDs_Start + 10, AE_OPTIMIZE_IDS >;


	static constexpr RenderTargetName	RenderTarget_Depth {"Depth"};
	static constexpr RenderTargetName	RenderTarget_DepthStencil {"DepthStencil"};


	
	//
	// Graphics Resource ID
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
		constexpr GfxResourceID () : _value{UMax} {}

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
		
		ND_ static constexpr uint	MaxIndex ()								{ return _IndexMask; }
		ND_ static constexpr uint	MaxGeneration ()						{ return _GenMask; }


	// debugging
	public:
		DEBUG_ONLY(
			class IDbgBaseClass;
			IDbgBaseClass*	_class = null;
		)
	};

	// add this to output dependency to indicate that pass writes to non-virtual resource and must not be skiped.
	static constexpr GfxResourceID	GfxResourceID_External {UMax, UMax, GfxResourceID::EType::Unknown};



	//
	// Unique ID
	//

	template <typename IDType>
	struct UniqueID
	{
	// types
	public:
		using ID_t	= IDType;
		using Self	= UniqueID< IDType >;


	// variables
	private:
		ID_t	_id;


	// methods
	public:
		UniqueID ()													{}
		UniqueID (Self &&other) : _id{other._id}					{ other._id = Default; }
		explicit UniqueID (const ID_t &id) : _id{id}				{}
		~UniqueID ()												{ ASSERT(not IsValid()); }	// handle must be released
		
		Self&				Set (ID_t id)							{ ASSERT(not IsValid());  _id = id;  return *this; }

		Self&				operator = (Self &&rhs)					{ ASSERT(not IsValid());  _id = rhs._id;  rhs._id = Default;  return *this; }
		Self&				operator = (const Self &rhs)			{ ASSERT(not IsValid());  _id = rhs._id;  rhs._id = Default;  return *this; }
		
		ND_ ID_t			Release ()								{ ID_t temp{_id};  _id = Default;  return temp; }
		ND_ bool			IsValid ()						const	{ return bool(_id); }

		ND_ ID_t const&		operator * ()					const	{ return _id; }
		ND_ ID_t const*		operator -> ()					const	{ return &_id; }

		ND_ bool			operator == (const Self &rhs)	const	{ return _id == rhs._id; }
		ND_ bool			operator != (const Self &rhs)	const	{ return _id != rhs._id; }

		ND_ explicit		operator bool ()				const	{ return IsValid(); }

		ND_					operator ID_t ()				const	{ return _id; }
	};


}	// AE::Graphics

namespace std
{

	template <>
	struct hash< AE::Graphics::GfxResourceID >
	{
		ND_ size_t  operator () (const AE::Graphics::GfxResourceID &value) const {
			return size_t(value.GetHash());
		}
	};

}	// std
