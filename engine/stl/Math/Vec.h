// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Math/Math.h"
#include "stl/Math/BitMath.h"
#include "stl/Math/GLM.h"

namespace AE::Math
{

	using bool2		= Vec< bool, 2 >;
	using bool3		= Vec< bool, 3 >;
	using bool4		= Vec< bool, 4 >;

	using byte2		= Vec< int8_t, 2 >;
	using byte3		= Vec< int8_t, 3 >;
	using byte4		= Vec< int8_t, 4 >;
	
	using ubyte2	= Vec< uint8_t, 2 >;
	using ubyte3	= Vec< uint8_t, 3 >;
	using ubyte4	= Vec< uint8_t, 4 >;

	using short2	= Vec< int16_t, 2 >;
	using short3	= Vec< int16_t, 3 >;
	using short4	= Vec< int16_t, 4 >;
	
	using ushort2	= Vec< uint16_t, 2 >;
	using ushort3	= Vec< uint16_t, 3 >;
	using ushort4	= Vec< uint16_t, 4 >;

	using uint2		= Vec< uint, 2 >;
	using uint3		= Vec< uint, 3 >;
	using uint4		= Vec< uint, 4 >;

	using int2		= Vec< int, 2 >;
	using int3		= Vec< int, 3 >;
	using int4		= Vec< int, 4 >;

	using float2	= Vec< float, 2 >;
	using float3	= Vec< float, 3 >;
	using float4	= Vec< float, 4 >;
	
	using double2	= Vec< double, 2 >;
	using double3	= Vec< double, 3 >;
	using double4	= Vec< double, 4 >;
	
/*
=================================================
	IsVec
=================================================
*/
	namespace _ae_math_hidden_
	{
		template <typename T>
		struct _IsVec {
			static constexpr bool	value = false;
		};

		template <typename T, int I>
		struct _IsVec< Vec<T,I> > {
			static constexpr bool	value = true;
		};
	}

	template <typename T>
	static constexpr bool  IsVec = _ae_math_hidden_::_IsVec<T>::value;
	
/*
=================================================
	VecSize
=================================================
*/
	namespace _ae_math_hidden_
	{
		template <typename T>
		struct _VecSize {
		};

		template <typename T, int I>
		struct _VecSize< Vec<T,I> > {
			static constexpr int	value = I;
		};
	}

	template <typename T>
	static constexpr uint  VecSize = _ae_math_hidden_::_VecSize<T>::value;
	
/*
=================================================
	operator !
=================================================
*/
	template <typename T, int I>
	ND_ forceinline GLM_CONSTEXPR EnableIf<IsScalar<T>, Vec<T,I>>  operator ! (const Vec<T,I> &value)
	{
		Vec<T,I>	res;
		for (int i = 0; i < I; ++i) {
			res[i] = !value[i];
		}
		return res;
	}
	
/*
=================================================
	operator << , operator >>
=================================================
*/
	template <typename T, int I, typename S>
	ND_ forceinline GLM_CONSTEXPR EnableIf<IsScalar<S> and not IsSameTypes<T,S>, Vec<T,I>>  operator << (const Vec<T,I> &lhs, const S &rhs)
	{
		return lhs << static_cast<T>(rhs);
	}

	template <typename T, int I, typename S>
	ND_ forceinline GLM_CONSTEXPR EnableIf<IsScalar<S> and not IsSameTypes<T,S>, Vec<T,I>>  operator >> (const Vec<T,I> &lhs, const S &rhs)
	{
		return lhs >> static_cast<T>(rhs);
	}
	
/*
=================================================
	operator *, oeprator /
=================================================
*/
	template <typename T, int I, typename S>
	ND_ forceinline GLM_CONSTEXPR EnableIf<IsScalar<S> and not IsSameTypes<T,S>, Vec<T,I>>  operator * (const Vec<T,I> &lhs, const S &rhs)
	{
		return lhs * static_cast<T>(rhs);
	}

	template <typename T, int I, typename S>
	ND_ forceinline GLM_CONSTEXPR EnableIf<IsScalar<S> and not IsSameTypes<T,S>, Vec<T,I>>  operator / (const Vec<T,I> &lhs, const S &rhs)
	{
		return lhs / static_cast<T>(rhs);
	}

/*
=================================================
	operator %
=================================================
*/
	/*template <typename T, int I>
	ND_ forceinline EnableIf<IsFloatPoint<T>, Vec<T,I>>  operator % (const Vec<T,I> &lhs, const Vec<T,I> &rhs)
	{
		return glm::mod( lhs, rhs );
	}*/
	
	template <typename T, int I>
	ND_ forceinline EnableIf<IsScalar<T> and IsFloatPoint<T>, Vec<T,I>&>  operator %= (Vec<T,I> &lhs, const Vec<T,I> &rhs)
	{
		return (lhs = glm::mod( lhs, rhs ));
	}

/*
=================================================
	operator <, >, ==
=================================================
*/
	template <typename T, int I, typename S>
	ND_ forceinline GLM_CONSTEXPR EnableIf<IsScalar<S>, Vec<bool,I>>  operator == (const Vec<T,I> &lhs, const S &rhs)
	{
		return glm::equal( lhs, Vec<T,I>{rhs} );
	}
	
	template <typename T, int I, typename S>
	ND_ forceinline GLM_CONSTEXPR EnableIf<IsScalar<S>, Vec<bool,I>>  operator != (const Vec<T,I> &lhs, const S &rhs)
	{
		return glm::notEqual( lhs, Vec<T,I>{rhs} );
	}
	
	template <typename T, int I>
	ND_ forceinline GLM_CONSTEXPR Vec<bool,I>  operator >= (const Vec<T,I> &lhs, const Vec<T,I> &rhs)
	{
		return glm::greaterThanEqual( lhs, rhs );
	}

	template <typename T, int I, typename S>
	ND_ forceinline GLM_CONSTEXPR EnableIf<IsScalar<S>, Vec<bool,I>>  operator >= (const Vec<T,I> &lhs, const S &rhs)
	{
		return glm::greaterThanEqual( lhs, Vec<T,I>{rhs} );
	}
	
	template <typename T, int I>
	ND_ forceinline GLM_CONSTEXPR Vec<bool,I>  operator <= (const Vec<T,I> &lhs, const Vec<T,I> &rhs)
	{
		return glm::lessThanEqual( lhs, rhs );
	}

	template <typename T, int I, typename S>
	ND_ forceinline GLM_CONSTEXPR EnableIf<IsScalar<S>, Vec<bool,I>>  operator <= (const Vec<T,I> &lhs, const S &rhs)
	{
		return glm::lessThanEqual( lhs, Vec<T,I>{rhs} );
	}
	
	template <typename T, int I>
	ND_ forceinline GLM_CONSTEXPR Vec<bool,I>  operator > (const Vec<T,I> &lhs, const Vec<T,I> &rhs)
	{
		return glm::greaterThan( lhs, rhs );
	}
	
	template <typename T, int I, typename S>
	ND_ forceinline GLM_CONSTEXPR EnableIf<IsScalar<S>, Vec<bool,I>>  operator > (const Vec<T,I> &lhs, const S &rhs)
	{
		return glm::greaterThan( lhs, Vec<T,I>{rhs} );
	}
	
	template <typename T, int I>
	ND_ forceinline GLM_CONSTEXPR Vec<bool,I>  operator < (const Vec<T,I> &lhs, const Vec<T,I> &rhs)
	{
		return glm::lessThan( lhs, rhs );
	}
	
	template <typename T, int I, typename S>
	ND_ forceinline GLM_CONSTEXPR EnableIf<IsScalar<S>, Vec<bool,I>>  operator < (const Vec<T,I> &lhs, const S &rhs)
	{
		return glm::lessThan( lhs, Vec<T,I>{rhs} );
	}

/*
=================================================
	All
=================================================
*/
	ND_ forceinline GLM_CONSTEXPR bool  All (const bool2 &v)
	{
		return v.x & v.y;
	}

	ND_ forceinline GLM_CONSTEXPR bool  All (const bool3 &v)
	{
		return v.x & v.y & v.z;
	}

	ND_ forceinline GLM_CONSTEXPR bool  All (const bool4 &v)
	{
		return v.x & v.y & v.z & v.w;
	}
	
/*
=================================================
	Any
=================================================
*/
	ND_ forceinline GLM_CONSTEXPR bool  Any (const bool2 &v)
	{
		return v.x | v.y;
	}

	ND_ forceinline GLM_CONSTEXPR bool  Any (const bool3 &v)
	{
		return v.x | v.y | v.z;
	}

	ND_ forceinline GLM_CONSTEXPR bool  Any (const bool4 &v)
	{
		return v.x | v.y | v.z | v.w;
	}

/*
=================================================
	Abs
=================================================
*/
	template <typename T, int I>
	ND_ forceinline EnableIf<IsScalar<T>, Vec<T,I>>  Abs (const Vec<T,I> &value)
	{
		return glm::abs( value );
	}

/*
=================================================
	Floor/Ceil/Fract
=================================================
*/
	template <typename T, int I>
	ND_ forceinline EnableIf<IsScalar<T> and IsFloatPoint<T>, Vec<T,I>>  Floor (const Vec<T,I> &value)
	{
		return glm::floor( value );
	}

	template <typename T, int I>
	ND_ forceinline EnableIf<IsScalar<T> and IsFloatPoint<T>, Vec<T,I>>  Ceil (const Vec<T,I> &value)
	{
		return glm::ceil( value );
	}

	template <typename T, int I>
	ND_ forceinline EnableIf<IsScalar<T> and IsFloatPoint<T>, Vec<T,I>>  Fract (const Vec<T,I> &value)
	{
		return glm::fract( value );
	}

/*
=================================================
	Lerp
=================================================
*/
	template <typename T, int I>
	ND_ forceinline EnableIf<IsScalar<T>, Vec<T,I>>  Lerp (const Vec<T,I> &lhs, const Vec<T,I> &rhs, const T factor)
	{
		return glm::mix( lhs, rhs, factor );
	}
	
/*
=================================================
	Dot
=================================================
*/
	template <typename T, int I>
	ND_ forceinline EnableIf<IsScalar<T>, T>  Dot (const Vec<T,I> &lhs, const Vec<T,I> &rhs)
	{
		return glm::dot( lhs, rhs );
	}

/*
=================================================
	Cross
=================================================
*/
	template <typename T>
	ND_ forceinline EnableIf<IsScalar<T>, Vec<T,3>>  Cross (const Vec<T,3> &lhs, const Vec<T,3> &rhs)
	{
		return glm::cross( lhs, rhs );
	}

/*
=================================================
	Equals
=================================================
*/
	template <typename T, int I>
	ND_ forceinline EnableIf<IsScalar<T>, Vec<bool,I>>  Equals (const Vec<T,I> &lhs, const Vec<T,I> &rhs, const T &err = std::numeric_limits<T>::epsilon())
	{
		Vec<bool,I>	res;
		for (int i = 0; i < I; ++i) {
			res[i] = Equals( lhs[i], rhs[i], err );
		}
		return res;
	}
	
/*
=================================================
	Min / Mag
=================================================
*/
	template <typename T, int I>
	ND_ forceinline Vec<T,I>  Min (const Vec<T,I> &lhs, const Vec<T,I> &rhs)
	{
		return glm::min( lhs, rhs );
	}
	
	template <typename T, int I>
	ND_ forceinline Vec<T,I>  Max (const Vec<T,I> &lhs, const Vec<T,I> &rhs)
	{
		return glm::max( lhs, rhs );
	}
	
/*
=================================================
	Clamp
=================================================
*/
	template <typename T, int I>
	ND_ forceinline Vec<T,I>  Clamp (const Vec<T,I> &value, const Vec<T,I> &minVal, const Vec<T,I> &maxVal)
	{
		return glm::clamp( value, minVal, maxVal );
	}

/*
=================================================
	Wrap
=================================================
*/
	template <typename T, int I>
	ND_ forceinline EnableIf<IsFloatPoint<T>, Vec<T,I>>  Wrap (const Vec<T,I>& v, const T& minValue, const T& maxValue)
	{
		Vec<T,I>	res;
		for (int i = 0; i < I; ++i) {
			res[i] = Wrap( v[i], minValue, maxValue );
		}
		return res;
	}

/*
=================================================
	Round / RoundToInt / RoundToUint
=================================================
*/
	template <typename T, int I>
	ND_ forceinline EnableIf<IsScalar<T> and IsFloatPoint<T>, T>  Round (const Vec<T,I>& v)
	{
		Vec<T,I>	res;
		for (int i = 0; i < I; ++i) {
			res[i] = Round( v[i] );
		}
		return res;
	}
	
	template <typename T, int I>
	ND_ forceinline auto  RoundToInt (const Vec<T,I>& v)
	{
		using R = decltype(RoundToInt(T()));

		Vec<R,I>	res;
		for (int i = 0; i < I; ++i) {
			res[i] = RoundToInt( v[i] );
		}
		return res;
	}
	
	template <typename T, int I>
	ND_ forceinline auto  RoundToUint (const Vec<T,I>& v)
	{
		using R = decltype(RoundToUint(T()));

		Vec<R,I>	res;
		for (int i = 0; i < I; ++i) {
			res[i] = RoundToUint( v[i] );
		}
		return res;
	}
	
/*
=================================================
	Length
=================================================
*/
	template <typename T, int I>
	ND_ forceinline EnableIf<IsScalar<T> and IsFloatPoint<T>, T>  Length (const Vec<T,I> &v)
	{
		return glm::length( v );
	}
	
	template <typename T, int I>
	ND_ forceinline EnableIf<IsScalar<T> and IsFloatPoint<T>, T>  LengthSqr (const Vec<T,I> &v)
	{
		return glm::length2( v );
	}
	
/*
=================================================
	Distance
=================================================
*/
	template <typename T, int I>
	ND_ forceinline EnableIf<IsScalar<T> and IsFloatPoint<T>, T>  Distance (const Vec<T,I> &lhs, const Vec<T,I> &rhs)
	{
		return glm::distance( lhs, rhs );
	}
	
	template <typename T, int I>
	ND_ forceinline EnableIf<IsScalar<T> and IsFloatPoint<T>, T>  DistanceSqr (const Vec<T,I> &lhs, const Vec<T,I> &rhs)
	{
		return glm::distance2( lhs, rhs );
	}
	
/*
=================================================
	Normalize
=================================================
*/
	template <typename T, int I>
	ND_ forceinline EnableIf<IsScalar<T> and IsFloatPoint<T>, Vec<T,I>>  Normalize (const Vec<T,I> &v)
	{
		return glm::normalize( v );
	}


}	// AE::Math


namespace std
{
#if AE_FAST_HASH
	template <typename T, uint32_t I>
	struct hash< AE::Math::Vec<T,I> > {
		ND_ size_t  operator () (const AE::Math::Vec<T,I> &value) const {
			return size_t(AE::STL::HashOf( value.data(), value.size() * sizeof(T) ));
		}
	};

#else
	template <typename T>
	struct hash< AE::Math::Vec<T,2> > {
		ND_ size_t  operator () (const AE::Math::Vec<T,2> &value) const {
			return size_t(AE::STL::HashOf( value.x ) + AE::STL::HashOf( value.y ));
		}
	};
	
	template <typename T>
	struct hash< AE::Math::Vec<T,3> > {
		ND_ size_t  operator () (const AE::Math::Vec<T,3> &value) const {
			return size_t(AE::STL::HashOf( value.x ) + AE::STL::HashOf( value.y ) + AE::STL::HashOf( value.z ));
		}
	};
	
	template <typename T>
	struct hash< AE::Math::Vec<T,4> > {
		ND_ size_t  operator () (const AE::Math::Vec<T,4> &value) const {
			return size_t(AE::STL::HashOf( value.x ) + AE::STL::HashOf( value.y ) + AE::STL::HashOf( value.z ) + AE::STL::HashOf( value.w ));
		}
	};
#endif

}	// std
