// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Common.h"
#include "stl/Containers/Ptr.h"

#ifdef AE_DEBUG
# include <sstream>
#endif

namespace AE::STL
{

/*
=================================================
	CheckPointerAlignment
=================================================
*/
	template <typename R, typename T>
	ND_ forceinline bool  CheckPointerAlignment (T const* ptr)
	{
		constexpr usize	align = alignof(R);

		STATIC_ASSERT( ((align & (align - 1)) == 0), "Align must be power of 2" );

		return (usize(ptr) & (align-1)) == 0;
	}
	
/*
=================================================
	CheckPointerCast
=================================================
*/
	template <typename R, typename T>
	inline void  CheckPointerCast (T const* ptr)
	{
		Unused( ptr );
		DEBUG_ONLY(
			if constexpr( not IsVoid<R> )
			{
				if ( not CheckPointerAlignment<R>( ptr ))
				{
					std::stringstream	str;
					str << "Failed to cast pointer from '" << typeid(T).name() << "' to '" << typeid(R).name()
						<< "': memory address " << std::hex << usize(ptr) << " is not aligned to " << std::dec << alignof(R)
						<< ", it may cause undefined behavior";
					AE_LOGE( str.str() );
				}
			}
		)
	}

/*
=================================================
	Cast
=================================================
*/
	template <typename R, typename T>
	ND_ forceinline constexpr R const volatile*  Cast (T const volatile* value)
	{
		STATIC_ASSERT( sizeof(R*) == sizeof(T*) );
		CheckPointerCast<R>( value );
		return static_cast< R const volatile *>( static_cast< void const volatile *>(value) );
	}

	template <typename R, typename T>
	ND_ forceinline constexpr R volatile*  Cast (T volatile* value)
	{
		STATIC_ASSERT( sizeof(R*) == sizeof(T*) );
		CheckPointerCast<R>( value );
		return static_cast< R volatile *>( static_cast< void volatile *>(value) );
	}

	template <typename R, typename T>
	ND_ forceinline constexpr R const*  Cast (T const* value)
	{
		STATIC_ASSERT( sizeof(R*) == sizeof(T*) );
		CheckPointerCast<R>( value );
		return static_cast< R const *>( static_cast< void const *>(value) );
	}
	
	template <typename R, typename T>
	ND_ forceinline constexpr R*  Cast (T* value)
	{
		STATIC_ASSERT( sizeof(R*) == sizeof(T*) );
		CheckPointerCast<R>( value );
		return static_cast< R *>( static_cast< void *>(value) );
	}

	template <typename R, typename T>
	ND_ forceinline constexpr Ptr<R const>  Cast (Ptr<T const> value)
	{
		return Cast<R>( value.get() );
	}
	
	template <typename R, typename T>
	ND_ forceinline constexpr Ptr<R>  Cast (Ptr<T> value)
	{
		return Cast<R>( value.get() );
	}
	
	template <typename R, typename T>
	ND_ forceinline constexpr R*  Cast (const UniquePtr<T> &value)
	{
		return Cast<R>( value.get() );
	}

	template <typename R, typename T>
	ND_ forceinline SharedPtr<R>  Cast (const SharedPtr<T> &other)
	{
		return std::static_pointer_cast<R>( other );
	}
	
/*
=================================================
	DynCast
=================================================
*/
	template <typename R, typename T>
	ND_ forceinline constexpr R const*  DynCast (T const* value)
	{
		return dynamic_cast< R const *>( value );
	}
	
	template <typename R, typename T>
	ND_ forceinline constexpr R*  DynCast (T* value)
	{
		return dynamic_cast< R *>( value );
	}

	template <typename R, typename T>
	ND_ forceinline constexpr Ptr<R const>  DynCast (Ptr<T const> value)
	{
		return DynCast<R>( value.operator->() );
	}
	
	template <typename R, typename T>
	ND_ forceinline constexpr Ptr<R>  DynCast (Ptr<T> value)
	{
		return DynCast<R>( value.operator->() );
	}

	template <typename R, typename T>
	ND_ forceinline SharedPtr<R>  DynCast (const SharedPtr<T> &other)
	{
		return std::dynamic_pointer_cast<R>( other );
	}

/*
=================================================
	BitCast
=================================================
*/
	template <typename To, typename From>
	ND_ inline constexpr To  BitCast (const From& src)
	{
		STATIC_ASSERT( sizeof(To) == sizeof(From), "must be same size!" );
		//STATIC_ASSERT( std::is_trivially_copyable<From>::value and std::is_trivial<To>::value, "must be trivial types!" ); // TODO
		//STATIC_ASSERT( not IsSameTypes< To, From >);	// to find unnecessary cast

		To	dst;
		std::memcpy( OUT &dst, &src, sizeof(To) );
		return dst;
	}

/*
=================================================
	CheckCast
=================================================
*/
	template <typename To, typename From>
	ND_ inline constexpr To  CheckCast (const From& src)
	{
		if constexpr( IsSigned<From> and IsUnsigned<To> )
		{
			ASSERT( src >= 0 );
		}

		ASSERT( static_cast<From>(static_cast<To>(src)) == src );

		return static_cast<To>(src);
	}


}	// AE::STL
