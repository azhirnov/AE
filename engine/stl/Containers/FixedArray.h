// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Containers/ArrayView.h"
#include "stl/Memory/MemUtils.h"
#include "stl/Math/Vec.h"

namespace AE::STL
{

	//
	// Fixed Size Array
	//

	template <typename T, usize ArraySize>
	struct FixedArray
	{
		STATIC_ASSERT( ArraySize < 256 );

	// types
	public:
		using iterator			= T *;
		using const_iterator	= const T *;
		using Self				= FixedArray< T, ArraySize >;

	private:
		using Count_t			= ubyte;


	// variables
	private:
		Count_t			_count	= 0;
		union {
			T			_array[ ArraySize ];
			char		_data[ ArraySize * sizeof(T) ];		// don't use this field!
		};


	// methods
	public:
		constexpr FixedArray ()
		{
			DEBUG_ONLY( memset( data(), 0, sizeof(T) * capacity() ));
			
			STATIC_ASSERT( alignof(Self) % alignof(T) == 0 );
		}

		constexpr FixedArray (std::initializer_list<T> list) : FixedArray()
		{
			ASSERT( list.size() <= capacity() );
			assign( list.begin(), list.end() );
		}

		constexpr FixedArray (ArrayView<T> view) : FixedArray()
		{
			ASSERT( view.size() <= capacity() );
			assign( view.begin(), view.end() );
		}

		constexpr FixedArray (const Self &other) : FixedArray()
		{
			assign( other.begin(), other.end() );
		}

		constexpr FixedArray (Self &&other) : _count{other._count}
		{
			ASSERT( not _IsMemoryAliased( other.begin(), other.end() ));

			for (usize i = 0, cnt = _count; i < cnt; ++i) {
				PlacementNew<T>( data() + i, RVRef(other._array[i]) );
			}
			other.clear();
		}

		~FixedArray ()
		{
			clear();
		}


		ND_ constexpr operator ArrayView<T> ()					const	{ return ArrayView<T>{ data(), size() }; }
		ND_ constexpr ArrayView<T>		ToArrayView()			const	{ return *this; }

		ND_ constexpr usize				size ()					const	{ return _count; }
		ND_ constexpr bool				empty ()				const	{ return _count == 0; }
		ND_ constexpr T *				data ()							{ return std::addressof( _array[0] ); }
		ND_ constexpr T const *			data ()					const	{ return std::addressof( _array[0] ); }

		ND_ constexpr T &				operator [] (usize i)			{ ASSERT( i < _count );  return _array[i]; }
		ND_ constexpr T const &			operator [] (usize i)	const	{ ASSERT( i < _count );  return _array[i]; }

		ND_ constexpr iterator			begin ()						{ return data(); }
		ND_ constexpr const_iterator	begin ()				const	{ return data(); }
		ND_ constexpr iterator			end ()							{ return data() + _count; }
		ND_ constexpr const_iterator	end ()					const	{ return data() + _count; }

		ND_ constexpr T &				front ()						{ ASSERT( _count > 0 );  return _array[0]; }
		ND_ constexpr T const&			front ()				const	{ ASSERT( _count > 0 );  return _array[0]; }
		ND_ constexpr T &				back ()							{ ASSERT( _count > 0 );  return _array[_count-1]; }
		ND_ constexpr T const&			back ()					const	{ ASSERT( _count > 0 );  return _array[_count-1]; }
		
		ND_ static constexpr usize		capacity ()						{ return ArraySize; }
		
		ND_ constexpr bool  operator == (ArrayView<T> rhs)		const	{ return ArrayView<T>{*this} == rhs; }
		ND_ constexpr bool  operator != (ArrayView<T> rhs)		const	{ return ArrayView<T>{*this} != rhs; }
		ND_ constexpr bool  operator >  (ArrayView<T> rhs)		const	{ return ArrayView<T>{*this} >  rhs; }
		ND_ constexpr bool  operator <  (ArrayView<T> rhs)		const	{ return ArrayView<T>{*this} <  rhs; }
		ND_ constexpr bool  operator >= (ArrayView<T> rhs)		const	{ return ArrayView<T>{*this} >= rhs; }
		ND_ constexpr bool  operator <= (ArrayView<T> rhs)		const	{ return ArrayView<T>{*this} <= rhs; }


		constexpr FixedArray&  operator = (const Self &rhs)
		{
			assign( rhs.begin(), rhs.end() );
			return *this;
		}
		
		constexpr FixedArray&  operator = (ArrayView<T> rhs)
		{
			ASSERT( rhs.size() < capacity() );
			assign( rhs.begin(), rhs.end() );
			return *this;
		}

		constexpr FixedArray&  operator = (Self &&rhs)
		{
			ASSERT( not _IsMemoryAliased( rhs.begin(), rhs.end() ));

			clear();
			_count = rhs._count;

			for (usize i = 0, cnt = _count; i < cnt; ++i) {
				PlacementNew<T>( data() + i, RVRef(rhs._array[i]) );
			}
			rhs.clear();
			return *this;
		}


		constexpr void  assign (const_iterator beginIter, const_iterator endIter)
		{
			ASSERT( beginIter <= endIter );
			ASSERT( not _IsMemoryAliased( beginIter, endIter ));

			clear();

			for (auto iter = beginIter; _count < capacity() and iter != endIter; ++iter, ++_count)
			{
				PlacementNew<T>( data() + _count, *iter );
			}
		}


		constexpr void  append (ArrayView<T> items)
		{
			for (auto& item : items) {
				push_back( item );
			}
		}


		constexpr void  push_back (const T &value)
		{
			ASSERT( _count < capacity() );
			PlacementNew<T>( data() + (_count++), value );
		}

		constexpr void  push_back (T &&value)
		{
			ASSERT( _count < capacity() );
			PlacementNew<T>( data() + (_count++), RVRef(value) );
		}


		template <typename ...Args>
		constexpr T&  emplace_back (Args&& ...args)
		{
			ASSERT( _count < capacity() );
			T* ptr = data() + (_count++);
			PlacementNew<T>( ptr, FwdArg<Args &&>( args )... );
			return *ptr;
		}


		constexpr void  pop_back ()
		{
			ASSERT( _count > 0 );
			--_count;

			_array[_count].~T();
			DEBUG_ONLY( memset( data() + _count, 0, sizeof(T) ));
		}


		constexpr bool  try_push_back (const T &value)
		{
			if ( _count < capacity() )
			{
				PlacementNew<T>( data() + (_count++), value );
				return true;
			}
			return false;
		}
		
		constexpr bool  try_push_back (T&& value)
		{
			if ( _count < capacity() )
			{
				PlacementNew<T>( data() + (_count++), RVRef(value) );
				return true;
			}
			return false;
		}
		
		template <typename ...Args>
		constexpr bool  try_emplace_back (Args&& ...args)
		{
			if ( _count < capacity() )
			{
				PlacementNew<T>( data() + (_count++), FwdArg<Args &&>( args )... );
				return true;
			}
			return false;
		}


		constexpr void  insert (usize pos, T &&value)
		{
			ASSERT( _count < capacity() );

			if ( pos >= _count )
				return push_back( RVRef(value) );

			for (usize i = _count++; i > pos; --i) {
				PlacementNew<T>( data() + i, RVRef(data()[i-1]) );
			}

			PlacementNew<T>( data() + pos, RVRef(value) );
		}


		constexpr void  resize (usize newSize)
		{
			newSize = Min( newSize, capacity() );

			if ( newSize < _count )
			{
				// delete elements
				for (usize i = newSize; i < _count; ++i)
				{
					_array[i].~T();
				}
				DEBUG_ONLY( memset( data() + newSize, 0, sizeof(T) * (_count - newSize) ));
			}

			if ( newSize > _count )
			{
				// create elements
				for (usize i = _count; i < newSize; ++i)
				{
					PlacementNew<T>( data() + i );
				}
			}

			_count = Count_t(newSize);
		}


		constexpr void  clear ()
		{
			for (usize i = 0; i < _count; ++i)
			{
				_array[i].~T();
			}

			_count = 0;

			DEBUG_ONLY( memset( data(), 0, sizeof(T) * capacity() ));
		}


		constexpr void  fast_erase (usize index)
		{
			ASSERT( index < _count );

			--_count;

			if ( index == _count )
			{
				// erase from back
				_array[_count].~T();
			}
			else
			{
				// move element from back to 'index'
				_array[index].~T();
				PlacementNew<T>( data() + index, RVRef(_array[_count]) );
			}

			DEBUG_ONLY( memset( data() + _count, 0, sizeof(T) ));
		}


	private:
		ND_ forceinline bool  _IsMemoryAliased (const_iterator beginIter, const_iterator endIter) const
		{
			return IsIntersects( begin(), end(), beginIter, endIter );
		}
	};


}	// AE::STL


namespace std
{
	template <typename T, size_t ArraySize>
	struct hash< AE::STL::FixedArray<T, ArraySize> >
	{
		ND_ size_t  operator () (const AE::STL::FixedArray<T, ArraySize> &value) const
		{
			return size_t(AE::STL::HashOf( AE::STL::ArrayView<T>{ value }));
		}
	};

}	// std
