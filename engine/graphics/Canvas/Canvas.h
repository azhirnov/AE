// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "graphics/Canvas/SurfaceDimensions.h"
#include "graphics/Font/VectorFont.h"
#include "graphics/Public/RenderGraph.h"

namespace AE::Graphics
{
	using BatchIndex_t = uint16_t;

	enum class EBlendMode : uint
	{
		None	= 0,
		Multiplicative,
		MultiplicativePremultiplied,
		Unknown	= None,
	};



	//
	// Canvas
	//

	class Canvas final : public SurfaceDimensions
	{
	// types
	public:
		struct Texture
		{
			UniformName			uniform;
			GfxResourceID		image;
			SamplerName			sampler;

			ND_ bool  operator == (const Texture &rhs) const;
		};

		struct UBuffer
		{
			UniformName			uniform;
			GfxResourceID		buffer;

			ND_ bool  operator == (const UBuffer &rhs) const;
		};

		struct Material
		{
			PipelineName				pplnName;
			RenderState::ColorBuffer	colorBuffer;

			DescriptorSetID				descSet;
			FixedArray<uint, 4>			dynamicOffsets;

			Material () {}

			Material&  SetPipeline (const PipelineName &name);
			Material&  SetDescriptorSet (DescriptorSetID ds, ArrayView<uint> offsets = Default);
			Material&  SetBlending (EBlendFactor srcBlendFactor, EBlendFactor dstBlendFactor, EBlendOp blendOp, bool4 colorMask = bool4{true});
			Material&  SetBlending (EBlendFactor srcBlendFactorColor, EBlendFactor srcBlendFactorAlpha,
									EBlendFactor dstBlendFactorColor, EBlendFactor dstBlendFactorAlpha,
									EBlendOp blendOpColor, EBlendOp blendOpAlpha, bool4 colorMask = bool4{true});
			Material&  SetBlending (EBlendMode mode);
			Material&  SetColorMask (bool4 colorMask);

			ND_ bool operator == (const Material &);
		};

	private:
		using VertexInputPtr	= Ptr< const VertexInputState >;
		using VertexInputMap_t	= HashMap< Pair<std::type_index, BytesU>, VertexInputState >;

		struct Batch
		{
			Material			material;
			VertexInputPtr		vertexInput;
			EPrimitive			topology	= Default;
			BytesU				vertexOffset;
			BytesU				indexOffset;
			uint				vertexCount	= 0;
			uint				indexCount	= 0;
			uint16_t			bufIndex	= UMax;

			Batch (const Material &mtr) : material{mtr} {}
		};

		struct Buffer
		{
			void *					vertices;
			void *					indices;
			BytesU					vertSize;
			BytesU					vertCapacity;
			BytesU					indexSize;
			BytesU					indexCapacity;
			UniqueID<GfxResourceID>	id;
		};
		using Buffers_t		= FixedArray< Buffer, 16 >;


		static constexpr BytesU		vertexStride		= 16_b;
		static constexpr BytesU		indexStride			= SizeOf<BatchIndex_t>;
		static constexpr uint		maxVertexCount		= 1u << 12;
		static constexpr BytesU		vertexBufferSize	= vertexStride * maxVertexCount;
		static constexpr BytesU		indexBufferSize		= indexStride * maxVertexCount * 3;
		static constexpr BytesU		bufferSize			= vertexBufferSize + indexBufferSize;


	// variables
	private:
		VertexInputMap_t	_vertexInputMap;
		Deque<Batch>		_batches;

		Buffers_t			_buffers;
		uint				_usedVBuffers	: 8;

		bool				_changed		= false;
		
		LinearAllocator<>	_allocator;

		IResourceManager &	_resMngr;


	// methods
	public:
		explicit Canvas (IResourceManager &rm);
		~Canvas ();

		bool  Flush (ITransferContext &ctx);
		void  Draw (IRenderContext &ctx);
		void  Clear ();


	// High level //
		void  DrawText (const VectorFont::PrecalculatedText &text, const FontPtr &font, const RectF &regionInPx, RGBA8u colorMask,
					    INOUT Material &mtr, INOUT DescriptorSet &ds, const UniformName &texName);


	// Low level //
		template <typename BatchType>
		void  AddBatch (const Material &mtr, const BatchType &batch);
		
		//template <typename BatchType>
		//void  AddBatch (const BatchType &batch);


	private:
		template <typename VertexType>
		ND_ VertexInputPtr  _AddAttribs (BytesU stride);
		
		template <typename BatchType>
		void  _AddPrimitive (const BatchType &src, BytesU vertStride, BytesU vertBufSize, BytesU idxBufSize, INOUT Batch &dst);
		
		template <typename BatchType>
		void  _BreakStrip (const BatchType &src, BytesU vertStride, BytesU vertBufSize, BytesU idxBufSize, INOUT Batch &dst);

		template <typename BatchType>
		void  _ContinueStrip (const BatchType &src, BytesU vertStride, BytesU vertBufSize, BytesU idxBufSize, INOUT Batch &dst);

		ND_ Batch*  _AppendBatch (const Material &mtr, VertexInputPtr attribs, EPrimitive topology,
								  BytesU vertStride, BytesU vertBufSize, BytesU idxBufSize);
	};

	
/*
=================================================
	AddBatch
=================================================
*/
	template <typename BatchType>
	inline void  Canvas::AddBatch (const Material &mtr, const BatchType &batch)
	{
		const BytesU	stride	= AlignToLarger( SizeOf<typename BatchType::Vertex_t>, vertexStride );
		VertexInputPtr	attribs = _AddAttribs< typename BatchType::Vertex_t >( stride );
		const BytesU	vb_size	= batch.VertexCount() * stride;
		const BytesU	ib_size	= batch.IndexCount() * SizeOf<BatchIndex_t>;
		Batch *			dst		= _AppendBatch( mtr, attribs, batch.Topology(), vb_size, stride, ib_size );

		if ( dst )
			_AddPrimitive( batch, stride, vb_size, ib_size, *dst );
	}
	
/*
=================================================
	_AddPrimitive
=================================================
*/
	template <typename BatchType>
	inline void  Canvas::_AddPrimitive (const BatchType &src, BytesU vertStride, BytesU vertBufSize, BytesU idxBufSize, INOUT Batch &dst)
	{
		if constexpr( src.Topology() == EPrimitive::LineStrip or
					  src.Topology() == EPrimitive::TriangleStrip )
		{
			_BreakStrip( src, vertStride, vertBufSize, idxBufSize, INOUT dst );
		}else
			_ContinueStrip( src, vertStride, vertBufSize, idxBufSize, INOUT dst );
	}

/*
=================================================
	_BreakStrip
=================================================
*/
	template <typename BatchType>
	inline void  Canvas::_BreakStrip (const BatchType &src, BytesU vertStride, BytesU vertBufSize, BytesU idxBufSize, INOUT Batch &dst)
	{
		using Vertex_t = typename BatchType::Vertex_t;

		auto&	buf		= _buffers[ dst.bufIndex ];
		auto*	indices	= Cast<BatchIndex_t>(buf.indices + buf.indexSize);
		uint	off		= dst.indexCount ? 2 : 0;

		src.GetVertices( OUT buf.vertices + buf.vertSize, vertStride );
		src.GetIndices( OUT indices + off, BatchIndex_t(dst.vertexCount) );

		if ( off ) {
			indices[0]	= indices[-1];
			indices[1]	= indices[2];
		}

		buf.vertSize	+= vertBufSize;
		buf.indexSize	+= idxBufSize + off * indexStride;

		dst.vertexCount += src.VertexCount();
		dst.indexCount  += src.IndexCount() + off;

		_changed = true;
	}

/*
=================================================
	_ContinueStrip
=================================================
*/
	template <typename BatchType>
	inline void  Canvas::_ContinueStrip (const BatchType &src, BytesU vertStride, BytesU vertBufSize, BytesU idxBufSize, INOUT Batch &dst)
	{
		auto&	buf = _buffers[ dst.bufIndex ];

		src.GetVertices( OUT buf.vertices + buf.vertSize, vertStride );
		src.GetIndices( OUT Cast<BatchIndex_t>(buf.indices + buf.indexSize), BatchIndex_t(dst.vertexCount) );

		buf.vertSize	+= vertBufSize;
		buf.indexSize	+= idxBufSize;

		dst.vertexCount += src.VertexCount();
		dst.indexCount  += src.IndexCount();

		_changed = true;
	}

/*
=================================================
	_AddAttribs
=================================================
*/
	template <typename VertexType>
	inline Canvas::VertexInputPtr  Canvas::_AddAttribs (BytesU stride)
	{
		auto[iter, inserted] = _vertexInputMap.insert({ {VertexType::GetTypeId(), stride}, {} });

		if ( inserted )
			iter->second = VertexType::GetAttribs( stride );
		
		return &iter->second;
	}


}	// AE::Graphics
