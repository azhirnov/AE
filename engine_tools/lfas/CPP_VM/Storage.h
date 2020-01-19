// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "CPP_VM/VirtualMachine.h"
#include "stl/Memory/MemUtils.h"
#include "stl/CompileTime/TemplateUtils.h"

namespace LFAS::CPP
{
namespace _lfas_cpp_hidden_
{

	//
	// Value Storage
	//

	template <typename T>
	class ValueStorage
	{
	// variables
	protected:
		T	_value;


	// methods
	public:
		ValueStorage ();
		ValueStorage (const T &val);
		ValueStorage (const ValueStorage<T> &);
		ValueStorage (ValueStorage<T> &&);
		~ValueStorage ();

		ValueStorage<T>&  operator = (const ValueStorage<T> &);
		ValueStorage<T>&  operator = (ValueStorage<T> &&);

		ND_ const T  Read () const;

		template <typename V>
		void  Write (V&& value);
	};

	

	//
	// Class Storage
	//

	template <typename T>
	class ClassStorage : public ValueStorage<T>
	{
		STATIC_ASSERT( IsClass<T> );

	// methods
	public:
		ClassStorage () = default;
		ClassStorage (const ClassStorage<T> &) = default;
		ClassStorage (ClassStorage<T> &&) = default;
		
		ClassStorage<T>&  operator = (const ClassStorage<T> &) = default;
		ClassStorage<T>&  operator = (ClassStorage<T> &&) = default;

		ND_ const T  Read () const	{ return ValueStorage<T>::Read(); }

		template <typename M>
		ND_ const M  Read (M T::*member) const;
		
		template <typename V>
		void  Write (V&& value)		{ return ValueStorage<T>::Write( std::forward<V>( value )); }

		template <typename M, typename V>
		void  Write (M T::*member, V&& value);
	};



	//
	// Array Value Storage
	//
	
	template <typename T, typename Count>
	class ArrayValueStorage
	{
	// types
	private:
		using Self = ArrayValueStorage< T, Count >;


	// variables
	protected:
		static constexpr size_t		_Count = Count::value;

		StaticArray< T, _Count >	_arr;


	// methods
	public:
		ArrayValueStorage ();
		ArrayValueStorage (const Self &) = delete;
		ArrayValueStorage (Self &&) = delete;
		~ArrayValueStorage ();
		
		Self&  operator = (const Self &) = delete;
		Self&  operator = (Self &&) = delete;

		ND_ const T  Read (size_t idx) const;

		template <typename V>
		void  Write (size_t idx, V&& value);
	};
	


	//
	// Array Class Storage
	//
	
	template <typename T, typename Count>
	class ArrayClassStorage : public ArrayValueStorage< T, Count >
	{
		STATIC_ASSERT( IsClass<T> );

	// methods
	public:
		template <typename M>
		ND_ const M  Read (size_t idx, M T::*member) const;
		
		template <typename M, typename V>
		void  Write (size_t idx, M T::*member, V&& value);
	};


}	// _lfas_cpp_hidden_
//-----------------------------------------------------------------------------


	template <typename T>
	using Storage = typename Conditional< IsClass<T>,
						DeferredTemplate< _lfas_cpp_hidden_::ClassStorage, T >,
						DeferredTemplate< _lfas_cpp_hidden_::ValueStorage, T > >::type;
	
	template <typename T, size_t Count>
	using ArrayStorage = typename Conditional< IsClass<T>,
							DeferredTemplate< _lfas_cpp_hidden_::ArrayClassStorage, T, ValueToType<Count> >,
							DeferredTemplate< _lfas_cpp_hidden_::ArrayValueStorage, T, ValueToType<Count> > >::type;


namespace _lfas_cpp_hidden_
{
/*
=================================================
	constructor
=================================================
*/
	template <typename T>
	inline ValueStorage<T>::ValueStorage ()
	{
		VirtualMachine::Instance().StorageCreate( AddressOf(_value), SizeOf<T> );
	}
	
	template <typename T>
	inline ValueStorage<T>::ValueStorage (const T &val) : _value{ val }
	{
		VirtualMachine::Instance().StorageCreate( AddressOf(_value), SizeOf<T> );
	}
	
	template <typename T>
	inline ValueStorage<T>::ValueStorage (const ValueStorage<T> &other) : _value{ other.Read() }
	{
		VirtualMachine::Instance().StorageCreate( AddressOf(_value), SizeOf<T> );
	}
	
	template <typename T>
	inline ValueStorage<T>::ValueStorage (ValueStorage<T> &&other) : _value{ other.Read() }
	{
		VirtualMachine::Instance().StorageCreate( AddressOf(_value), SizeOf<T> );
	}

/*
=================================================
	destructor
=================================================
*/
	template <typename T>
	inline ValueStorage<T>::~ValueStorage ()
	{
		VirtualMachine::Instance().StorageDestroy( AddressOf(_value) );
	}
	
/*
=================================================
	operator =
=================================================
*/
	template <typename T>
	inline ValueStorage<T>&  ValueStorage<T>::operator = (const ValueStorage<T> &rhs)
	{
		Write( rhs.Read() );
		return *this;
	}
	
	template <typename T>
	inline ValueStorage<T>&  ValueStorage<T>::operator = (ValueStorage<T> &&rhs)
	{
		Write( rhs.Read() );
		return *this;
	}

/*
=================================================
	Read
=================================================
*/
	template <typename T>
	inline const T  ValueStorage<T>::Read () const
	{
		VirtualMachine::Instance().StorageReadAccess( AddressOf(_value), 0_b, SizeOf<T> );
		return _value;
	}

/*
=================================================
	Write
=================================================
*/
	template <typename T>
	template <typename V>
	inline void  ValueStorage<T>::Write (V&& value)
	{
		VirtualMachine::Instance().StorageWriteAccess( AddressOf(_value), 0_b, SizeOf<T> );
		_value = std::forward<V>( value );
	}
//-----------------------------------------------------------------------------



/*
=================================================
	Read
=================================================
*/
	template <typename T>
	template <typename M>
	inline const M  ClassStorage<T>::Read (M T::*member) const
	{
		VirtualMachine::Instance().StorageReadAccess( AddressOf(_value), OffsetOf(member), SizeOf<M> );
		return _value.*member;
	}

/*
=================================================
	Write
=================================================
*/
	template <typename T>
	template <typename M, typename V>
	inline void  ClassStorage<T>::Write (M T::*member, V&& value)
	{
		VirtualMachine::Instance().StorageWriteAccess( AddressOf(_value), OffsetOf(member), SizeOf<M> );
		_value.*member = std::forward<V>( value );
	}
//-----------------------------------------------------------------------------

	

/*
=================================================
	constructor
=================================================
*/
	template <typename T, typename C>
	inline ArrayValueStorage<T,C>::ArrayValueStorage ()
	{
		VirtualMachine::Instance().StorageCreate( AddressOf(_arr), SizeOf<decltype(_arr)> );
	}
	
/*
=================================================
	destructor
=================================================
*/
	template <typename T, typename C>
	inline ArrayValueStorage<T,C>::~ArrayValueStorage ()
	{
		VirtualMachine::Instance().StorageDestroy( AddressOf(_arr) );
	}
	
/*
=================================================
	Read
=================================================
*/
	template <typename T, typename C>
	inline const T  ArrayValueStorage<T,C>::Read (size_t idx) const
	{
		CHECK( idx < _arr.size() );
		VirtualMachine::Instance().StorageReadAccess( AddressOf(_arr), AddressDistance( _arr[idx], _arr ), SizeOf<T> );
		return _arr[idx];
	}
	
/*
=================================================
	Write
=================================================
*/
	template <typename T, typename C>
	template <typename V>
	inline void  ArrayValueStorage<T,C>::Write (size_t idx, V&& value)
	{
		VirtualMachine::Instance().StorageWriteAccess( AddressOf(_arr), AddressDistance( _arr[idx], _arr ), SizeOf<T> );
		_arr[idx] = std::forward<V>( value );
	}
//-----------------------------------------------------------------------------

	
	
/*
=================================================
	Read
=================================================
*/
	template <typename T, typename C>
	template <typename M>
	inline const M  ArrayClassStorage<T,C>::Read (size_t idx, M T::*member) const
	{
		CHECK( idx < _arr.size() );
		VirtualMachine::Instance().StorageReadAccess( AddressOf(_arr), AddressDistance( _arr[idx], _arr ) + OffsetOf(member), SizeOf<M> );
		return _arr[idx].*member;
	}

/*
=================================================
	Write
=================================================
*/
	template <typename T, typename C>	
	template <typename M, typename V>
	inline void  ArrayClassStorage<T,C>::Write (size_t idx, M T::*member, V&& value)
	{
		VirtualMachine::Instance().StorageWriteAccess( AddressOf(_arr), AddressDistance( _arr[idx], _arr ) + OffsetOf(member), SizeOf<M> );
		_arr[idx].*member = std::forward<V>( value );
	}


}	// _lfas_cpp_hidden_
}	// LFAS::CPP
