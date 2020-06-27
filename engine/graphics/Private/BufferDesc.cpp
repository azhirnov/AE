// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "graphics/Public/BufferDesc.h"

namespace AE::Graphics
{
	
/*
=================================================
	Validate
=================================================
*/
	void BufferViewDesc::Validate (const BufferDesc &desc)
	{
		ASSERT( offset < desc.size );
		ASSERT( format != Default );
	
		offset	= Min( offset, desc.size-1 );
		size	= Min( size, desc.size - offset );
	}
	
/*
=================================================
	operator ==
=================================================
*/
	bool  BufferViewDesc::operator == (const BufferViewDesc &rhs) const
	{
		return	(format	== rhs.format)	&
				(offset	== rhs.offset)	&
				(size	== rhs.size);
	}
//-----------------------------------------------------------------------------

	
/*
=================================================
	ToPhysical
=================================================
*/
	BufferDesc  VirtualBufferDesc::ToPhysical (EVirtualResourceUsage usage) const
	{
		BufferDesc	result;
		result.size		= size;
		result.usage	= Zero;
		
		for (uint t = 1; t <= uint(usage); t <<= 1)
		{
			if ( not AllBits( usage, EVirtualResourceUsage(t) ))
				continue;

			BEGIN_ENUM_CHECKS();
			switch( EVirtualResourceUsage(t) )
			{
				case EVirtualResourceUsage::TransferSrc :			result.usage |= EBufferUsage::TransferSrc;		break;
				case EVirtualResourceUsage::TransferDst :			result.usage |= EBufferUsage::TransferDst;		break;
				case EVirtualResourceUsage::Storage :				result.usage |= EBufferUsage::Storage;			break;
				case EVirtualResourceUsage::Uniform :				result.usage |= EBufferUsage::Uniform;			break;
				case EVirtualResourceUsage::UniformTexel :			result.usage |= EBufferUsage::UniformTexel;		break;
				case EVirtualResourceUsage::StorageTexel :			result.usage |= EBufferUsage::StorageTexel;		break;
				case EVirtualResourceUsage::IndexBuffer :			result.usage |= EBufferUsage::Index;			break;
				case EVirtualResourceUsage::VertexBuffer :			result.usage |= EBufferUsage::Vertex;			break;
				case EVirtualResourceUsage::IndirectBuffer :		result.usage |= EBufferUsage::Indirect;			break;
				case EVirtualResourceUsage::Sampled :
				case EVirtualResourceUsage::ColorAttachment :
				case EVirtualResourceUsage::DepthStencilAttachment :
				case EVirtualResourceUsage::ShadingRate :
				case EVirtualResourceUsage::FragmentDensityMap :
				case EVirtualResourceUsage::Present :
				case EVirtualResourceUsage::RayTracing :
				case EVirtualResourceUsage::ShaderDeviceAddress :
				case EVirtualResourceUsage::Unknown :
				default :
					ASSERT(!"unsupported virtual resource usage!");
					break;
			}
			END_ENUM_CHECKS();
		}

		return result;
	}

}	// AE::Graphics


namespace std
{
	size_t  hash< AE::Graphics::BufferViewDesc >::operator () (const AE::Graphics::BufferViewDesc &value) const
	{
		return size_t(AE::STL::HashOf( value.format ) + AE::STL::HashOf( value.offset ) + AE::STL::HashOf( value.size ));
	}

}	// std
