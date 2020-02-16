// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'
/*
	UMax constant is maximum value of unsigned integer type.
*/

#pragma once

namespace AE::STL
{

	//
	// UMax
	//
	namespace _ae_stl_hidden_
	{
		struct _UMax
		{
			template <typename T>
			ND_ constexpr operator const T () const
			{
				STATIC_ASSERT( T(~T(0)) > T(0) );
				return T(~T(0));
			}
			
			template <typename T>
			ND_ friend constexpr bool operator == (const T& left, const _UMax &right)
			{
				return T(right) == left;
			}
			
			template <typename T>
			ND_ friend constexpr bool operator != (const T& left, const _UMax &right)
			{
				return T(right) != left;
			}
		};
	}
	static constexpr _ae_stl_hidden_::_UMax		UMax {};


	
	//
	// Zero
	//
	namespace _ae_stl_hidden_
	{
		struct _Zero
		{
			template <typename T>
			ND_ constexpr operator const T () const
			{
				//STATIC_ASSERT( std::is_integral_v<T> || std::is_enum_v<T> );
				return T(0);
			}
			
			template <typename T>
			ND_ friend constexpr auto  operator == (const T& left, const _Zero &right)
			{
				return T(right) == left;
			}
			
			template <typename T>
			ND_ friend constexpr auto  operator != (const T& left, const _Zero &right)
			{
				return T(right) != left;
			}

			template <typename T>
			ND_ friend constexpr auto  operator > (const T& left, const _Zero &right)
			{
				return left > T(right);
			}

			template <typename T>
			ND_ friend constexpr auto  operator < (const T& left, const _Zero &right)
			{
				return left < T(right);
			}

			template <typename T>
			ND_ friend constexpr auto  operator >= (const T& left, const _Zero &right)
			{
				return left >= T(right);
			}

			template <typename T>
			ND_ friend constexpr auto  operator <= (const T& left, const _Zero &right)
			{
				return left <= T(right);
			}
		};
	}
	static constexpr _ae_stl_hidden_::_Zero		Zero {};



	//
	// Any Float Constant
	//

	struct AnyFloatConst
	{
	private:
		const double	_d;
		const float		_f;


	public:
		constexpr AnyFloatConst (double val) : _d{val}, _f{float(val)} {}
		
		template <typename T>
		ND_ constexpr operator const T () const
		{
			if constexpr( std::is_same_v< T, double >)
				return _d;
			
			if constexpr( std::is_same_v< T, float >)
				return _f;

			// fail to compile
		}
	};


}	// AE::STL
