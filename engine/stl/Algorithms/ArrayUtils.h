// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Math/Bytes.h"
#include "stl/Containers/FixedArray.h"
#include "stl/Containers/NtStringView.h"

namespace AE::STL
{
	
/*
=================================================
	CountOf
=================================================
*/
	template <typename T>
	ND_ forceinline constexpr usize  CountOf (T& value)
	{
		return std::size( value );
	}
	
	template <typename ...Types>
	ND_ forceinline constexpr usize  CountOf ()
	{
		return sizeof... (Types);
	}
	
	template <typename T, usize I, typename Class>
	ND_ forceinline constexpr usize  CountOf (T (Class::*) [I])
	{
		return I;
	}
	
	template <usize I>
	ND_ forceinline constexpr usize  CountOf (const BitSet<I> &)
	{
		return I;
	}

/*
=================================================
	ArraySizeOf
=================================================
*/
	template <typename T, typename A>
	ND_ forceinline Bytes  ArraySizeOf (const std::vector<T,A> &arr)
	{
		return Bytes{ arr.size() * sizeof(T) };
	}

	template <typename T, usize S>
	ND_ forceinline Bytes  ArraySizeOf (const FixedArray<T,S> &arr)
	{
		return Bytes{ arr.size() * sizeof(T) };
	}

	template <typename T>
	ND_ forceinline Bytes  ArraySizeOf (const ArrayView<T> &arr)
	{
		return Bytes{ arr.size() * sizeof(T) };
	}
	
	template <typename T, usize S>
	ND_ forceinline constexpr Bytes  ArraySizeOf (const StaticArray<T,S> &)
	{
		return Bytes{ S * sizeof(T) };
	}

	template <typename T, usize I>
	ND_ forceinline constexpr Bytes  ArraySizeOf (const T (&)[I])
	{
		return Bytes{ sizeof(T) * I };
	}
	
/*
=================================================
	StringSizeOf
=================================================
*/
	template <typename T, typename A>
	ND_ forceinline Bytes  StringSizeOf (const std::basic_string<T,A> &str)
	{
		return Bytes{ str.size() * sizeof(T) };
	}

	template <typename T>
	ND_ forceinline Bytes  StringSizeOf (BasicStringView<T> str)
	{
		return Bytes{ str.size() * sizeof(T) };
	}

	template <typename T>
	ND_ forceinline Bytes  StringSizeOf (const NtBasicStringView<T> &str)
	{
		return Bytes{ str.size() * sizeof(T) };
	}

/*
=================================================
	Distance
=================================================
*/
	template <typename T>
	ND_ forceinline constexpr ptrdiff_t  Distance (T *lhs, T *rhs)
	{
		return std::distance< T *>( lhs, rhs );
	}
	
	template <typename T>
	ND_ forceinline constexpr ptrdiff_t  Distance (const T *lhs, T *rhs)
	{
		return std::distance< T const *>( lhs, rhs );
	}
	
	template <typename T>
	ND_ forceinline constexpr ptrdiff_t  Distance (T *lhs, const T *rhs)
	{
		return std::distance< T const *>( lhs, rhs );
	}
	
/*
=================================================
	LowerBound (binary search)
=================================================
*/
	template <typename T, typename Key>
	ND_ forceinline usize  LowerBound (ArrayView<T> arr, const Key &key)
	{
		usize	left	= 0;
		usize	right	= arr.size();

		for (; left < right; )
		{
			usize	mid = (left + right) >> 1;

			if ( key > arr[mid] )
				left = mid + 1;
			else
				right = mid;
		}

		return left < arr.size() and key == arr[left] ? left : UMax;
	}
	
	template <typename T, typename Key>
	ND_ forceinline usize  LowerBound (const Array<T> &arr, const Key &key)
	{
		return LowerBound( ArrayView<T>{arr}, key );
	}
	
/*
=================================================
	BinarySearch
=================================================
*/
	template <typename T, typename Key>
	ND_ forceinline usize  BinarySearch (ArrayView<T> arr, const Key &key)
	{
		ptrdiff_t	left	= 0;
		ptrdiff_t	right	= arr.size();

		for (; left < right; )
		{
			ptrdiff_t	mid = (left + right) >> 1;

			if ( key > arr[mid] )
				left = mid + 1;
			else
			if ( not (key == arr[mid]) )
				right = mid - 1;
			else
				return mid;
		}

		return left < ptrdiff_t(arr.size()) and key == arr[left] ? left : -1;
	}
	
	template <typename T, typename Key>
	ND_ forceinline usize  BinarySearch (const Array<T> &arr, const Key &key)
	{
		return BinarySearch( ArrayView<T>{arr}, key );
	}
	
/*
=================================================
	ExponentialSearch
=================================================
*/
	template <typename T, typename Key>
	ND_ forceinline usize  ExponentialSearch (ArrayView<T> arr, const Key &key)
	{
		if ( arr.empty() )
			return UMax;

		usize	left	= 0;
		usize	right	= arr.size();
		usize	bound	= 1;

		for (; bound < right and key > arr[bound]; bound *= 2)
		{}

		left  = bound >> 1;
		right = Min( bound+1, right );

		for (; left < right; )
		{
			usize	mid = (left + right) >> 1;

			if ( key > arr[mid] )
				left = mid + 1;
			else
				right = mid;
		}

		return left < arr.size() and key == arr[left] ? left : UMax;
	}
	
	template <typename T, typename Key>
	ND_ forceinline usize  ExponentialSearch (const Array<T> &arr, const Key &key)
	{
		return ExponentialSearch( ArrayView<T>{arr}, key );
	}
	
/*
=================================================
	Reverse
=================================================
*/
	namespace _hidden_
	{
		template <typename Container>
		class ReverseContainerView
		{
		private:
			Container &		_container;

		public:
			explicit ReverseContainerView (Container& container) : _container{container} {}

			ND_ auto  begin ()	{ return std::rbegin( _container ); }
			ND_ auto  end ()	{ return std::rend( _container ); }
		};

	}	// _hidden_

	template <typename Container>
	ND_ auto  Reverse (Container& container)
	{
		return STL::_hidden_::ReverseContainerView<Container>{ container };
	}

	template <typename Container>
	ND_ auto  Reverse (const Container& container)
	{
		return STL::_hidden_::ReverseContainerView<const Container>{ container };
	}

}	// AE::STL
