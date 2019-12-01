// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Containers/ArrayView.h"
#include "stl/CompileTime/TypeList.h"
#include "stl/Containers/InPlace.h"

namespace AE::STL
{

	//
	// Fixed Size Tuple Array
	//

	template <size_t ArraySize, typename ...Types>
	struct FixedTupleArray
	{
	// types
	private:
		template <typename T>
		using ElemArray	= InPlace< T [ArraySize] >;

		using Array_t	= Tuple< ElemArray<Types>... >;
		using Self		= FixedTupleArray< ArraySize, Types... >;
		using Types_t	= TypeList< Types... >;


	// variables
	private:
		Array_t		_arrays;
		size_t		_count	= 0;


	// methods
	public:
		constexpr FixedTupleArray ()
		{}

		~FixedTupleArray ()
		{
			clear();
		}

		
		template <size_t I>
		ND_ constexpr auto			get ()			const	{ return ArrayView<typename Types_t::template Get<I>>{ _Data<I>(), _count }; }

		template <typename T>
		ND_ constexpr ArrayView<T>	get ()			const	{ return get< Types_t::template Index<T> >(); }

		template <size_t I>
		ND_ decltype(auto)			at (size_t i)			{ ASSERT( i < _count ); return _Data<I>()[i]; }
		
		template <size_t I>
		ND_ decltype(auto)			at (size_t i)	const	{ ASSERT( i < _count ); return _Data<I>()[i]; }

		ND_ constexpr size_t		size ()			const	{ return _count; }
		ND_ constexpr bool			empty ()		const	{ return _count == 0; }

		ND_ static constexpr size_t	capacity ()				{ return ArraySize; }

		
		constexpr void  push_back (const Types&... values)
		{
			ASSERT( _count < capacity() );
			_PushBack<0>( values... );
			++_count;
		}

		constexpr void  push_back (Types&&... values)
		{
			ASSERT( _count < capacity() );
			_PushBack<0>( std::forward<Types>(values)... );
			++_count;
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
		
		constexpr void  insert (size_t pos, const Types&... values)
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

		constexpr void  insert (size_t pos, Types&&... values)
		{
			ASSERT( _count < capacity() );
			if ( pos >= _count ) {
				_PushBack<0>( std::forward<Types>(values)... );
			}else{
				_Move<0>( pos, pos+1, _count - pos );
				_Insert<0>( pos, std::forward<Types>(values)... );
			}
			++_count;
		}

		constexpr void resize (size_t newSize)
		{
			newSize = Min( newSize, capacity() );

			if ( newSize < _count )
			{
				// delete elements
				for (size_t i = newSize; i < _count; ++i) {
					_Destroy<0>( i );
				}
			}

			if ( newSize > _count )
			{
				// create elements
				for (size_t i = _count; i < newSize; ++i) {
					_Create<0>( i );
				}
			}

			_count = newSize;
		}

		constexpr void  erase (size_t pos)
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

		constexpr void  fast_erase (size_t pos)
		{
			ASSERT( _count > 0 );
			--_count;
			_Destroy<0>( pos );

			if ( pos < _count )
				_Replace<0>( _count, pos );
		}

		constexpr void clear ()
		{
			for (size_t i = 0; i < _count; ++i) {
				_Destroy<0>( i );
			}

			_count = 0;
		}


	private:
		template <size_t I>		ND_ constexpr auto*	_Data () const	{ return &(*&_arrays.Get<I>())[0]; }
		template <size_t I>		ND_ constexpr auto*	_Data ()		{ return &(*&_arrays.Get<I>())[0]; }


		template <size_t I, typename Arg0, typename ...Args>
		constexpr void  _PushBack (Arg0 &&arg0, Args&&... args)
		{
			using T = std::remove_const_t< std::remove_reference_t< Arg0 >>;

			PlacementNew<T>( _Data<I>() + _count, std::forward<Arg0>(arg0) );
			
			if constexpr( I+1 < Types_t::Count )
				_PushBack<I+1>( std::forward<Args&&>(args)... );
		}
		

		template <size_t I, typename Arg0, typename ...Args>
		constexpr void  _Insert (size_t pos, Arg0 &&arg0, Args&&... args)
		{
			using T = std::remove_const_t< std::remove_reference_t< Arg0 >>;
			T* data = _Data<I>();
			
			data[pos].~T();
			PlacementNew<T>( data + pos, std::forward<Arg0>(arg0) );
			
			if constexpr( I+1 < Types_t::Count )
				_Insert<I+1>( pos, std::forward<Args&&>(args)... );
		}


		template <size_t I>
		constexpr void  _Destroy (size_t index)
		{
			using T = typename TypeList< Types... >::template Get<I>;
			T* data = _Data<I>();

			data[index].~T();
			DEBUG_ONLY( std::memset( data + index, 0xCD, sizeof(T) ));

			if constexpr( I+1 < Types_t::Count )
				_Destroy<I+1>( index );
		}
		

		template <size_t I>
		constexpr void  _Move (size_t srcIdx, size_t dstIdx, size_t count)
		{
			using T = typename Types_t::template Get<I>;
			T* data = _Data<I>();

			for (size_t i = 0; i < count; ++i) {
				PlacementNew<T>( data + dstIdx + i, std::move(data[srcIdx + i]) );
			}

			if constexpr( I+1 < Types_t::Count )
				_Move<I+1>( srcIdx, dstIdx, count );
		}


		template <size_t I>
		constexpr void  _Replace (size_t srcIdx, size_t dstIdx)
		{
			using T = typename Types_t::template Get<I>;
			T* data = _Data<I>();

			PlacementNew<T>( data + dstIdx, std::move(data[srcIdx]) );

			data[srcIdx].~T();
			DEBUG_ONLY( std::memset( data + srcIdx, 0xCD, sizeof(T) ));
			
			if constexpr( I+1 < Types_t::Count )
				_Replace<I+1>( srcIdx, dstIdx );
		}


		template <size_t I>
		constexpr void  _Create (size_t index)
		{
			using T = typename Types_t::template Get<I>;
			PlacementNew<T>( _Data<I>() + index );

			if constexpr( I+1 < Types_t::Count )
				_Create<I+1>( index );
		}
	};


}	// AE::STL
