// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "graphics/Public/Common.h"
#include "graphics/Public/VertexDesc.h"
#include "graphics/Public/IDs.h"

namespace AE::Graphics
{
	
	//
	// Vertex Input State
	//

	class VertexInputState
	{
	// types
	public:
		struct VertexAttrib
		{
			VertexName		id;
			uint			index;
			EVertexType		type;		// float|int|uint <1,2,3,4,...> are available
		};

		using Self	= VertexInputState;
		
		struct VertexInput
		{
		// variables
			EVertexType			type;	// src type, if src type is normalized short3, then dst type is float3.
			uint				index;
			Bytes<uint>			offset;
			uint				bufferBinding;

		// methods
			VertexInput ();
			VertexInput (EVertexType type, Bytes<uint> offset, uint bufferBinding);
			
			ND_ EVertexType ToDstType () const;

			ND_ bool  operator == (const VertexInput &rhs) const;
		};


		struct BufferBinding
		{
		// variables
			uint				index;
			Bytes<uint>			stride;
			EVertexInputRate	rate;

		// methods
			BufferBinding ();
			BufferBinding (uint index, Bytes<uint> stride, EVertexInputRate rate);
			
			ND_ bool  operator == (const BufferBinding &rhs) const;
		};

		static constexpr uint	BindingIndex_Auto	= UMax;
		static constexpr uint	VertexIndex_Unknown	= UMax;

		
		using Vertices_t	= FixedMap< VertexName, VertexInput, GraphicsConfig::MaxVertexAttribs >;
		using Bindings_t	= FixedMap< VertexBufferName, BufferBinding, GraphicsConfig::MaxVertexBuffers >;
		
		friend struct std::hash < VertexInputState::VertexInput >;
		friend struct std::hash < VertexInputState::BufferBinding >;
		friend struct std::hash < VertexInputState >;


	// variables
	private:
		Vertices_t		_vertices;
		Bindings_t		_bindings;


	// methods
	public:
		VertexInputState () {}

		template <typename ClassType, typename ValueType>
		Self & Add (const VertexName &id, ValueType ClassType:: *vertex, const VertexBufferName &bufferId = Default);

		template <typename ClassType, typename ValueType>
		Self & Add (const VertexName &id, ValueType ClassType:: *vertex, bool norm, const VertexBufferName &bufferId = Default);

		Self & Add (const VertexName &id, EVertexType type, BytesU offset, const VertexBufferName &bufferId = Default);

		Self & Bind (const VertexBufferName &bufferId, Bytes<uint> stride, uint index = BindingIndex_Auto, EVertexInputRate rate = EVertexInputRate::Vertex);
		Self & Bind (const VertexBufferName &bufferId, BytesU stride, uint index = BindingIndex_Auto, EVertexInputRate rate = EVertexInputRate::Vertex);

		void  Clear ();

		bool  ApplyAttribs (ArrayView<VertexAttrib> attribs);
		
		ND_ bool	operator == (const VertexInputState &rhs) const;
		ND_ HashVal	CalcHash () const;

		ND_ Vertices_t const&	Vertices ()			const	{ return _vertices; }
		ND_ Bindings_t const&	BufferBindings ()	const	{ return _bindings; }
	};
	
		
/*
=================================================
	Add
=================================================
*/
	template <typename ClassType, typename ValueType>
	inline VertexInputState&  VertexInputState::Add (const VertexName &id, ValueType ClassType:: *vertex, const VertexBufferName &bufferId)
	{
		return Add( id,
					VertexDesc< ValueType >::attrib,
					OffsetOf( vertex ),
					bufferId );
	}

/*
=================================================
	Add
=================================================
*/
	template <typename ClassType, typename ValueType>
	inline VertexInputState&  VertexInputState::Add (const VertexName &id, ValueType ClassType:: *vertex, bool norm, const VertexBufferName &bufferId)
	{
		const EVertexType	attrib = VertexDesc< ValueType >::attrib;

		return Add( id,
					norm ? (attrib | EVertexType::NormalizedFlag) : (attrib & ~EVertexType::NormalizedFlag),
					OffsetOf( vertex ),
					bufferId );
	}

}	// AE::Graphics
