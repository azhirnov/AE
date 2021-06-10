// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Algorithms/Cast.h"

namespace AE::Math
{

	//
	// Bytes template
	//

	template <typename T>
	struct TBytes
	{
		STATIC_ASSERT( IsInteger<T> and IsScalar<T>, "must be integer scalar" );
		
	// types
	public:
		using Value_t	= T;


	// variables
	private:
		T	_value;


	// methods
	public:
		constexpr TBytes () : _value(0) {}

		explicit constexpr TBytes (T value) : _value(value) {}

		template <typename B>
		explicit constexpr TBytes (const TBytes<B> &other) : _value(T(other)) {}
		
		ND_ explicit constexpr operator sbyte ()	const	{ return sbyte(_value); }
		ND_ explicit constexpr operator sshort ()	const	{ return sshort(_value); }
		ND_ explicit constexpr operator int ()		const	{ return int(_value); }
		ND_ explicit constexpr operator slong ()	const	{ return slong(_value); }

		ND_ explicit constexpr operator ubyte ()	const	{ return ubyte(_value); }
		ND_ explicit constexpr operator ushort ()	const	{ return ushort(_value); }
		ND_ explicit constexpr operator uint ()		const	{ return uint(_value); }
		ND_ explicit constexpr operator ulong ()	const	{ return ulong(_value); }

		template <typename R>
		ND_ explicit constexpr operator R * ()		const	{ return BitCast<R *>( CheckCast<usize>( _value )); }
		
		ND_ constexpr T		Kb ()	const					{ return _value >> 10; }
		ND_ constexpr T		Mb ()	const					{ return _value >> 20; }
		ND_ constexpr T		Gb ()	const					{ return _value >> 30; }
		
		ND_ static constexpr TBytes<T>	FromBits (T value)	{ return TBytes<T>( value >> 3 ); }
		ND_ static constexpr TBytes<T>	FromKb (T value)	{ return TBytes<T>( value << 10 ); }
		ND_ static constexpr TBytes<T>	FromMb (T value)	{ return TBytes<T>( value << 20 ); }
		ND_ static constexpr TBytes<T>	FromGb (T value)	{ return TBytes<T>( value << 30 ); }
		

		template <typename B>	ND_ static constexpr TBytes<T>	SizeOf ()			{ return TBytes<T>( sizeof(B) ); }
		template <typename B>	ND_ static constexpr TBytes<T>	SizeOf (const B &)	{ return TBytes<T>( sizeof(B) ); }
		
		template <typename B>	ND_ static constexpr TBytes<T>	AlignOf ()			{ return TBytes<T>( alignof(B) ); }
		template <typename B>	ND_ static constexpr TBytes<T>	AlignOf (const B &)	{ return TBytes<T>( alignof(B) ); }


		// move any pointer
		template <typename B>	ND_ friend B*  operator +  (B *lhs, const TBytes<T> &rhs)	{ return BitCast<B *>( usize(lhs) + usize(rhs._value) ); }
		template <typename B>	ND_ friend B*  operator -  (B *lhs, const TBytes<T> &rhs)	{ return BitCast<B *>( usize(lhs) - usize(rhs._value) ); }
		template <typename B>		friend B*& operator += (B* &lhs, const TBytes<T> &rhs)	{ return (lhs = lhs + rhs); }
		template <typename B>		friend B*& operator -= (B* &lhs, const TBytes<T> &rhs)	{ return (lhs = lhs + rhs); }


		ND_ constexpr TBytes<T>	operator ~ () const							{ return TBytes<T>( ~_value ); }

			TBytes<T>&			operator += (const TBytes<T> &rhs)			{ _value += rhs._value;  return *this; }
		ND_ constexpr TBytes<T>	operator +  (const TBytes<T> &rhs) const	{ return TBytes<T>( _value + rhs._value ); }
		
			TBytes<T>&			operator -= (const TBytes<T> &rhs)			{ _value -= rhs._value;  return *this; }
		ND_ constexpr TBytes<T>	operator -  (const TBytes<T> &rhs) const	{ return TBytes<T>( _value - rhs._value ); }

			TBytes<T>&			operator *= (const TBytes<T> &rhs)			{ _value *= rhs._value;  return *this; }
		ND_ constexpr TBytes<T>	operator *  (const TBytes<T> &rhs) const	{ return TBytes<T>( _value * rhs._value ); }
		
			TBytes<T>&			operator /= (const TBytes<T> &rhs)			{ _value /= rhs._value;  return *this; }
		ND_ constexpr TBytes<T>	operator /  (const TBytes<T> &rhs) const	{ return TBytes<T>( _value / rhs._value ); }
		
			TBytes<T>&			operator %= (const TBytes<T> &rhs)			{ _value %= rhs._value;  return *this; }
		ND_ constexpr TBytes<T>	operator %  (const TBytes<T> &rhs) const	{ return TBytes<T>( _value % rhs._value ); }
		

			TBytes<T>&			operator += (const T rhs)					{ _value += rhs;  return *this; }
		ND_ constexpr TBytes<T>	operator +  (const T rhs) const				{ return TBytes<T>( _value + rhs ); }
		ND_ friend TBytes<T>	operator +  (T lhs, const TBytes<T> &rhs)	{ return TBytes<T>( lhs + rhs._value ); }
		
			TBytes<T>&			operator -= (const T rhs)					{ _value -= rhs;  return *this; }
		ND_ constexpr TBytes<T>	operator -  (const T rhs) const				{ return TBytes<T>( _value - rhs ); }
		ND_ friend TBytes<T>	operator -  (T lhs, const TBytes<T> &rhs)	{ return TBytes<T>( lhs - rhs._value ); }

			TBytes<T>&			operator *= (const T rhs)					{ _value *= rhs;  return *this; }
		ND_ constexpr TBytes<T>	operator *  (const T rhs) const				{ return TBytes<T>( _value * rhs ); }
		ND_ friend TBytes<T>	operator *  (T lhs, const TBytes<T> &rhs)	{ return TBytes<T>( lhs * rhs._value ); }
		
			TBytes<T>&			operator /= (const T rhs)					{ _value /= rhs;  return *this; }
		ND_ constexpr TBytes<T>	operator /  (const T rhs) const				{ return TBytes<T>( _value / rhs ); }
		ND_ friend TBytes<T>	operator /  (T lhs, const TBytes<T> &rhs)	{ return TBytes<T>( lhs / rhs._value ); }
		
			TBytes<T>&			operator %= (const T rhs)					{ _value %= rhs;  return *this; }
		ND_ constexpr TBytes<T>	operator %  (const T rhs) const				{ return TBytes<T>( _value % rhs ); }
		ND_ friend TBytes<T>	operator %  (T lhs, const TBytes<T> &rhs)	{ return TBytes<T>( lhs % rhs._value ); }


		ND_ constexpr bool		operator == (const TBytes<T> &rhs) const	{ return _value == rhs._value; }
		ND_ constexpr bool		operator != (const TBytes<T> &rhs) const	{ return _value != rhs._value; }
		ND_ constexpr bool		operator >  (const TBytes<T> &rhs) const	{ return _value >  rhs._value; }
		ND_ constexpr bool		operator <  (const TBytes<T> &rhs) const	{ return _value <  rhs._value; }
		ND_ constexpr bool		operator >= (const TBytes<T> &rhs) const	{ return _value >= rhs._value; }
		ND_ constexpr bool		operator <= (const TBytes<T> &rhs) const 	{ return _value <= rhs._value; }
		
		ND_ constexpr bool		operator == (const T rhs) const				{ return _value == rhs; }
		ND_ constexpr bool		operator != (const T rhs) const				{ return _value != rhs; }
		ND_ constexpr bool		operator >  (const T rhs) const				{ return _value >  rhs; }
		ND_ constexpr bool		operator <  (const T rhs) const				{ return _value <  rhs; }
		ND_ constexpr bool		operator >= (const T rhs) const				{ return _value >= rhs; }
		ND_ constexpr bool		operator <= (const T rhs) const				{ return _value <= rhs; }

		ND_ friend constexpr bool	operator == (T lhs, TBytes<T> rhs)		{ return lhs == rhs._value; }
		ND_ friend constexpr bool	operator != (T lhs, TBytes<T> rhs)		{ return lhs != rhs._value; }
		ND_ friend constexpr bool	operator >  (T lhs, TBytes<T> rhs)		{ return lhs >  rhs._value; }
		ND_ friend constexpr bool	operator <  (T lhs, TBytes<T> rhs)		{ return lhs <  rhs._value; }
		ND_ friend constexpr bool	operator >= (T lhs, TBytes<T> rhs)		{ return lhs >= rhs._value; }
		ND_ friend constexpr bool	operator <= (T lhs, TBytes<T> rhs)		{ return lhs <= rhs._value; }
	};
	

	using Bytes = TBytes< ulong >;
	
	template <typename T>
	inline static constexpr Bytes	SizeOf = Bytes::SizeOf<T>();
	
	template <typename T>
	inline static constexpr Bytes	AlignOf = Bytes::AlignOf<T>();
	

	ND_ constexpr Bytes  operator "" _b  (unsigned long long value)	{ return Bytes( CheckCast<Bytes::Value_t>(value) ); }
	ND_ constexpr Bytes  operator "" _Kb (unsigned long long value)	{ return Bytes::FromKb( CheckCast<Bytes::Value_t>(value) ); }
	ND_ constexpr Bytes  operator "" _Mb (unsigned long long value)	{ return Bytes::FromMb( CheckCast<Bytes::Value_t>(value) ); }
	ND_ constexpr Bytes  operator "" _Gb (unsigned long long value)	{ return Bytes::FromGb( CheckCast<Bytes::Value_t>(value) ); }

	
/*
=================================================
	OffsetOf
=================================================
*/
	template <typename A, typename B>
	ND_ constexpr forceinline Bytes  OffsetOf (A B::*member)
	{
		const union U {
			B		b;
			int		tmp;
			U () : tmp(0) {}
			~U () {}
		} u;
		return Bytes( usize(std::addressof(u.b.*member)) - usize(std::addressof(u.b)) );
	}

}	// AE::Math


namespace std
{
	template <typename T>
	struct hash< AE::Math::TBytes<T> >
	{
		ND_ size_t  operator () (const AE::Math::TBytes<T> &value) const
		{
			return size_t(AE::STL::HashOf( T(value) ));
		}
	};

}	// std
