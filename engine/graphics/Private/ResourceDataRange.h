// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "graphics/Public/Common.h"

namespace AE::Graphics
{

	//
	// Resource Data Range
	//

	template <typename T>
	struct ResourceDataRange
	{
		STATIC_ASSERT( IsInteger<T> );

	// types
	public:
		using Self			= ResourceDataRange<T>;
		using Value_t		= T;
		using SubRange_t	= Self;


	// variables
	public:
		T	begin;
		T	end;


	// methods
	public:
		ResourceDataRange () : begin{UMax}, end{Zero} {}
		ResourceDataRange (T begin, T end) : begin{begin}, end{end} {}

		ND_ T		Count ()							const	{ ASSERT( not IsEmpty() );  return end == UMax ? UMax : end - begin; }

		ND_ bool	IsWhole ()							const	{ return end == UMax; }
		ND_ bool	IsEmpty ()							const	{ return end <= begin; }

		ND_ bool	operator == (const Self &rhs)		const	{ return begin == rhs.begin and end == rhs.end; }
		ND_ bool	operator != (const Self &rhs)		const	{ return not (*this == rhs); }

		ND_ Self	operator + (T rhs)					const	{ return (Self(*this) += rhs); }
			
		ND_ Self	Intersect (const Self &other)		const	{ return Self{Max( begin, other.begin ), Min( end, other.end )}; }
		ND_ bool	IsIntersects (const Self &other)	const	{ return not ((end < other.begin) | (begin > other.end)); }
		

		Self &		operator += (T rhs)
		{
			begin = AdditionIsSafe( begin, rhs ) ? begin + rhs : UMax;
			end   = AdditionIsSafe( end,   rhs ) ? end   + rhs : UMax;
			return *this;
		}


		Self &		Merge (const Self &other)
		{
			ASSERT( IsIntersects( other ));
			begin = Min( begin, other.begin );
			end   = Max( end,   other.end   );
			return *this;
		}
	};


}	// AE::Graphics
