// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Math/Vec.h"

namespace AE::STL
{

	//
	// Fixed Size String
	//

	template <typename CharT, size_t StringSize>
	struct TFixedString
	{
	// type
	public:
		using value_type		= CharT;
		using iterator			= CharT *;
		using const_iterator	= CharT const *;
		using View_t			= BasicStringView< CharT >;
		using Self				= TFixedString< CharT, StringSize >;


	// variables
	private:
		size_t		_length	= 0;
		CharT		_array [StringSize] = {};


	// methods
	public:
		constexpr TFixedString ()
		{
			DEBUG_ONLY( memset( _array, 0, sizeof(_array) ));
		}
		
		TFixedString (const View_t &view) : TFixedString{ view.data(), view.length() }
		{}
		
		constexpr TFixedString (const CharT *str)
		{
			for (; str[_length] and _length < StringSize; ++_length) {
				_array[_length] = str[_length];
			}
			_array[_length] = CharT(0);
		}

		constexpr TFixedString (const CharT *str, size_t length)
		{
			ASSERT( length < StringSize );
			
			for (; _length < length and _length < StringSize; ++_length) {
				_array[_length] = str[_length];
			}
			_array[_length] = CharT(0);
		}


		ND_ constexpr operator View_t ()					const	{ return View_t{ _array, length() }; }

		ND_ constexpr size_t		size ()					const	{ return _length; }
		ND_ constexpr size_t		length ()				const	{ return size(); }
		ND_ static constexpr size_t	capacity ()						{ return StringSize; }
		ND_ constexpr bool			empty ()				const	{ return _length == 0; }
		ND_ constexpr CharT const *	c_str ()				const	{ return _array; }
		ND_ constexpr CharT const *	data ()					const	{ return _array; }
		ND_ CharT *					data ()							{ return _array; }

		ND_ constexpr CharT &		operator [] (size_t i)			{ ASSERT( i < _length );  return _array[i]; }
		ND_ constexpr CharT const &	operator [] (size_t i)	const	{ ASSERT( i < _length );  return _array[i]; }

		ND_ bool	operator == (const View_t &rhs)			const	{ return View_t(*this) == rhs; }
		ND_ bool	operator != (const View_t &rhs)			const	{ return not (*this == rhs); }
		ND_ bool	operator >  (const View_t &rhs)			const	{ return View_t(*this) > rhs; }
		ND_ bool	operator <  (const View_t &rhs)			const	{ return View_t(*this) < rhs; }

		ND_ iterator		begin ()								{ return &_array[0]; }
		ND_ const_iterator	begin ()						const	{ return &_array[0]; }
		ND_ iterator		end ()									{ return &_array[_length]; }
		ND_ const_iterator	end ()							const	{ return &_array[_length]; }


		void clear ()
		{
			_array[0]	= CharT(0);
			_length		= 0;
		}

		void resize (size_t newSize)
		{
			ASSERT( newSize < capacity() );

			newSize = Min( newSize, capacity()-1 );

			if ( newSize > _length )
			{
				std::memset( &_array[_length], 0, newSize - _length );
			}

			_length = newSize;
		}
	};


	template <size_t StringSize>
	using FixedString = TFixedString< char, StringSize >;

}	// AE::STL


namespace std
{

	template <typename CharT, size_t StringSize>
	struct hash< AE::STL::TFixedString< CharT, StringSize > >
	{
		ND_ size_t  operator () (const AE::STL::TFixedString<CharT, StringSize> &value) const
		{
			return hash< AE::STL::BasicStringView<CharT> >()( value );
		}
	};

}	// std
