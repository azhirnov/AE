// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Common.h"

namespace AE::Math
{

	//
	// Fractional
	//

	template <typename T>
	struct Fractional
	{
	// types
		using Self	= Fractional<T>;


	// variables
		T	numerator	{0};
		T	denominator	{1};


	// methods
		constexpr Fractional () {}

		constexpr explicit Fractional (T num, T denom = T(1))
		{
			//ASSERT( denom > 0 );
			const T	gcd = _GreatestCommonDivisor( num, denom );
			if ( gcd ) {
				numerator	= num / gcd;
				denominator	= denom / gcd;
			}
		}

		constexpr Fractional (const Self &) = default;
		
		constexpr Self&  operator = (const Self &) = default;


		ND_ constexpr Self  operator - () const
		{
			return Self{ -numerator, denominator };
		}

		ND_ constexpr Self  operator + (const Self &rhs) const
		{
			return Self{ numerator * rhs.denominator + rhs.numerator * denominator, denominator * rhs.denominator };
		}
		
		ND_ constexpr Self  operator - (const Self &rhs) const
		{
			return Self{ numerator * rhs.denominator - rhs.numerator * denominator, denominator * rhs.denominator };
		}
		
		ND_ constexpr Self  operator * (const Self &rhs) const
		{
			return Self{ numerator * rhs.numerator, denominator * rhs.denominator };
		}
		
		ND_ constexpr Self  operator / (const Self &rhs) const
		{
			return Self{ numerator * rhs.denominator, denominator * rhs.numerator };
		}

		ND_ constexpr Self  Pow (uint value) const
		{
			Self	result;
			result.numerator	= numerator;
			result.denominator	= denominator;

			for (uint i = 0; i < value; ++i) {
				result.numerator	*= numerator;
				result.denominator	*= denominator;
			}
			return result;
		}

		ND_ constexpr bool  operator == (const Self &rhs) const
		{
			return	(numerator == T(0) and rhs.numerator == T(0))				or
					(numerator == rhs.numerator and denominator == rhs.denominator);
		}

		ND_ constexpr bool  IsZero ()		const		{ return numerator == T(0); }

		ND_ constexpr bool  IsInteger ()	const		{ return denominator == T(1); }


	private:
		static constexpr T  _GreatestCommonDivisor (T value1, T value2)
		{
			return value2 != 0 ? _GreatestCommonDivisor( value2, value1 % value2 ) : value1;
		}
	};
	

	using FractionalI	= Fractional< int >;


}	// AE::Math
