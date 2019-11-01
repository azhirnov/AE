// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Math/Math.h"
#include "stl/CompileTime/TypeTraits.h"
#include "stl/Math/GLM.h"

namespace AE::STL
{

	template <typename T, uint I>
	using Vec = glm::vec< I, T >;
	

	using bool2		= Vec< bool, 2 >;
	using bool3		= Vec< bool, 3 >;
	using bool4		= Vec< bool, 4 >;
	
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
	namespace _ae_stl_hidden_
	{
		template <typename T>
		struct _IsVec {
			static constexpr bool	value = false;
		};

		template <typename T, uint I>
		struct _IsVec< Vec<T,I> > {
			static constexpr bool	value = true;
		};
	}

	template <typename T>
	static constexpr bool  IsVec = _ae_stl_hidden_::_IsVec<T>::value;
	
/*
=================================================
	VecSize
=================================================
*/
	namespace _ae_stl_hidden_
	{
		template <typename T>
		struct _VecSize {
		};

		template <typename T, uint I>
		struct _VecSize< Vec<T,I> > {
			static constexpr uint	value = I;
		};
	}

	template <typename T>
	static constexpr uint  VecSize = _ae_stl_hidden_::_VecSize<T>::value;
	
/*
=================================================
	operator !
=================================================
*/
	template <typename T, uint I>
	ND_ forceinline Vec<T,I>  operator ! (const Vec<T,I> &value)
	{
		Vec<T,I>	res;
		for (uint i = 0; i < I; ++i) {
			res[i] = !value[i];
		}
		return res;
	}
	
/*
=================================================
	operator *
=================================================
*/
	template <typename T, uint I, typename S>
	ND_ forceinline EnableIf<IsScalar<S> and not IsSameTypes<T,S>, Vec<T,I>>  operator * (const Vec<T,I> &lhs, const S &rhs)
	{
		return lhs * T(rhs);
	}

/*
=================================================
	operator %
=================================================
*/
	/*template <typename T, uint I>
	ND_ forceinline EnableIf<IsFloatPoint<T>, Vec<T,I>>  operator % (const Vec<T,I> &lhs, const Vec<T,I> &rhs)
	{
		return glm::mod( lhs, rhs );
	}*/
	
	template <typename T, uint I>
	ND_ forceinline EnableIf<IsFloatPoint<T>, Vec<T,I>&>  operator %= (Vec<T,I> &lhs, const Vec<T,I> &rhs)
	{
		return (lhs = glm::mod( lhs, rhs ));
	}
/*
=================================================
	All
=================================================
*/
	ND_ forceinline bool  All (const bool2 &v)
	{
		return v.x & v.y;
	}

	ND_ forceinline bool  All (const bool3 &v)
	{
		return v.x & v.y & v.z;
	}

	ND_ forceinline bool  All (const bool4 &v)
	{
		return v.x & v.y & v.z & v.w;
	}
	
/*
=================================================
	Any
=================================================
*/
	ND_ forceinline bool  Any (const bool2 &v)
	{
		return v.x | v.y;
	}

	ND_ forceinline bool  Any (const bool3 &v)
	{
		return v.x | v.y | v.z;
	}

	ND_ forceinline bool  Any (const bool4 &v)
	{
		return v.x | v.y | v.z | v.w;
	}

/*
=================================================
	Abs
=================================================
*/
	template <typename T, uint I>
	ND_ forceinline Vec<T,I>  Abs (const Vec<T,I> &value)
	{
		return glm::abs( value );
	}

/*
=================================================
	Floor
=================================================
*/
	template <typename T, uint I>
	ND_ forceinline EnableIf<IsFloatPoint<T>, Vec<T,I>>  Floor (const Vec<T,I> &value)
	{
		return glm::floor( value );
	}

/*
=================================================
	Ceil
=================================================
*/
	template <typename T, uint I>
	ND_ forceinline EnableIf<IsFloatPoint<T>, Vec<T,I>>  Ceil (const Vec<T,I> &value)
	{
		return glm::ceil( value );
	}

/*
=================================================
	Fract
=================================================
*/
	template <typename T, uint I>
	ND_ forceinline EnableIf<IsFloatPoint<T>, Vec<T,I>>  Fract (const Vec<T,I> &value)
	{
		return glm::fract( value );
	}

/*
=================================================
	Lerp
=================================================
*/
	template <typename T, uint I>
	ND_ forceinline Vec<T,I>  Lerp (const Vec<T,I> &lhs, const Vec<T,I> &rhs, const T factor)
	{
		return glm::mix( lhs, rhs, factor );
	}
	
/*
=================================================
	Dot
=================================================
*/
	template <typename T, uint I>
	ND_ forceinline T  Dot (const Vec<T,I> &lhs, const Vec<T,I> &rhs)
	{
		return glm::dot( lhs, rhs );
	}

/*
=================================================
	Cross
=================================================
*/
	template <typename T>
	ND_ forceinline Vec<T,3>  Cross (const Vec<T,3> &lhs, const Vec<T,3> &rhs)
	{
		return glm::cross( lhs, rhs );
	}

/*
=================================================
	Equals
=================================================
*/
	template <typename T, uint I>
	ND_ forceinline Vec<bool,I>  Equals (const Vec<T,I> &lhs, const Vec<T,I> &rhs, const T &err = std::numeric_limits<T>::epsilon())
	{
		Vec<bool,I>	res;
		for (uint i = 0; i < I; ++i) {
			res[i] = Equals( lhs[i], rhs[i], err );
		}
		return res;
	}

/*
=================================================
	Wrap
=================================================
*/
	template <typename T, uint I>
	ND_ forceinline EnableIf<IsFloatPoint<T>, Vec<T,I>>  Wrap (const Vec<T,I>& v, const T& minValue, const T& maxValue)
	{
		Vec<T,I>	res;
		for (uint i = 0; i < I; ++i) {
			res[i] = Wrap( v[i], minValue, maxValue );
		}
		return res;
	}

/*
=================================================
	Round / RoundToInt / RoundToUint
=================================================
*/
	template <typename T, uint I>
	ND_ forceinline constexpr EnableIf<IsScalar<T> and IsFloatPoint<T>, T>  Round (const Vec<T,I>& v)
	{
		Vec<T,I>	res;
		for (uint i = 0; i < I; ++i) {
			res[i] = Round( v[i] );
		}
		return res;
	}
	
	template <typename T, uint I>
	ND_ forceinline constexpr auto  RoundToInt (const Vec<T,I>& v)
	{
		using R = decltype(RoundToInt(T()));

		Vec<R,I>	res;
		for (uint i = 0; i < I; ++i) {
			res[i] = RoundToInt( v[i] );
		}
		return res;
	}
	
	template <typename T, uint I>
	ND_ forceinline constexpr auto  RoundToUint (const Vec<T,I>& v)
	{
		using R = decltype(RoundToUint(T()));

		Vec<R,I>	res;
		for (uint i = 0; i < I; ++i) {
			res[i] = RoundToUint( v[i] );
		}
		return res;
	}

}	// AE::STL


namespace std
{
#if AE_FAST_HASH
	template <typename T, uint I>
	struct hash< AE::STL::Vec<T,I> > {
		ND_ size_t  operator () (const AE::STL::Vec<T,I> &value) const {
			return size_t(AE::STL::HashOf( value.data(), value.size() * sizeof(T) ));
		}
	};

#else
	template <typename T>
	struct hash< AE::STL::Vec<T,2> > {
		ND_ size_t  operator () (const AE::STL::Vec<T,2> &value) const {
			return size_t(AE::STL::HashOf( value.x ) + AE::STL::HashOf( value.y ));
		}
	};
	
	template <typename T>
	struct hash< AE::STL::Vec<T,3> > {
		ND_ size_t  operator () (const AE::STL::Vec<T,3> &value) const {
			return size_t(AE::STL::HashOf( value.x ) + AE::STL::HashOf( value.y ) + AE::STL::HashOf( value.z ));
		}
	};
	
	template <typename T>
	struct hash< AE::STL::Vec<T,4> > {
		ND_ size_t  operator () (const AE::STL::Vec<T,4> &value) const {
			return size_t(AE::STL::HashOf( value.x ) + AE::STL::HashOf( value.y ) + AE::STL::HashOf( value.z ) + AE::STL::HashOf( value.w ));
		}
	};
#endif

}	// std
