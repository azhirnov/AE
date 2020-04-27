// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "graphics/Canvas/Canvas.h"
#include "graphics/Font/VectorFont.h"
#include "graphics/Font/FormattedText.h"
#include "graphics/Canvas/Vertices.h"
#include "stl/Algorithms/Utf8.h"

#ifndef AE_ENABLE_UTF8PROC
# error AE_ENABLE_UTF8PROC required
#endif

namespace AE::Graphics
{

/*
=================================================
	Texture::operator ==
=================================================
*/
	bool  Canvas::Texture::operator == (const Texture &rhs) const
	{
		return	uniform		== rhs.uniform	and
				image		== rhs.image	and
				sampler		== rhs.sampler;
	}
	
/*
=================================================
	UBuffer::operator ==
=================================================
*/
	bool  Canvas::UBuffer::operator == (const UBuffer &rhs) const
	{
		return	uniform		== rhs.uniform	and
				buffer		== rhs.buffer;
	}
//-----------------------------------------------------------------------------


/*
=================================================
	operator ==
=================================================
*/
	bool  Canvas::Material::operator == (const Material &rhs)
	{
		return	pplnName		== rhs.pplnName			and
				descSet			== rhs.descSet			and
				dynamicOffsets	== rhs.dynamicOffsets	and
				colorBuffer		== rhs.colorBuffer;
	}
	
/*
=================================================
	SetPipeline
=================================================
*/
	Canvas::Material&  Canvas::Material::SetPipeline (const PipelineName &name)
	{
		ASSERT( name.IsDefined() );
		pplnName = name;
		return *this;
	}
	
/*
=================================================
	SetDescriptorSet
=================================================
*/
	Canvas::Material&  Canvas::Material::SetDescriptorSet (DescriptorSetID ds, ArrayView<uint> offsets)
	{
		descSet			= ds;
		dynamicOffsets	= offsets;
		return *this;
	}

/*
=================================================
	SetBlending
=================================================
*/
	Canvas::Material&  Canvas::Material::SetBlending (EBlendFactor srcBlendFactor, EBlendFactor dstBlendFactor, EBlendOp blendOp, bool4 colorMask)
	{
		colorBuffer.srcBlendFactor	= srcBlendFactor;
		colorBuffer.dstBlendFactor	= dstBlendFactor;
		colorBuffer.blendOp			= blendOp;
		colorBuffer.colorMask		= colorMask;
		colorBuffer.blend			= true;
		return *this;
	}

	Canvas::Material&  Canvas::Material::SetBlending (EBlendFactor srcBlendFactorColor, EBlendFactor srcBlendFactorAlpha,
													  EBlendFactor dstBlendFactorColor, EBlendFactor dstBlendFactorAlpha,
													  EBlendOp blendOpColor, EBlendOp blendOpAlpha, bool4 colorMask)
	{
		colorBuffer.srcBlendFactor	= { srcBlendFactorColor, srcBlendFactorAlpha };
		colorBuffer.dstBlendFactor	= { dstBlendFactorColor, dstBlendFactorAlpha };
		colorBuffer.blendOp			= { blendOpColor, blendOpAlpha };
		colorBuffer.colorMask		= colorMask;
		colorBuffer.blend			= true;
		return *this;
	}

	Canvas::Material&  Canvas::Material::SetBlending (EBlendMode mode)
	{
		BEGIN_ENUM_CHECKS();
		switch ( mode )
		{
			case EBlendMode::None :
				break;

			case EBlendMode::Multiplicative :
				return SetBlending( EBlendFactor::SrcAlpha, EBlendFactor::One,
									EBlendFactor::OneMinusSrcAlpha, EBlendFactor::OneMinusSrcAlpha,
									EBlendOp::Add, EBlendOp::Add );

			case EBlendMode::MultiplicativePremultiplied :
				return SetBlending( EBlendFactor::One, EBlendFactor::OneMinusSrcAlpha, EBlendOp::Add );
		}
		END_ENUM_CHECKS();

		colorBuffer.blend = false;
		return *this;
	}
	
/*
=================================================
	SetColorMask
=================================================
*/
	Canvas::Material&  Canvas::Material::SetColorMask (bool4 colorMask)
	{
		colorBuffer.colorMask = colorMask;
		return *this;
	}
//-----------------------------------------------------------------------------


	
/*
=================================================
	constructor
=================================================
*/
	Canvas::Canvas (IResourceManager &rm) :
		_usedVBuffers{ 0 },		_resMngr{ rm }
	{
		_allocator.SetBlockSize( 4_Mb );
	}

/*
=================================================
	destructor
=================================================
*/
	Canvas::~Canvas ()
	{
		Clear();
	}
	
/*
=================================================
	Flush
=================================================
*/
	bool  Canvas::Flush (ITransferContext &ctx)
	{
		if ( not _changed )
			return true;

		for (uint i = 0; i < _usedVBuffers; ++i)
		{
			auto& buf = _buffers[i];

			CHECK_ERR( ctx.UploadBuffer( buf.id, 0_b, buf.vertSize, buf.vertices ));
			CHECK_ERR( ctx.UploadBuffer( buf.id, vertexBufferSize, buf.indexSize, buf.indices ));

			buf.vertSize	= 0_b;
			buf.indexSize	= 0_b;
			buf.vertices	= null;
			buf.indices		= null;
		}

		_allocator.Discard();

		_usedVBuffers	= 0;
		_changed		= false;

		return true;
	}

/*
=================================================
	Draw
=================================================
*/
	void  Canvas::Draw (IRenderContext &ctx)
	{
		const auto	ctx_info	= ctx.GetContextInfo();

		GraphicsPipelineDesc	desc;
		desc.renderPassId	= ctx_info.renderPass;
		desc.subpassIndex	= ctx_info.subpassIndex;
		desc.viewportCount	= ctx_info.layerCount;

		for (auto& batch : _batches)
		{
			auto&	buf = _buffers[ batch.bufIndex ];

			desc.renderState.color.buffers[0]		= batch.material.colorBuffer;
			desc.vertexInput						= *batch.vertexInput;
			desc.renderState.inputAssembly.topology	= batch.topology;

			auto	ppln = _resMngr.GetGraphicsPipeline( batch.material.pplnName, desc );
			if ( not ppln )
				continue;

			ctx.BindVertexBuffer( 0, buf.id, batch.vertexOffset );
			ctx.BindIndexBuffer( buf.id, batch.indexOffset + vertexBufferSize, EIndex::UShort );
			ctx.BindPipeline( ppln );

			if ( batch.material.descSet )
				ctx.BindDescriptorSet( 0, batch.material.descSet, batch.material.dynamicOffsets );

			ctx.DrawIndexed( batch.indexCount );
		}

		_batches.clear();
	}
	
/*
=================================================
	Clear
=================================================
*/
	void  Canvas::Clear ()
	{
		_usedVBuffers	= 0;
		_changed		= false;

		for (auto& buf : _buffers) {
			_resMngr.ReleaseResource( buf.id );
		}

		_buffers.clear();
		_batches.clear();
		_vertexInputMap.clear();
	}

/*
=================================================
	_AppendBatch
=================================================
*/
	Canvas::Batch*  Canvas::_AppendBatch (const Material &mtr, VertexInputPtr attribs, EPrimitive topology,
										  BytesU vertStride, BytesU vertBufSize, BytesU idxBufSize)
	{
		Batch*	dst	= null;	

		// try to reuse last batch
		if ( _batches.size() )
		{
			auto&	last	= _batches.back();
			auto&	buf		= _buffers[ last.bufIndex ];

			if ( last.material		== mtr			and
				 last.vertexInput	== attribs		and
				 last.topology		== topology		and
				 AlignToLarger( buf.vertCapacity  - buf.vertSize,  vertStride )  >= vertBufSize	and
				 AlignToLarger( buf.indexCapacity - buf.indexSize, indexStride ) >= idxBufSize )
			{
				dst = &last;
			}
		}

		// create new batch
		if ( not dst )
		{
			dst = &_batches.emplace_back( mtr );

			dst->vertexInput	= attribs;
			dst->topology		= topology;

			bool	create_buf = true;

			if ( _usedVBuffers )
			{
				auto&	buf = _buffers[_usedVBuffers - 1];

				create_buf = AlignToLarger( buf.vertCapacity  - buf.vertSize,  vertStride )  < vertBufSize and
							 AlignToLarger( buf.indexCapacity - buf.indexSize, indexStride ) < idxBufSize;
			}

			if ( create_buf )
			{
				// create vertex & index buffer
				if ( ++_usedVBuffers >= _buffers.size() )
				{
					if ( _buffers.size() == _buffers.capacity() )
					{
						ASSERT(!"overflow!");
						return null;
					}

					auto&	buf = _buffers.emplace_back();

					BufferDesc	desc;
					desc.size		= bufferSize;
					desc.usage		= EBufferUsage::Vertex | EBufferUsage::Index | EBufferUsage::TransferDst;
					desc.memType	= EMemoryType::DeviceLocal;

					buf.id = _resMngr.CreateBuffer( desc );
					CHECK_ERR( buf.id );

					buf.indexCapacity	= indexBufferSize;
					buf.vertCapacity	= vertexBufferSize;
				}

				// allocate memory
				{
					auto&	buf = _buffers[ _usedVBuffers - 1 ];

					buf.vertices	= _allocator.Alloc( buf.vertCapacity, 16_b );
					buf.indices		= _allocator.Alloc( buf.indexCapacity, 16_b );
				}
			}
			
			dst->bufIndex		= uint16_t(_usedVBuffers-1);
			dst->vertexOffset	= _buffers[ dst->bufIndex ].vertSize;
			dst->indexOffset	= _buffers[ dst->bufIndex ].indexSize;
		}

		return dst;
	}
	
/*
=================================================
	DrawText
=================================================
*/
	void  Canvas::DrawText (const VectorFont::PrecalculatedText &text, const FontPtr &font, const RectF &regionInPx, RGBA8u colorMask,
							INOUT Material &mtr, INOUT DescriptorSet &descSet, const UniformName &texName)
	{
		using Chunk		= FormattedText::Chunk;
		using GlyphKey	= VectorFont::GlyphKey;
		using GlyphPtr	= Ptr< const VectorFont::Glyph >;
		using Vertex_t	= SdfVertex2D;
		
		const BytesU	stride		= AlignToLarger( SizeOf<Vertex_t>, vertexStride );
		VertexInputPtr	attribs		= _AddAttribs<Vertex_t>( stride );
		BytesU			ib_size		= SizeOf<BatchIndex_t> * 6;
		BytesU			vb_size		= stride * 4;
		Batch*			batch		= null;
		uint			line_idx	= 1;
		float2			line		= float2{ regionInPx.left, regionInPx.top + text.LineHeights().front() };	// pixels
		char32_t		prev_c		= '\0';
		const float		region_w	= regionInPx.Width();	// pixels
		const float2	px_to_vp	= GetPixelsToViewport();
		GfxResourceID	curr_image;
		
		_changed = true;

		for (auto* chunk = text.Text().GetFirst(); chunk; chunk = chunk->next)
		{
			const uint		font_height	= VectorFont::ValidateFontHeight( chunk->height );
			const float		font_scale	= float(chunk->height) / font_height;
			const RGBA8u	color		= RGBA8u{ RGBA32f(chunk->color) * RGBA32f(colorMask) };

			for (size_t pos = 0; pos < chunk->length;)
			{
				const char32_t	c = Utf8Decode( chunk->string, chunk->length, INOUT pos );
				GlyphPtr		glyph;
				
				// windows style "\r\n" or linux style "\n"
				if ( ((prev_c == '\r') & (c == '\n')) | (c == '\n') )
				{
					line.x  = regionInPx.left;
					line.y += text.LineHeights()[ line_idx++ ];
					prev_c  = c;
					continue;
				}

				prev_c = c;

				if ( not font->LoadGlyph( GlyphKey{ c, font_height }, OUT glyph ))
					continue;
				
				const float  width = (glyph->offsetX[0] + glyph->size.x + glyph->offsetX[1]) * font_scale;	// pixels

				if ( text.IsWordWrap() & (line.x + width > region_w) )
				{
					line.x  = regionInPx.left;
					line.y += text.LineHeights()[ line_idx++ ];
				}
				
				// in viewport space
				const float  pos_x1 = (line.x + glyph->offsetX[0] * font_scale) * px_to_vp.x - 1.0f;
				const float  pos_x2 = (line.x + (glyph->offsetX[0] + glyph->size.x) * font_scale) * px_to_vp.x - 1.0f;
				const float  pos_y1 = (line.y - glyph->offsetY * font_scale) * px_to_vp.y - 1.0f;
				const float  pos_y2 = (line.y + (glyph->size.y - glyph->offsetY) * font_scale) * px_to_vp.y - 1.0f;

				line.x += width;

				if ( not glyph->imageId )
					continue;

				// begin new batch
				if ( curr_image != glyph->imageId or batch == null )
				{
					curr_image = glyph->imageId;

					descSet.BindTexture( texName, glyph->imageId, SamplerName{"LinearClamp"} );

					mtr.descSet			= _resMngr.CreateDescriptorSet( descSet );
					mtr.dynamicOffsets	= descSet.GetDynamicOffsets();

					batch = _AppendBatch( mtr, attribs, EPrimitive::TriangleList, stride, vb_size, ib_size );
					if ( not batch )
						return;
				}

				auto&	buf		 = _buffers[ batch->bufIndex ];
				auto*	indices	 = Cast<BatchIndex_t>(buf.indices + buf.indexSize);
				void*	vertices = buf.vertices + buf.vertSize;

				*Cast<Vertex_t>( vertices + stride*0 ) = Vertex_t{ float2(pos_x1, pos_y1), float2(glyph->texcoord.left,  glyph->texcoord.top),    color };
				*Cast<Vertex_t>( vertices + stride*1 ) = Vertex_t{ float2(pos_x1, pos_y2), float2(glyph->texcoord.left,  glyph->texcoord.bottom), color };
				*Cast<Vertex_t>( vertices + stride*2 ) = Vertex_t{ float2(pos_x2, pos_y1), float2(glyph->texcoord.right, glyph->texcoord.top),    color };
				*Cast<Vertex_t>( vertices + stride*3 ) = Vertex_t{ float2(pos_x2, pos_y2), float2(glyph->texcoord.right, glyph->texcoord.bottom), color };

				indices[0] = BatchIndex_t(batch->vertexCount + 0);
				indices[1] = BatchIndex_t(batch->vertexCount + 1);
				indices[2] = BatchIndex_t(batch->vertexCount + 2);
				indices[3] = BatchIndex_t(batch->vertexCount + 1);
				indices[4] = BatchIndex_t(batch->vertexCount + 2);
				indices[5] = BatchIndex_t(batch->vertexCount + 3);

				buf.vertSize	+= vb_size;
				buf.indexSize	+= ib_size;

				batch->vertexCount += 4;
				batch->indexCount  += 6;

				if ( buf.vertSize >= buf.vertCapacity or buf.indexSize >= buf.indexCapacity )
					batch = null;
			}
		}
	}
	

}	// AE::Graphics
