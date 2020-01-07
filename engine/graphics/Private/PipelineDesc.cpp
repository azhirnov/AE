// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "graphics/Public/PipelineDesc.h"

namespace AE::Graphics
{
	
/*
=================================================
	GraphicsPipelineDesc::operator ==
=================================================
*/
	bool  GraphicsPipelineDesc::operator == (const GraphicsPipelineDesc &rhs) const
	{
		return	renderPassId	== rhs.renderPassId		and
				subpassIndex	== rhs.subpassIndex		and
				vertexInput		== rhs.vertexInput		and
				renderState		== rhs.renderState		and
				dynamicState	== rhs.dynamicState		and
				specialization	== rhs.specialization;
	}
	
/*
=================================================
	GraphicsPipelineDesc::CalcHash
=================================================
*/
	HashVal  GraphicsPipelineDesc::CalcHash () const
	{
		HashVal	result;
		result << HashOf( renderPassId );
		result << HashOf( subpassIndex );
		result << vertexInput.CalcHash();
		result << renderState.CalcHash();
		result << HashOf( dynamicState );
		result << HashOf( specialization );
		return result;
	}
//-----------------------------------------------------------------------------
	

/*
=================================================
	MeshPipelineDesc::operator ==
=================================================
*/
	bool  MeshPipelineDesc::operator == (const MeshPipelineDesc &rhs) const
	{
		return	renderPassId		== rhs.renderPassId		and
				subpassIndex		== rhs.subpassIndex		and
				renderState			== rhs.renderState		and
				dynamicState		== rhs.dynamicState		and
				All( taskGroupSize	== rhs.taskGroupSize )	and
				All( meshGroupSize	== rhs.meshGroupSize )	and
				specialization		== rhs.specialization;
	}
	
/*
=================================================
	MeshPipelineDesc::CalcHash
=================================================
*/
	HashVal  MeshPipelineDesc::CalcHash () const
	{
		HashVal	result;
		result << HashOf( renderPassId );
		result << HashOf( subpassIndex );
		result << renderState.CalcHash();
		result << HashOf( dynamicState );
		result << HashOf( taskGroupSize );
		result << HashOf( meshGroupSize );
		result << HashOf( specialization );
		return result;
	}
//-----------------------------------------------------------------------------
	

/*
=================================================
	ComputePipelineDesc::operator ==
=================================================
*/
	bool  ComputePipelineDesc::operator == (const ComputePipelineDesc &rhs) const
	{
		return	All( localGroupSize	== rhs.localGroupSize )	and
				specialization		== rhs.specialization;
	}
	
/*
=================================================
	ComputePipelineDesc::CalcHash
=================================================
*/
	HashVal  ComputePipelineDesc::CalcHash () const
	{
		HashVal	result;
		result << HashOf( localGroupSize );
		result << HashOf( specialization );
		return result;
	}
//-----------------------------------------------------------------------------
	

/*
=================================================
	RayTracingPipelineDesc::operator ==
=================================================
*/
	bool  RayTracingPipelineDesc::operator == (const RayTracingPipelineDesc &rhs) const
	{
		return	specialization == rhs.specialization;
	}
	
/*
=================================================
	RayTracingPipelineDesc::CalcHash
=================================================
*/
	HashVal  RayTracingPipelineDesc::CalcHash () const
	{
		HashVal	result;
		result << HashOf( specialization );
		return result;
	}


}	// AE::Graphics
