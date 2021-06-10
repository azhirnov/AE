// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

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

}	// AE::Graphics


namespace std
{
	size_t  hash< AE::Graphics::BufferViewDesc >::operator () (const AE::Graphics::BufferViewDesc &value) const
	{
		return size_t(AE::STL::HashOf( value.format ) + AE::STL::HashOf( value.offset ) + AE::STL::HashOf( value.size ));
	}

}	// std
