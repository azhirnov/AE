// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Math/Bytes.h"

namespace AE::STL
{

	//
	// Untyped Storage
	//

	template <usize Size, usize Align>
	struct UntypedStorage final
	{
	// types
	public:
		using Self = UntypedStorage< Size, Align >;


	// variables
	private:
		alignas(Align) char	_buffer [Size];


	// methods
	public:
		UntypedStorage ()
		{
			DEBUG_ONLY( memset( _buffer, 0, sizeof(_buffer) ));
		}

		template <typename T>
		ND_ T*  Cast (Bytes offset = 0_b)
		{
			ASSERT( SizeOf<T> + offset <= sizeof(_buffer) );
			return BitCast<T *>( _buffer + offset );
		}

		template <typename T>
		ND_ T const*  Cast (Bytes offset = 0_b) const
		{
			ASSERT( SizeOf<T> + offset <= sizeof(_buffer) );
			return BitCast<T const*>( _buffer + offset );
		}
	};


}	// AE::STL
