// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Memory/MemUtils.h"

namespace AE::STL
{

	//
	// In Place Storage
	//

	template <typename T>
	struct InPlace final
	{
	// types
	public:
		using Self		= InPlace< T >;
		using Value_t	= T;


	// variables
	private:
		union {
			T			_value;
			ubyte		_data [ sizeof(T) ];	// don't use it!
		};
		DEBUG_ONLY(
			bool		_isCreated = false;
		)


	// methods
	public:
		InPlace ()
		{}

		InPlace (const Self &other)
			DEBUG_ONLY(: _isCreated{other._isCreated})
		{
			PlacementNew<T>( &_value, other._value );
		}

		InPlace (Self&& other)
			DEBUG_ONLY(: _isCreated{other._isCreated})
		{
			PlacementNew<T>( &_value, RVRef(other._value) );
		}

		~InPlace ()
		{
			ASSERT( not _isCreated );
		}


		Self&  operator = (const Self &rhs) = delete;
		Self&  operator = (Self&& rhs) = delete;


		template <typename ...Args>
		Self&  Create (Args&& ...args)
		{
			DEBUG_ONLY(
				ASSERT( not _isCreated );
				_isCreated = true;
			)
			PlacementNew<T>( &_value, FwdArg<Args &&>( args )... );
			return *this;
		}

		void  Destroy ()
		{
			DEBUG_ONLY(
				ASSERT( _isCreated );
				_isCreated = false;
			)
			_value.~T();
		}

		ND_ T *			operator -> ()			{ ASSERT( _isCreated );  return &_value; }
		ND_ T const*	operator -> ()	const	{ ASSERT( _isCreated );  return &_value; }

		ND_ T &			operator * ()			{ ASSERT( _isCreated );  return _value; }
		ND_ T const&	operator * ()	const	{ ASSERT( _isCreated );  return _value; }

		ND_ T *			operator & ()			{ return &_value; }
		ND_ T const*	operator & ()	const	{ return &_value; }

		DEBUG_ONLY(
			ND_ bool	IsCreated ()	const	{ return _isCreated; }
		)
	};


}	// AE::STL
