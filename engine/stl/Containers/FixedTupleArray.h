// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Containers/ArrayView.h"
#include "stl/CompileTime/TypeList.h"
#include "stl/Containers/InPlace.h"

namespace AE::STL
{

	//
	// Fixed Size Tuple Array
	//

	template <usize ArraySize, typename ...Types>
	struct FixedTupleArray
	{
		STATIC_ASSERT( ArraySize < 256 );

	// types
	private:
		template <typename T>
		using ElemArray	= InPlace< T [ArraySize] >;

		using Array_t	= Tuple< ElemArray<Types>... >;
		using Self		= FixedTupleArray< ArraySize, Types... >;
		using Types_t	= TypeList< Types... >;


	// variables
	private:
		usize		_count	= 0;
		Array_t		_arrays;


	// methods
	public:
		constexpr FixedTupleArray ()
		{}

		~FixedTupleArray ()
		{
			clear();
		}

		
		template <usize I>
		ND_ constexpr auto			get ()			const	{ return ArrayView<typename Types_t::template Get<I>>{ _Data<I>(), _count }; }

		template <typename T>
		ND_ constexpr ArrayView<T>	get ()			const	{ return get< Types_t::template Index<T> >(); }

		template <usize I>
		ND_ decltype(auto)			at (usize i)			{ ASSERT( i < _count ); return _Data<I>()[i]; }
		
		template <usize I>
		ND_ decltype(auto)			at (usize i)	const	{ ASSERT( i < _count ); return _Data<I>()[i]; }

		ND_ constexpr usize		size ()			const	{ return _count; }
		ND_ constexpr bool			empty ()		const	{ return _count == 0; }

		ND_ static constexpr usize	capacity ()				{ return ArraySize; }

		
		constexpr void  push_back (const Types&... values)
		{
			ASSERT( _count < capacity() );
			_PushBack<0>( values... );
			++_count;
		}

		constexpr void  push_back (Types&&... values)
		{
			ASSERT( _count < capacity() );
			_PushBack<0>( FwdArg<Types>(values)... );
			++_count;
		}
		

		constexpr bool  try_push_back (const Types&... values)
		{
			if ( _count < capacity() )
			{
				_PushBack<0>( values... );
				++_count;
				return true;
			}
			return false;
		}

		constexpr bool  try_push_back (Types&&... values)
		{
			if ( _count < capacity() )
			{
				_PushBack<0>( FwdArg<Types>(values)... );
				++_count;
				return true;
			}
			return false;
		}


		constexpr void  emplace_back ()
		{
			ASSERT( _count < capacity() );
			_Create<0>( _count );
			++_count;
		}

		constexpr void  pop_back ()
		{
			ASSERT( _count > 0 );
			--_count;
			_Destroy<0>( _count );
		}
		
		constexpr void  insert (usize pos, const Types&... values)
		{
			ASSERT( _count < capacity() );
			if ( pos >= _count ) {
				_PushBack<0>( values... );
			}else{
				_Move<0>( pos, pos+1, _count - pos );
				_Insert<0>( pos, values... );
			}
			++_count;
		}

		constexpr void  insert (usize pos, Types&&... values)
		{
			ASSERT( _count < capacity() );
			if ( pos >= _count ) {
				_PushBack<0>( FwdArg<Types>(values)... );
			}else{
				_Move<0>( pos, pos+1, _count - pos );
				_Insert<0>( pos, FwdArg<Types>(values)... );
			}
			++_count;
		}

		constexpr void resize (usize newSize)
		{
			newSize = Min( newSize, capacity() );

			if ( newSize < _count )
			{
				// delete elements
				for (usize i = newSize; i < _count; ++i) {
					_Destroy<0>( i );
				}
			}

			if ( newSize > _count )
			{
				// create elements
				for (usize i = _count; i < newSize; ++i) {
					_Create<0>( i );
				}
			}

			_count = newSize;
		}

		constexpr void  erase (usize pos)
		{
			ASSERT( _count > 0 );
			--_count;
			_Destroy<0>( pos );

			if ( pos < _count )
			{
				_Move<0>( pos+1, pos, _count - pos );
				_Destroy<0>( _count );
			}
		}

		constexpr void  fast_erase (usize pos)
		{
			ASSERT( _count > 0 );
			--_count;
			_Destroy<0>( pos );

			if ( pos < _count )
				_Replace<0>( _count, pos );
		}

		constexpr void clear ()
		{
			for (usize i = 0; i < _count; ++i) {
				_Destroy<0>( i );
			}

			_count = 0;
		}


		ND_ bool  operator == (const Self &rhs) const
		{
			if ( this == &rhs )
				return true;

			if ( _count != rhs._count )
				return false;

			return _arrays == rhs._arrays;
		}


	private:
		template <usize I>		ND_ constexpr auto*	_Data () const	{ return &(*&_arrays.template Get<I>())[0]; }
		template <usize I>		ND_ constexpr auto*	_Data ()		{ return &(*&_arrays.template Get<I>())[0]; }


		template <usize I, typename Arg0, typename ...Args>
		constexpr void  _PushBack (Arg0 &&arg0, Args&&... args)
		{
			using T = std::remove_const_t< std::remove_reference_t< Arg0 >>;

			PlacementNew<T>( _Data<I>() + _count, FwdArg<Arg0>(arg0) );
			
			if constexpr( I+1 < Types_t::Count )
				_PushBack<I+1>( FwdArg<Args&&>(args)... );
		}
		

		template <usize I, typename Arg0, typename ...Args>
		constexpr void  _Insert (usize pos, Arg0 &&arg0, Args&&... args)
		{
			using T = std::remove_const_t< std::remove_reference_t< Arg0 >>;
			T* data = _Data<I>();
			
			data[pos].~T();
			PlacementNew<T>( data + pos, FwdArg<Arg0>(arg0) );
			
			if constexpr( I+1 < Types_t::Count )
				_Insert<I+1>( pos, FwdArg<Args&&>(args)... );
		}


		template <usize I>
		constexpr void  _Destroy (usize index)
		{
			using T = typename TypeList< Types... >::template Get<I>;
			T* data = _Data<I>();

			data[index].~T();
			DEBUG_ONLY( std::memset( data + index, 0xCD, sizeof(T) ));

			if constexpr( I+1 < Types_t::Count )
				_Destroy<I+1>( index );
		}
		

		template <usize I>
		constexpr void  _Move (usize srcIdx, usize dstIdx, usize count)
		{
			using T = typename Types_t::template Get<I>;
			T* data = _Data<I>();

			for (usize i = 0; i < count; ++i) {
				PlacementNew<T>( data + dstIdx + i, RVRef(data[srcIdx + i]) );
			}

			if constexpr( I+1 < Types_t::Count )
				_Move<I+1>( srcIdx, dstIdx, count );
		}


		template <usize I>
		constexpr void  _Replace (usize srcIdx, usize dstIdx)
		{
			using T = typename Types_t::template Get<I>;
			T* data = _Data<I>();

			PlacementNew<T>( data + dstIdx, RVRef(data[srcIdx]) );

			data[srcIdx].~T();
			DEBUG_ONLY( std::memset( data + srcIdx, 0xCD, sizeof(T) ));
			
			if constexpr( I+1 < Types_t::Count )
				_Replace<I+1>( srcIdx, dstIdx );
		}


		template <usize I>
		constexpr void  _Create (usize index)
		{
			using T = typename Types_t::template Get<I>;
			PlacementNew<T>( _Data<I>() + index );

			if constexpr( I+1 < Types_t::Count )
				_Create<I+1>( index );
		}
	};


}	// AE::STL
