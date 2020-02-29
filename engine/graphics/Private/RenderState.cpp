// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "graphics/Public/RenderState.h"

namespace AE::Graphics
{
	
/*
=================================================
	ColorBuffer::operator ==
=================================================
*/
	bool  RenderState::ColorBuffer::operator == (const ColorBuffer &rhs) const
	{
		return	blend == rhs.blend	and
				(blend ?
					(srcBlendFactor	== rhs.srcBlendFactor	and
					 dstBlendFactor	== rhs.dstBlendFactor	and
					 blendOp		== rhs.blendOp			and
					 All( colorMask	== rhs.colorMask )) : true);
	}

/*
=================================================
	ColorBuffer::CalcHash
=================================================
*/
	HashVal  RenderState::ColorBuffer::CalcHash () const
	{
	#if AE_FAST_HASH
		return size_t(HashOf( AddressOf(*this), sizeof(*this) ));
	#else
		HashVal	result;
		result << HashOf( blend );
		result << HashOf( srcBlendFactor.color );
		result << HashOf( srcBlendFactor.alpha );
		result << HashOf( dstBlendFactor.color );
		result << HashOf( dstBlendFactor.alpha );
		result << HashOf( blendOp.color );
		result << HashOf( blendOp.alpha );
		result << HashOf( colorMask );
		return result;
	#endif
	}
//-----------------------------------------------------------------------------


/*
=================================================
	ColorBuffersState::operator ==
=================================================
*/
	bool  RenderState::ColorBuffersState::operator == (const ColorBuffersState &rhs) const
	{
		return	buffers		== rhs.buffers	and
				logicOp		== rhs.logicOp	and
				blendColor	== rhs.blendColor;
	}

/*
=================================================
	ColorBuffersState::CalcHash
=================================================
*/
	HashVal  RenderState::ColorBuffersState::CalcHash () const
	{
	#if AE_FAST_HASH
		return size_t(HashOf( AddressOf(*this), sizeof(*this) ));
	#else
		HashVal	result;
		result << HashOf( blendColor );
		result << HashOf( logicOp );
		result << HashOf( buffers.size() );

		for (auto& cb : buffers) {
			result << cb.CalcHash();
		}
		return result;
	#endif
	}
//-----------------------------------------------------------------------------

	
/*
=================================================
	StencilFaceState::operator ==
=================================================
*/
	bool  RenderState::StencilFaceState::operator == (const StencilFaceState &rhs) const
	{
		return	failOp		== rhs.failOp		and
				depthFailOp	== rhs.depthFailOp	and
				passOp		== rhs.passOp		and
				compareOp	== rhs.compareOp	and
				reference	== rhs.reference	and
				writeMask	== rhs.writeMask	and
				compareMask	== rhs.compareMask;
	}

/*
=================================================
	StencilFaceState::CalcHash
=================================================
*/
	HashVal  RenderState::StencilFaceState::CalcHash () const
	{
	#if AE_FAST_HASH
		return size_t(HashOf( AddressOf(*this), sizeof(*this) ));
	#else
		HashVal	result;
		result << HashOf( failOp );
		result << HashOf( depthFailOp );
		result << HashOf( passOp );
		result << HashOf( compareOp );
		result << HashOf( reference );
		result << HashOf( writeMask );
		result << HashOf( compareMask );
		return result;
	#endif
	}
//-----------------------------------------------------------------------------

	
/*
=================================================
	StencilBufferState::operator ==
=================================================
*/
	bool  RenderState::StencilBufferState::operator == (const StencilBufferState &rhs) const
	{
		return	enabled		== rhs.enabled	and
				(enabled ? 
					(front	== rhs.front	and
					 back	== rhs.back)	: true);
	}
	
/*
=================================================
	StencilBufferState::CalcHash
=================================================
*/
	HashVal  RenderState::StencilBufferState::CalcHash () const
	{
		HashVal	result;
		if ( enabled )
		{
		#if AE_FAST_HASH
			return size_t(HashOf( AddressOf(*this), sizeof(*this) ));
		#else
			result << front.CalcHash();
			result << back.CalcHash();
		#endif
		}
		return result;
	}
//-----------------------------------------------------------------------------
	

/*
=================================================
	DepthBufferState::operator ==
=================================================
*/
	bool  RenderState::DepthBufferState::operator == (const DepthBufferState &rhs) const
	{
		return	compareOp		== rhs.compareOp		and
				boundsEnabled	== rhs.boundsEnabled	and
				(boundsEnabled ? All(Equals( bounds, rhs.bounds )) : true)	and
				write			== rhs.write			and
				test			== rhs.test;
	}

/*
=================================================
	DepthBufferState::CalcHash
=================================================
*/
	HashVal  RenderState::DepthBufferState::CalcHash () const
	{
	#if AE_FAST_HASH
		return size_t(HashOf( AddressOf(*this), sizeof(*this) ));
	#else
		HashVal	result;
		result << HashOf( compareOp );
		result << HashOf( boundsEnabled );
		result << HashOf( bounds );
		result << HashOf( test );
		result << HashOf( write );
		return result;
	#endif
	}
//-----------------------------------------------------------------------------
	

/*
=================================================
	InputAssemblyState::operator ==
=================================================
*/
	bool  RenderState::InputAssemblyState::operator == (const InputAssemblyState &rhs) const
	{
		return	topology			== rhs.topology		and
				primitiveRestart	== rhs.primitiveRestart;
	}
	
/*
=================================================
	InputAssemblyState::CalcHash
=================================================
*/
	HashVal  RenderState::InputAssemblyState::CalcHash () const
	{
	#if AE_FAST_HASH
		return size_t(HashOf( AddressOf(*this), sizeof(*this) ));
	#else
		HashVal	result;
		result << HashOf( topology );
		result << HashOf( primitiveRestart );
		return result;
	#endif
	}
//-----------------------------------------------------------------------------
	

/*
=================================================
	RasterizationState::operator ==
=================================================
*/
	bool  RenderState::RasterizationState::operator == (const RasterizationState &rhs) const
	{
		return	polygonMode					==	rhs.polygonMode				and
				Equals(	lineWidth,				rhs.lineWidth )				and
				Equals(	depthBiasConstFactor,	rhs.depthBiasConstFactor )	and
				Equals(	depthBiasClamp,			rhs.depthBiasClamp )		and
				Equals(	depthBiasSlopeFactor,	rhs.depthBiasSlopeFactor )	and
				depthBias					==	rhs.depthBias				and
				depthClamp					==	rhs.depthClamp				and
				rasterizerDiscard			==	rhs.rasterizerDiscard		and
				cullMode					==	rhs.cullMode				and
				frontFaceCCW				==	rhs.frontFaceCCW;
	}
	
/*
=================================================
	RasterizationState::CalcHash
=================================================
*/
	HashVal  RenderState::RasterizationState::CalcHash () const
	{
	#if AE_FAST_HASH
		return size_t(HashOf( AddressOf(*this), sizeof(*this) ));
	#else
		HashVal	result;
		result << HashOf( polygonMode );
		result << HashOf( lineWidth );
		result << HashOf( depthBiasConstFactor );
		result << HashOf( depthBiasClamp );
		result << HashOf( depthBiasSlopeFactor );
		result << HashOf( depthBias );
		result << HashOf( depthClamp );
		result << HashOf( rasterizerDiscard );
		result << HashOf( cullMode );
		result << HashOf( frontFaceCCW );
		return result;
	#endif
	}
//-----------------------------------------------------------------------------


/*
=================================================
	MultisampleState::operator ==
=================================================
*/
	bool  RenderState::MultisampleState::operator == (const MultisampleState &rhs) const
	{
		return	sampleMask				==	rhs.sampleMask			and
				samples					==	rhs.samples				and
				Equals(	minSampleShading,	rhs.minSampleShading )	and
				sampleShading			==	rhs.sampleShading		and
				alphaToCoverage			==	rhs.alphaToCoverage		and
				alphaToOne				==	rhs.alphaToOne;
	}
	
/*
=================================================
	MultisampleState::CalcHash
=================================================
*/
	HashVal  RenderState::MultisampleState::CalcHash () const
	{
	#if AE_FAST_HASH
		return size_t(HashOf( AddressOf(*this), sizeof(*this) ));
	#else
		HashVal	result;
		result << HashOf( sampleMask );
		result << HashOf( samples );
		result << HashOf( minSampleShading );
		result << HashOf( sampleShading );
		result << HashOf( alphaToCoverage );
		result << HashOf( alphaToOne );
		return result;
	#endif
	}
//-----------------------------------------------------------------------------


/*
=================================================
	operator ==
=================================================
*/
	bool  RenderState::operator == (const RenderState &rhs) const
	{
		return	color			== rhs.color			and
				depth			== rhs.depth			and
				stencil			== rhs.stencil			and
				inputAssembly	== rhs.inputAssembly	and
				rasterization	== rhs.rasterization	and
				multisample		== rhs.multisample;
	}

/*
=================================================
	CalcHash
=================================================
*/
	HashVal  RenderState::CalcHash () const
	{
	#if AE_FAST_HASH
		return size_t(HashOf( AddressOf(*this), sizeof(*this) ));
	#else
		HashVal	result;
		result << color.CalcHash();
		result << depth.CalcHash();
		result << stencil.CalcHash();
		result << inputAssembly.CalcHash();
		result << rasterization.CalcHash();
		result << multisample.CalcHash();
		return result;
	#endif
	}


}	// AE::Graphics
