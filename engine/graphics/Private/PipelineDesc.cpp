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
		return	(renderPassId	== rhs.renderPassId)	&
				(subpassIndex	== rhs.subpassIndex)	&
				(viewportCount	== rhs.viewportCount)	&
				(vertexInput	== rhs.vertexInput)		&
				(renderState	== rhs.renderState)		&
				(dynamicState	== rhs.dynamicState)	&
				(specialization	== rhs.specialization);
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
		result << HashOf( viewportCount );
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
		return	(renderPassId			== rhs.renderPassId)				&
				(subpassIndex			== rhs.subpassIndex)				&
				(viewportCount			== rhs.viewportCount)				&
				(renderState			== rhs.renderState)					&
				(dynamicState			== rhs.dynamicState)				&
				(hasTaskGroupSize		== rhs.hasTaskGroupSize)			&
				(hasTaskGroupSize ?
					All( taskGroupSize	== rhs.taskGroupSize ) : true)		&
				(hasMeshGroupSize		== rhs.hasMeshGroupSize)			&
				(hasMeshGroupSize ?
					All( meshGroupSize	== rhs.meshGroupSize ) : true)		&
				(specialization		== rhs.specialization);
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
		result << HashOf( viewportCount );
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
		return	(localGroupSize.has_value()	== rhs.localGroupSize.has_value())	&
				(localGroupSize.has_value() ?
					All( *localGroupSize	== *rhs.localGroupSize ) : true)	&
				(specialization				== rhs.specialization)				&
				(dispatchBase				== rhs.dispatchBase);
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
		result << HashOf( dispatchBase );
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
		return	(specialization == rhs.specialization);
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
