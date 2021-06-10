// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'
/*
	Map emulation on static array with linear search.
	Use only for small number of elements.

	Recomended maximum size is 8..16 elements, .
*/

#pragma once

#include "stl/Algorithms/Cast.h"
#include "stl/Containers/ArrayView.h"
#include "stl/Memory/MemUtils.h"
#include "stl/Math/Math.h"

namespace AE::STL
{

	//
	// Fixed Size Map
	//

	template <typename Key, typename Value, usize ArraySize>
	struct FixedMap
	{
		STATIC_ASSERT( ArraySize < 256 );

	// types
	private:
		using Self				= FixedMap< Key, Value, ArraySize >;
		using Index_t			= Conditional< (ArraySize < 0xFF), ubyte, Conditional< (ArraySize < 0xFFFF), ushort, uint >>;
		using Pair_t			= Pair< const Key, Value >;

	public:
		using iterator			= Pair_t *;
		using const_iterator	= Pair_t const *;
		using pair_type			= Pair< Key, Value >;
		using key_type			= Key;
		using value_type		= Value;


	// variables
	private:
		Index_t				_count		= 0;
		mutable Index_t		_indices [ArraySize];
		union {
			pair_type		_array  [ArraySize];
			char			_buffer [sizeof(pair_type) * ArraySize];	// don't use this field!
		};


	// methods
	public:
		FixedMap ();
		FixedMap (Self &&);
		FixedMap (const Self &);

		~FixedMap ()	{ clear(); }

		ND_ usize			size ()			const	{ return _count; }
		ND_ bool			empty ()		const	{ return _count == 0; }

		ND_ iterator		begin ()				{ return BitCast< iterator >(&_array[0]); }
		ND_ const_iterator	begin ()		const	{ return BitCast< const_iterator >(&_array[0]); }
		ND_ iterator		end ()					{ return begin() + _count; }
		ND_ const_iterator	end ()			const	{ return begin() + _count; }
		
		ND_ static constexpr usize	capacity ()		{ return ArraySize; }

		ND_ explicit operator ArrayView<pair_type> () const	{ return { &_array[0], _count }; }
		
			Self&	operator = (Self &&);
			Self&	operator = (const Self &);

		ND_ Pair_t const&	operator [] (usize i)	const;

		ND_ bool	operator == (const Self &rhs) const;
		ND_ bool	operator != (const Self &rhs) const		{ return not (*this == rhs); }

			Pair<iterator,bool>  insert (const pair_type &value);
			Pair<iterator,bool>  insert (pair_type &&value);

			template <typename M>
			Pair<iterator,bool>  insert_or_assign (const key_type& key, M&& obj);
			
			template <typename M>
			Pair<iterator,bool>  insert_or_assign (key_type&& key, M&& obj);

		ND_ const_iterator	find (const key_type &key) const;
		ND_ iterator		find (const key_type &key);

		ND_ usize			count (const key_type &key) const;

		ND_ HashVal			CalcHash () const;
		
			void			clear ();


	private:
		ND_ forceinline bool _IsMemoryAliased (const Self* other) const
		{
			return IsIntersects( this, this+1, other, other+1 );
		}
	};
	

	
/*
=================================================
	constructor
=================================================
*/
	template <typename K, typename V, usize S>
	inline FixedMap<K,V,S>::FixedMap ()
	{
		DEBUG_ONLY( memset( _indices, 0xCD, sizeof(_indices) ));
		DEBUG_ONLY( memset( _array,   0xCD, sizeof(_array)   ));
	}
	
/*
=================================================
	constructor
=================================================
*/
	template <typename K, typename V, usize S>
	inline FixedMap<K,V,S>::FixedMap (const Self &other) : _count{ other._count }
	{
		ASSERT( not _IsMemoryAliased( &other ));

		for (usize i = 0; i < _count; ++i)
		{
			PlacementNew< pair_type >( &_array[i], other._array[i] );
			_indices[i] = other._indices[i];
		}
	}
	
/*
=================================================
	constructor
=================================================
*/
	template <typename K, typename V, usize S>
	inline FixedMap<K,V,S>::FixedMap (Self &&other) : _count{ other._count }
	{
		ASSERT( not _IsMemoryAliased( &other ));

		for (usize i = 0; i < _count; ++i)
		{
			PlacementNew< pair_type >( &_array[i], RVRef(other._array[i]) );
			_indices[i] = other._indices[i];
		}
		other.clear();
	}
	
/*
=================================================
	operator =
=================================================
*/
	template <typename K, typename V, usize S>
	inline FixedMap<K,V,S>&  FixedMap<K,V,S>::operator = (Self &&rhs)
	{
		ASSERT( not _IsMemoryAliased( &rhs ));

		clear();
		_count = rhs._count;

		for (usize i = 0; i < _count; ++i)
		{
			PlacementNew< pair_type >( &_array[i], RVRef(rhs._array[i]) );
			_indices[i] = rhs._indices[i];
		}

		rhs.clear();
		return *this;
	}
	
/*
=================================================
	operator =
=================================================
*/
	template <typename K, typename V, usize S>
	inline FixedMap<K,V,S>&  FixedMap<K,V,S>::operator = (const Self &rhs)
	{
		ASSERT( not _IsMemoryAliased( &rhs ));

		clear();
		_count = rhs._count;

		for (usize i = 0; i < _count; ++i)
		{
			PlacementNew< pair_type >( &_array[i], rhs._array[i] );
			_indices[i] = rhs._indices[i];
		}

		return *this;
	}

/*
=================================================
	operator ==
=================================================
*/
	template <typename K, typename V, usize S>
	inline bool	 FixedMap<K,V,S>::operator == (const Self &rhs) const
	{
		if ( this == &rhs )
			return true;

		if ( _count != rhs._count )
			return false;

		for (usize i = 0; i < _count; ++i)
		{
			if ( not (_array[_indices[i]] == rhs._array[rhs._indices[i]]) )
				return false;
		}
		return true;
	}
	
/*
=================================================
	operator []
=================================================
*/
	template <typename K, typename V, usize S>
	inline typename FixedMap<K,V,S>::Pair_t const&  FixedMap<K,V,S>::operator [] (usize i) const
	{
		ASSERT( i < _count );
		return reinterpret_cast<Pair_t const&>( _array[ _indices[i]]);
	}

/*
=================================================
	RecursiveBinarySearch
=================================================
*/
namespace _hidden_
{
	template <typename K, typename V, typename I, usize N>
	struct RecursiveBinarySearch
	{
		forceinline static int  Find (int left, int right, const K &key, const I* indices, const Pair<K,V>* data)
		{
			if ( left < right )
			{
				int		mid  = (left + right) >> 1;
				auto&	curr = data [indices [mid]].first;
				
				if ( curr < key )
					left = mid + 1;
				else
				if ( curr > key )
					right = mid - 1;
				else
					return mid;
			}
			
			return RecursiveBinarySearch< K, V, I, (N >> 1) >::Find( left, right, key, indices, data );
		}
		
		forceinline static int  LowerBound (int left, int right, const K &key, const I* indices, const Pair<K,V>* data)
		{
			if ( left < right )
			{
				int		mid  = (left + right) >> 1;
				auto&	curr = data [indices [mid]].first;

				if ( curr < key )
					left = mid + 1;
				else
					right = mid;
			}
			
			return RecursiveBinarySearch< K, V, I, (N >> 1) >::LowerBound( left, right, key, indices, data );
		}
	};
	
	template <typename K, typename V, typename I>
	struct RecursiveBinarySearch< K, V, I, 0 >
	{
		forceinline static int  Find (int left, int, const K &, const I *, const Pair<K,V> *)
		{
			return left;
		}

		forceinline static int  LowerBound (int left, int, const K &, const I *, const Pair<K,V> *)
		{
			return left;
		}
	};

}	// _hidden_
	
/*
=================================================
	insert
=================================================
*/
	template <typename K, typename V, usize S>
	inline Pair< typename FixedMap<K,V,S>::iterator, bool >
		FixedMap<K,V,S>::insert (const pair_type &value)
	{
		using BinarySearch = STL::_hidden_::RecursiveBinarySearch< K, V, Index_t, S >;

		usize	i = BinarySearch::LowerBound( 0, _count, value.first, _indices, _array );

		if ( i < _count and value.first == _array[_indices[i]].first )
			return { BitCast< iterator >( &_array[ _indices[i] ]), false };

		// insert
		ASSERT( _count < capacity() );

		const usize	j = _count++;
		PlacementNew<pair_type>( &_array[j], value );
			
		if ( i < _count )
			for (usize k = _count-1; k > i; --k) {
				_indices[k] = _indices[k-1];
			}
		else
			i = j;

		_indices[i]	= Index_t(j);
		return { BitCast< iterator >( &_array[j] ), true };
	}
	
/*
=================================================
	insert
=================================================
*/
	template <typename K, typename V, usize S>
	inline Pair< typename FixedMap<K,V,S>::iterator, bool >
		FixedMap<K,V,S>::insert (pair_type &&value)
	{
		using BinarySearch = STL::_hidden_::RecursiveBinarySearch< K, V, Index_t, S >;

		usize	i = BinarySearch::LowerBound( 0, _count, value.first, _indices, _array );

		if ( i < _count and value.first == _array[_indices[i]].first )
			return { BitCast< iterator >( &_array[ _indices[i] ]), false };

		// insert
		ASSERT( _count < capacity() );
	
		const usize	j = _count++;
		PlacementNew<pair_type>( &_array[j], RVRef(value) );
	
		if ( i < _count )
			for (usize k = _count-1; k > i; --k) {
				_indices[k] = _indices[k-1];
			}
		else
			i = j;

		_indices[i]	= Index_t(j);
		return { BitCast< iterator >( &_array[j] ), true };
	}
	
/*
=================================================
	insert_or_assign
=================================================
*/
	template <typename K, typename V, usize S>
	template <typename M>
	inline Pair< typename FixedMap<K,V,S>::iterator, bool >
		FixedMap<K,V,S>::insert_or_assign (const key_type& key, M&& obj)
	{
		using BinarySearch = STL::_hidden_::RecursiveBinarySearch< K, V, Index_t, S >;

		usize	i = BinarySearch::LowerBound( 0, _count, key, _indices, _array );

		if ( i < _count and key == _array[_indices[i]].first )
		{
			auto	iter = BitCast< iterator >( &_array[ _indices[i] ]);
			std::swap( iter->second, obj );
			return { iter, false };
		}

		// insert
		ASSERT( _count < capacity() );
	
		const usize	j = _count++;
		PlacementNew<pair_type>( &_array[j], pair_type{ key, FwdArg<M &&>(obj) });
		
		if ( i < _count )
			for (usize k = _count-1; k > i; --k) {
				_indices[k] = _indices[k-1];
			}
		else
			i = j;

		_indices[i]	= Index_t(j);
		return { BitCast< iterator >( &_array[j] ), true };
	}
	
/*
=================================================
	insert_or_assign
=================================================
*/
	template <typename K, typename V, usize S>		
	template <typename M>
	inline Pair< typename FixedMap<K,V,S>::iterator, bool >
		FixedMap<K,V,S>::insert_or_assign (key_type&& key, M&& obj)
	{
		using BinarySearch = STL::_hidden_::RecursiveBinarySearch< K, V, Index_t, S >;

		usize	i = BinarySearch::LowerBound( 0, _count, key, _indices, _array );

		if ( i < _count and key == _array[_indices[i]].first )
		{
			auto	iter = BitCast< iterator >( &_array[ _indices[i] ]);
			std::swap( iter->second, obj );
			return { iter, false };
		}

		// insert
		ASSERT( _count < capacity() );
	
		const usize	j = _count++;
		PlacementNew<pair_type>( &_array[j], pair_type{ RVRef(key), FwdArg<M &&>(obj) });
		
		if ( i < _count )
			for (usize k = _count-1; k > i; --k) {
				_indices[k] = _indices[k-1];
			}
		else
			i = j;

		_indices[i]	= Index_t(j);
		return { BitCast< iterator >( &_array[j] ), true };
	}

/*
=================================================
	find
=================================================
*/
	template <typename K, typename V, usize S>
	inline typename FixedMap<K,V,S>::const_iterator
		FixedMap<K,V,S>::find (const key_type &key) const
	{
		using BinarySearch = STL::_hidden_::RecursiveBinarySearch< K, V, Index_t, S >;

		usize	i = BinarySearch::Find( 0, _count, key, _indices, _array );

		if ( i < _count and key == _array[_indices[i]].first )
			return BitCast< const_iterator >( &_array[ _indices[i] ]);
		
		return end();
	}
	
/*
=================================================
	find
=================================================
*/
	template <typename K, typename V, usize S>
	inline typename FixedMap<K,V,S>::iterator
		FixedMap<K,V,S>::find (const key_type &key)
	{
		using BinarySearch = STL::_hidden_::RecursiveBinarySearch< K, V, Index_t, S >;
		
		usize	i = BinarySearch::Find( 0, _count, key, _indices, _array );
		
		if ( i < _count and key == _array[_indices[i]].first )
			return BitCast< iterator >( &_array[ _indices[i] ]);

		return end();
	}
	
/*
=================================================
	count
=================================================
*/
	template <typename K, typename V, usize S>
	inline usize  FixedMap<K,V,S>::count (const key_type &key) const
	{
		using BinarySearch = STL::_hidden_::RecursiveBinarySearch< K, V, Index_t, S >;
		
		usize	cnt = 0;
		usize	i   = BinarySearch::Find( 0, _count, key, _indices, _array );
		
		if ( i < _count and key == _array[_indices[i]].first )
			++cnt;

		return cnt;
	}

/*
=================================================
	clear
=================================================
*/
	template <typename K, typename V, usize S>
	inline void  FixedMap<K,V,S>::clear ()
	{
		for (usize i = 0; i < _count; ++i) {
			_array[i].~pair_type();
		}

		_count = 0;
		
		DEBUG_ONLY( memset( _indices, 0, sizeof(_indices) ));
		DEBUG_ONLY( memset( _array,   0, sizeof(_array)   ));
	}
	
/*
=================================================
	CalcHash
=================================================
*/
	template <typename K, typename V, usize S>
	inline HashVal  FixedMap<K,V,S>::CalcHash () const
	{
		HashVal		result = HashOf( size() );

		for (uint i = 0; i < size(); ++i) {
			result << HashOf( _array[ _indices[i] ] );
		}
		return result;
	}

}	// AE::STL


namespace std
{
	template <typename Key, typename Value, size_t ArraySize>
	struct hash< AE::STL::FixedMap<Key, Value, ArraySize> >
	{
		ND_ size_t  operator () (const AE::STL::FixedMap<Key, Value, ArraySize> &value) const
		{
			return size_t(value.CalcHash());
		}
	};

}	// std
