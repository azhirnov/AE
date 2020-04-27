// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Math/Bytes.h"

namespace AE::STL
{

/*
=================================================
	AddressOf
=================================================
*/
	template <typename T>
	ND_ forceinline decltype(auto)  AddressOf (T &value)
	{
		return std::addressof( value );
	}

/*
=================================================
	AddressDistance
=================================================
*/
	template <typename LT, typename RT>
	ND_ forceinline BytesU  AddressDistance (LT &lhs, RT &rhs)
	{
		return BytesU{ size_t(AddressOf(lhs)) - size_t(AddressOf(rhs)) };
	}
	
/*
=================================================
	PlacementNew
=================================================
*/
	template <typename T, typename ...Types>
	forceinline T *  PlacementNew (OUT void *ptr, Types&&... args)
	{
		ASSERT( CheckPointerAlignment<T>( ptr ));
		return ( new(ptr) T{ std::forward<Types>(args)... } );
	}
	
/*
=================================================
	PlacementDelete
=================================================
*/
	template <typename T>
	forceinline void  PlacementDelete (INOUT T &val)
	{
		val.~T();

		DEBUG_ONLY( std::memset( &val, 0xFE, sizeof(val) ));
	}
	
/*
=================================================
	Reconstruct
=================================================
*/
	template <typename T, typename ...Args>
	forceinline void  Reconstruct (INOUT T &value, Args&& ...args)
	{
		ASSERT( CheckPointerAlignment<T>( &value ));

		value.~T();
		DEBUG_ONLY( std::memset( &value, 0xFE, sizeof(value) ));

		new(&value) T{ std::forward<Args>(args)... };
	}

/*
=================================================
	MemCopy
=================================================
*/
	template <typename T1, typename T2>
	forceinline void  MemCopy (T1 &dst, const T2 &src)
	{
		STATIC_ASSERT( sizeof(dst) >= sizeof(src) );
		//STATIC_ASSERT( std::is_trivial_v<T1> and std::is_trivial_v<T2> );	// TODO
		STATIC_ASSERT( not IsConst<T1> );

		std::memcpy( &dst, &src, sizeof(src) );
	}

	forceinline void  MemCopy (void *dst, BytesU dstSize, const void *src, BytesU srcSize)
	{
		ASSERT( srcSize <= dstSize );
		ASSERT( dst and src );

		std::memcpy( dst, src, size_t(std::min(srcSize, dstSize)) );
	}


}	// AE::STL
