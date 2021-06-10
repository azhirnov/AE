// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Math/Bytes.h"
#include "stl/Math/Math.h"
#include "stl/Memory/MemUtils.h"

namespace AE::STL
{

	//
	// Memory Writer
	//

	struct MemWriter
	{
	// variables
	private:
		void *		_ptr	= null;
		usize		_offset	= 0;
		Bytes		_size;


	// methods
	public:
		MemWriter () {}
		MemWriter (void *ptr, Bytes size) : _ptr{ptr}, _size{size} {}


		ND_ void*  Reserve (Bytes size, Bytes align)
		{
			ASSERT( _ptr );
			usize	result = AlignToLarger( usize(_ptr) + _offset, usize(align) );

			_offset = (result - usize(_ptr)) + usize(size);
			ASSERT( _offset <= _size );

			return BitCast<void *>( result );
		}


		template <typename T>
		ND_ T&  Reserve ()
		{
			return *Cast<T>( Reserve( SizeOf<T>, AlignOf<T> ));
		}

		template <typename T, typename ...Args>
		ND_ T&  Emplace (Args&& ...args)
		{
			return *PlacementNew<T>( &Reserve<T>(), FwdArg<Args>( args )... );
		}

		template <typename T, typename ...Args>
		ND_ T&  EmplaceSized (Bytes size, Args&& ...args)
		{
			ASSERT( size >= SizeOf<T> );
			return *PlacementNew<T>( Reserve( size, AlignOf<T> ), FwdArg<Args>( args )... );
		}


		template <typename T>
		ND_ T*  ReserveArray (usize count)
		{
			return count ? Cast<T>( Reserve( SizeOf<T> * count, AlignOf<T> )) : null;
		}

		template <typename T, typename ...Args>
		ND_ T*  EmplaceArray (usize count, Args&& ...args)
		{
			T*	result = ReserveArray<T>( count );

			for (usize i = 0; i < count; ++i) {
				PlacementNew<T>( result + i, FwdArg<Args>( args )... );
			}
			return result;
		}


		void Clear ()
		{
			ASSERT( _ptr );
			memset( _ptr, 0, usize(_size) );
		}


		ND_ Bytes  OffsetOf (void *ptr, Bytes defaultValue = ~0_b) const
		{
			if ( ptr ) {
				ASSERT( ptr >= _ptr and ptr < _ptr + _size );
				return Bytes{usize(ptr) - usize(_ptr)};
			}
			return defaultValue;
		}
	};


}	// AE::STL
