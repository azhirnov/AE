// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "threading/Common.h"
#include "stl/Containers/Ptr.h"
#include "stl/Algorithms/Cast.h"

namespace AE::Threading
{
	
	//
	// Enable Reference Counting
	//

	template <typename T>
	struct EnableRC
	{
		template <typename B>
		friend struct RC;

	// variables
	private:
		Atomic<int>		_counter {0};


	// methods
	public:
		EnableRC () {}
		virtual ~EnableRC () {}

		EnableRC (EnableRC<T> &&) = delete;
		EnableRC (const EnableRC<T> &) = delete;

		EnableRC<T>&  operator = (EnableRC<T> &&) = delete;
		EnableRC<T>&  operator = (const EnableRC<T> &) = delete;

		ND_ auto  GetRC ();

		void  AddRef ();
		void  ReleaseRef ();
	};



	//
	// Reference Counter Pointer
	//

	template <typename T>
	struct RC
	{
		template <typename B>
		friend struct EnableRC;

	// variables
	private:
		T *		_ptr = null;


	// methods
	public:
		RC () {}
		RC (std::nullptr_t) {}

		RC (T* ptr) : _ptr{ptr}							{ _Inc(); }
		RC (Ptr<T> ptr) : _ptr{ptr}						{ _Inc(); }
		RC (RC<T> &&other) : _ptr{other._ptr}			{ other._ptr = null; }
		RC (const RC<T> &other) : _ptr{other._ptr}		{ _Inc(); }

		~RC ()											{ _Dec(); }

		RC&  operator = (std::nullptr_t)				{ _Dec();  _ptr = null;                return *this; }
		RC&  operator = (T* rhs)						{ _Dec();  _ptr = rhs;        _Inc();  return *this; }
		RC&  operator = (Ptr<T> rhs)					{ _Dec();  _ptr = rhs.get();  _Inc();  return *this; }
		RC&  operator = (const RC<T> &rhs)				{ _Dec();  _ptr = rhs._ptr;   _Inc();  return *this; }
		RC&  operator = (RC<T> &&rhs)					{ _Dec();  _ptr = rhs.release();       return *this; }

		template <typename B, typename = EnableIf<IsBaseOf<T,B>> >
		RC&  operator = (RC<B> &&rhs)					{ _Dec();  _ptr = rhs.release();       return *this; }
		
		template <typename B, typename = EnableIf<IsBaseOf<T,B>> >
		RC&  operator = (const RC<B> &&rhs)				{ _Dec();  _ptr = rhs.get();  _Inc();  return *this; }

		ND_ bool  operator == (const T* rhs)	 const	{ return _ptr == rhs; }
		ND_ bool  operator == (Ptr<T> rhs)		 const	{ return _ptr == rhs.get(); }
		ND_ bool  operator == (const RC<T> &rhs) const	{ return _ptr == rhs._ptr; }

		ND_ T *		operator -> ()				const	{ ASSERT( _ptr );  return _ptr; }
		ND_ T &		operator *  ()				const	{ ASSERT( _ptr );  return *_ptr; }

		ND_ T *		get ()						const	{ return _ptr; }
		ND_ T *		release ()							{ T* p = _ptr;  _ptr = null;  return p; }
		ND_ int		use_count ()				const	{ return _ptr ? _ptr->_counter.load( EMemoryOrder::Relaxed ) : 0; }

		ND_ explicit operator bool ()			const	{ return _ptr; }

	private:
		RC (T* ptr, int) : _ptr{ptr} {}

		void  _Inc ();
		void  _Dec ();
	};

//-----------------------------------------------------------------------------


/*
=================================================
	MakeRC
=================================================
*/
	template <typename T, typename ...Args>
	ND_ forceinline RC<T>  MakeRC (Args&& ...args)
	{
		return RC<T>{ new T{ std::forward<Args>(args)... }};
	}
	
/*
=================================================
	_Inc
=================================================
*/
	template <typename T>
	forceinline void  RC<T>::_Inc ()
	{
		if ( _ptr )
			_ptr->_counter.fetch_add( 1, EMemoryOrder::Relaxed );
	}
	
/*
=================================================
	_Dec
=================================================
*/
	template <typename T>
	forceinline void  RC<T>::_Dec ()
	{
		if ( _ptr )
		{
			const auto	res = _ptr->_counter.fetch_sub( 1, EMemoryOrder::Release );
			ASSERT( res > 0 );

			if_unlikely( res == 1 )
			{
				ThreadFence( EMemoryOrder::Acquire );
				delete _ptr;
				_ptr = null;
			}
		}
	}
//-----------------------------------------------------------------------------



/*
=================================================
	GetRC
=================================================
*/
	template <typename T>
	forceinline auto  EnableRC<T>::GetRC ()
	{
		return RC<T>{ this, 0 };
	}
	
/*
=================================================
	AddRef
=================================================
*/
	template <typename T>
	forceinline void  EnableRC<T>::AddRef ()
	{
		_counter.fetch_add( 1, EMemoryOrder::Relaxed );
	}
	
/*
=================================================
	ReleaseRef
=================================================
*/
	template <typename T>
	forceinline void  EnableRC<T>::ReleaseRef ()
	{
		const auto	res = _counter.fetch_sub( 1, EMemoryOrder::Release );
		ASSERT( res > 0 );

		if_unlikely( res == 1 )
		{
			ThreadFence( EMemoryOrder::Acquire );
			delete this;
		}
	}

}	// AE::Threading
//-----------------------------------------------------------------------------


namespace AE::STL
{

	template <typename R, typename T>
	ND_ forceinline constexpr Threading::RC<R>  Cast (const Threading::RC<T> &value)
	{
		return Threading::RC<R>{ Cast<R>( value.get() )};
	}

}	// AE::STL
