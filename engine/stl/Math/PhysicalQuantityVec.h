// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Math/Vec.h"
#include "stl/Math/PhysicalQuantity.h"

namespace AE::Math
{

	//
	// Physical Quantity Vector
	//

	template <typename Quantity,
			  int VecLength
			 >
	struct PhysicalQuantityVec;
	
	template <int VecLength,
			  typename ValueType,
			  typename Dimension,
			  typename ValueScale
			 >
	using TPhysicalQuantityVec = PhysicalQuantityVec< PhysicalQuantity<ValueType, Dimension, ValueScale>, VecLength >;



	template <typename Quantity>
	struct PhysicalQuantityVec <Quantity,2> : Vec<Quantity, 2>
	{
	// types
		using Value_t		= typename Quantity::Value_t;
		using Scale_t		= typename Quantity::Scale_t;
		using Dimension_t	= typename Quantity::Dimension_t;
		using Self			= PhysicalQuantityVec< Quantity, 2 >;

	// methods
		constexpr PhysicalQuantityVec () {}
		
		template <typename S>
		constexpr PhysicalQuantityVec (const TPhysicalQuantityVec<2, Value_t, Dimension_t, S> &other) : Vec<Quantity, 2>{other.x, other.y} {}

		constexpr PhysicalQuantityVec (Value_t X, Value_t Y) : Vec<Quantity, 2>{X,Y} {}
		constexpr PhysicalQuantityVec (Quantity X, Quantity Y) : Vec<Quantity, 2>{X,Y} {}

		constexpr explicit PhysicalQuantityVec (Value_t V) : Vec<Quantity, 2>{V,V} {}
		constexpr explicit PhysicalQuantityVec (Quantity V) : Vec<Quantity, 2>{V,V} {}
		
		constexpr explicit PhysicalQuantityVec (const Vec<Value_t, 2> &V) : Vec<Quantity, 2>{V.x,V.y} {}

		ND_ constexpr explicit operator Vec<Value_t, 2> () const { return { this->x.GetNonScaled(), this->y.GetNonScaled() }; }
	};
	


	template <typename Quantity>
	struct PhysicalQuantityVec <Quantity,3> : Vec<Quantity, 3>
	{
	// types
		using Value_t		= typename Quantity::Value_t;
		using Scale_t		= typename Quantity::Scale_t;
		using Dimension_t	= typename Quantity::Dimension_t;
		using Self			= PhysicalQuantityVec< Quantity, 3 >;
		
	// methods
		constexpr PhysicalQuantityVec () {}
		
		template <typename S>
		constexpr PhysicalQuantityVec (const TPhysicalQuantityVec<3, Value_t, Dimension_t, S> &other) : Vec<Quantity, 3>{other.x, other.y, other.z} {}

		constexpr PhysicalQuantityVec (Value_t X, Value_t Y, Value_t Z) : Vec<Quantity, 3>{X,Y,Z} {}
		constexpr PhysicalQuantityVec (Quantity X, Quantity Y, Quantity Z) : Vec<Quantity, 3>{X,Y,Z} {}
		
		constexpr explicit PhysicalQuantityVec (Value_t V) : Vec<Quantity, 3>{V,V,V} {}
		constexpr explicit PhysicalQuantityVec (Quantity V) : Vec<Quantity, 3>{V,V,V} {}
		
		constexpr explicit PhysicalQuantityVec (const Vec<Value_t, 3> &V) : Vec<Quantity, 3>{V.x,V.y,V.z} {}
		
		ND_ constexpr explicit operator Vec<Value_t, 3> () const { return { this->x.GetNonScaled(), this->y.GetNonScaled(), this->z.GetNonScaled() }; }
	};
	


	template <typename Quantity>
	struct PhysicalQuantityVec <Quantity,4> : Vec<Quantity, 4>
	{
	// types
		using Value_t		= typename Quantity::Value_t;
		using Scale_t		= typename Quantity::Scale_t;
		using Dimension_t	= typename Quantity::Dimension_t;
		using Self			= PhysicalQuantityVec< Quantity, 4 >;
		
	// methods
		constexpr PhysicalQuantityVec () {}
		
		template <typename S>
		constexpr PhysicalQuantityVec (const TPhysicalQuantityVec<4, Value_t, Dimension_t, S> &other) : Vec<Quantity, 4>{other.x, other.y, other.z, other.w} {}

		constexpr PhysicalQuantityVec (Value_t X, Value_t Y, Value_t Z, Value_t W) : Vec<Quantity, 4>{X,Y,Z,W} {}
		constexpr PhysicalQuantityVec (Quantity X, Quantity Y, Quantity Z, Quantity W) : Vec<Quantity, 4>{X,Y,Z,W} {}
		
		constexpr explicit PhysicalQuantityVec (Value_t V) : Vec<Quantity, 4>{V,V,V,V} {}
		constexpr explicit PhysicalQuantityVec (Quantity V) : Vec<Quantity, 4>{V,V,V,V} {}
		
		constexpr explicit PhysicalQuantityVec (const Vec<Value_t, 4> &V) : Vec<Quantity, 4>{V.x,V.y,V.z,V.w} {}
		
		ND_ constexpr explicit operator Vec<Value_t, 4> () const { return { this->x.GetNonScaled(), this->y.GetNonScaled(), this->z.GetNonScaled(), this->w.GetNonScaled() }; }
	};
	


namespace _fgc_hidden_
{
/*
=================================================
	PhysicalQuantityVec_GetNonScaled
=================================================
*/
	template <int VecLength, typename ValueType, typename Dimension, typename Scale>
	ND_ constexpr Vec<ValueType, VecLength>
		PhysicalQuantityVec_GetNonScaled (const TPhysicalQuantityVec<VecLength, ValueType, Dimension, Scale> &value)
	{
		Vec<ValueType, VecLength>	ret;
		for (int i = 0; i < VecLength; ++i) {
			ret[i] = value[i].GetNonScaled();
		}
		return ret;
	}
	
/*
=================================================
	PhysicalQuantityVec_GetScaled
=================================================
*/
	template <int VecLength, typename ValueType, typename Dimension, typename Scale>
	ND_ constexpr Vec<ValueType, VecLength>
		PhysicalQuantityVec_GetScaled (const TPhysicalQuantityVec<VecLength, ValueType, Dimension, Scale> &value)
	{
		Vec<ValueType, VecLength>	ret;
		for (int i = 0; i < VecLength; ++i) {
			ret[i] = value[i].GetScaled();
		}
		return ret;
	}
	
/*
=================================================
	PhysicalQuantityVec_ToScale
=================================================
*/
	template <typename NewScale, int VecLength, typename ValueType, typename Dimension, typename Scale>
	ND_ constexpr Vec<ValueType, VecLength>
		PhysicalQuantityVec_ToScale (const TPhysicalQuantityVec<VecLength, ValueType, Dimension, Scale> &value)
	{
		const auto	scale = Scale::Value / NewScale::Value;

		Vec<ValueType, VecLength>	ret;
		for (int i = 0; i < VecLength; ++i) {
			ret[i] = value[i].GetNonScaled() * scale;
		}
		return ret;
	}

}	// _fgc_hidden_
	

/*
=================================================
	operator + (vec, vec)
=================================================
*/
	template <int VecLength, typename ValueType, typename Dimension, typename LhsScale, typename RhsScale
			 >
	ND_ constexpr auto  operator + (const TPhysicalQuantityVec<VecLength, ValueType, Dimension, LhsScale> &lhs,
									const TPhysicalQuantityVec<VecLength, ValueType, Dimension, RhsScale> &rhs)
	{
		using Scale = ValueScaleTempl::template Add< LhsScale, RhsScale >;
		using Type  = PhysicalQuantity< ValueType, Dimension, Scale >;
		
		PhysicalQuantityVec<Type, VecLength>	ret;
		for (int i = 0; i < VecLength; ++i) {
			ret[i] = Type{Scale::Get( lhs[i].GetNonScaled(), rhs[i].GetNonScaled() )};
		}
		return ret;
	}

/*
=================================================
	operator + (vec, scalar)
=================================================
*/
	template <int VecLength, typename ValueType, typename Dimension, typename LhsScale, typename RhsScale
			 >
	ND_ constexpr auto  operator + (const TPhysicalQuantityVec<VecLength, ValueType, Dimension, LhsScale> &lhs,
									PhysicalQuantity<ValueType, Dimension, RhsScale> rhs)
	{
		using Scale = ValueScaleTempl::template Add< LhsScale, RhsScale >;
		using Type  = PhysicalQuantity< ValueType, Dimension, Scale >;
		
		PhysicalQuantityVec<Type, VecLength>	ret;
		for (int i = 0; i < VecLength; ++i) {
			ret[i] = Type{Scale::Get( lhs[i].GetNonScaled(), rhs.GetNonScaled() )};
		}
		return ret;
	}

/*
=================================================
	operator + (scalar, vec)
=================================================
*/
	template <int VecLength, typename ValueType, typename Dimension, typename LhsScale, typename RhsScale
			 >
	ND_ constexpr auto  operator + (PhysicalQuantity<ValueType, Dimension, RhsScale> lhs,
									const TPhysicalQuantityVec<VecLength, ValueType, Dimension, LhsScale> &rhs)
	{
		using Scale = ValueScaleTempl::template Add< LhsScale, RhsScale >;
		using Type  = PhysicalQuantity< ValueType, Dimension, Scale >;
		
		PhysicalQuantityVec<Type, VecLength>	ret;
		for (int i = 0; i < VecLength; ++i) {
			ret[i] = Type{Scale::Get( lhs.GetNonScaled(), rhs[i].GetNonScaled() )};
		}
		return ret;
	}
	
/*
=================================================
	operator - (vec, vec)
=================================================
*/
	template <int VecLength, typename ValueType, typename Dimension, typename LhsScale, typename RhsScale
			 >
	ND_ constexpr auto  operator - (const TPhysicalQuantityVec<VecLength, ValueType, Dimension, LhsScale> &lhs,
									const TPhysicalQuantityVec<VecLength, ValueType, Dimension, RhsScale> &rhs)
	{
		using Scale = ValueScaleTempl::template Sub< LhsScale, RhsScale >;
		using Type  = PhysicalQuantity< ValueType, Dimension, Scale >;
		
		PhysicalQuantityVec<Type, VecLength>	ret;
		for (int i = 0; i < VecLength; ++i) {
			ret[i] = Type{Scale::Get( lhs[i].GetNonScaled(), rhs[i].GetNonScaled() )};
		}
		return ret;
	}

/*
=================================================
	operator - (vec, scalar)
=================================================
*/
	template <int VecLength, typename ValueType, typename Dimension, typename LhsScale, typename RhsScale
			 >
	ND_ constexpr auto  operator - (const TPhysicalQuantityVec<VecLength, ValueType, Dimension, LhsScale> &lhs,
									PhysicalQuantity<ValueType, Dimension, RhsScale> rhs)
	{
		using Scale = ValueScaleTempl::template Sub< LhsScale, RhsScale >;
		using Type  = PhysicalQuantity< ValueType, Dimension, Scale >;
		
		PhysicalQuantityVec<Type, VecLength>	ret;
		for (int i = 0; i < VecLength; ++i) {
			ret[i] = Type{Scale::Get( lhs[i].GetNonScaled(), rhs.GetNonScaled() )};
		}
		return ret;
	}

/*
=================================================
	operator - (scalar, vec)
=================================================
*/
	template <int VecLength, typename ValueType, typename Dimension, typename LhsScale, typename RhsScale
			 >
	ND_ constexpr auto  operator - (PhysicalQuantity<ValueType, Dimension, RhsScale> lhs,
									const TPhysicalQuantityVec<VecLength, ValueType, Dimension, LhsScale> &rhs)
	{
		using Scale = ValueScaleTempl::template Sub< LhsScale, RhsScale >;
		using Type  = PhysicalQuantity< ValueType, Dimension, Scale >;
		
		PhysicalQuantityVec<Type, VecLength>	ret;
		for (int i = 0; i < VecLength; ++i) {
			ret[i] = Type{Scale::Get( lhs.GetNonScaled(), rhs[i].GetNonScaled() )};
		}
		return ret;
	}
	
/*
=================================================
	operator * (vec, vec)
=================================================
*/
	template <int VecLength, typename ValueType, typename LhsDim, typename LhsScale, typename RhsDim, typename RhsScale
			 >
	ND_ constexpr auto  operator * (const TPhysicalQuantityVec<VecLength, ValueType, LhsDim, LhsScale> &lhs,
									const TPhysicalQuantityVec<VecLength, ValueType, RhsDim, RhsScale> &rhs)
	{
		using Scale = ValueScaleTempl::template Mul< LhsScale, RhsScale >;
		using Type  = PhysicalQuantity< ValueType, typename LhsDim::template Mul<RhsDim>, Scale >;
		
		PhysicalQuantityVec<Type, VecLength>	ret;
		for (int i = 0; i < VecLength; ++i) {
			ret[i] = Type{Scale::Get( lhs[i].GetNonScaled(), rhs[i].GetNonScaled() )};
		}
		return ret;
	}
	
	template <int VecLength, typename ValueType, typename Dimension, typename Scale>
	ND_ constexpr auto  operator * (const Vec<ValueType, VecLength> &lhs,
									const TPhysicalQuantityVec<VecLength, ValueType, Dimension, Scale> &rhs)
	{
		TPhysicalQuantityVec<VecLength, ValueType, Dimension, Scale>	ret = rhs;
		for (int i = 0; i < VecLength; ++i) {
			ret[i] *= lhs[i];
		}
		return ret;
	}
	
	template <int VecLength, typename ValueType, typename Dimension, typename Scale>
	ND_ constexpr auto  operator * (const TPhysicalQuantityVec<VecLength, ValueType, Dimension, Scale> &lhs,
									const Vec<ValueType, VecLength> &rhs)
	{
		TPhysicalQuantityVec<VecLength, ValueType, Dimension, Scale>	ret = lhs;
		for (int i = 0; i < VecLength; ++i) {
			ret[i] *= rhs[i];
		}
		return ret;
	}

/*
=================================================
	operator * (vec, scalar)
=================================================
*/
	template <int VecLength, typename ValueType, typename LhsDim, typename LhsScale, typename RhsDim, typename RhsScale
			 >
	ND_ constexpr auto  operator * (const TPhysicalQuantityVec<VecLength, ValueType, LhsDim, LhsScale> &lhs,
									PhysicalQuantity<ValueType, RhsDim, RhsScale> rhs)
	{
		using Scale = ValueScaleTempl::template Mul< LhsScale, RhsScale >;
		using Type  = PhysicalQuantity< ValueType, typename LhsDim::template Mul<RhsDim>, Scale >;
		
		PhysicalQuantityVec<Type, VecLength>	ret;
		for (int i = 0; i < VecLength; ++i) {
			ret[i] = Type{Scale::Get( lhs[i].GetNonScaled(), rhs.GetNonScaled() )};
		}
		return ret;
	}
	
	template <int VecLength, typename ValueType, typename Dimension, typename Scale>
	ND_ constexpr auto  operator * (const Vec<ValueType, VecLength> &lhs,
									PhysicalQuantity<ValueType, Dimension, Scale> rhs)
	{
		TPhysicalQuantityVec<VecLength, ValueType, Dimension, Scale>	ret{ rhs };
		for (int i = 0; i < VecLength; ++i) {
			ret[i] *= lhs[i];
		}
		return ret;
	}

/*
=================================================
	operator * (scalar, vec)
=================================================
*/
	template <int VecLength, typename ValueType, typename LhsDim, typename LhsScale, typename RhsDim, typename RhsScale
			 >
	ND_ constexpr auto  operator * (PhysicalQuantity<ValueType, LhsDim, RhsScale> lhs,
									const TPhysicalQuantityVec<VecLength, ValueType, RhsDim, LhsScale> &rhs)
	{
		using Scale = ValueScaleTempl::template Mul< LhsScale, RhsScale >;
		using Type  = PhysicalQuantity< ValueType, typename LhsDim::template Mul<RhsDim>, Scale >;
		
		PhysicalQuantityVec<Type, VecLength>	ret;
		for (int i = 0; i < VecLength; ++i) {
			ret[i] = Type{Scale::Get( lhs.GetNonScaled(), rhs[i].GetNonScaled() )};
		}
		return ret;
	}
	
	template <int VecLength, typename ValueType, typename Dimension, typename Scale>
	ND_ constexpr auto  operator * (PhysicalQuantity<ValueType, Dimension, Scale> lhs,
									const Vec<ValueType, VecLength> &rhs)
	{
		TPhysicalQuantityVec<VecLength, ValueType, Dimension, Scale>	ret{ lhs };
		for (int i = 0; i < VecLength; ++i) {
			ret[i] *= rhs[i];
		}
		return ret;
	}

/*
=================================================
	operator / (vec, vec)
=================================================
*/
	template <int VecLength, typename ValueType, typename LhsDim, typename LhsScale, typename RhsDim, typename RhsScale
			 >
	ND_ constexpr auto  operator / (const TPhysicalQuantityVec<VecLength, ValueType, LhsDim, LhsScale> &lhs,
									const TPhysicalQuantityVec<VecLength, ValueType, RhsDim, RhsScale> &rhs)
	{
		using Scale = ValueScaleTempl::template Div< LhsScale, RhsScale >;
		using Type  = PhysicalQuantity< ValueType, typename LhsDim::template Div<RhsDim>, Scale >;
		
		PhysicalQuantityVec<Type, VecLength>	ret;
		for (int i = 0; i < VecLength; ++i) {
			ret[i] = Type{Scale::Get( lhs[i].GetNonScaled(), rhs[i].GetNonScaled() )};
		}
		return ret;
	}
	
	template <int VecLength, typename ValueType, typename Dimension, typename Scale>
	ND_ constexpr auto  operator / (const Vec<ValueType, VecLength> &lhs,
									const TPhysicalQuantityVec<VecLength, ValueType, Dimension, Scale> &rhs)
	{
		TPhysicalQuantityVec<VecLength, ValueType, Dimension, Scale>	ret{ lhs };
		for (int i = 0; i < VecLength; ++i) {
			ret[i] /= rhs[i];
		}
		return ret;
	}
	
	template <int VecLength, typename ValueType, typename Dimension, typename Scale>
	ND_ constexpr auto  operator / (const TPhysicalQuantityVec<VecLength, ValueType, Dimension, Scale> &lhs,
									const Vec<ValueType, VecLength> &rhs)
	{
		TPhysicalQuantityVec<VecLength, ValueType, Dimension, Scale>	ret{ lhs };
		for (int i = 0; i < VecLength; ++i) {
			ret[i] /= rhs[i];
		}
		return ret;
	}

/*
=================================================
	operator / (vec, scalar)
=================================================
*/
	template <int VecLength, typename ValueType, typename LhsDim, typename LhsScale, typename RhsDim, typename RhsScale
			 >
	ND_ constexpr auto  operator / (const TPhysicalQuantityVec<VecLength, ValueType, LhsDim, LhsScale> &lhs,
									PhysicalQuantity<ValueType, RhsDim, RhsScale> rhs)
	{
		using Scale = ValueScaleTempl::template Div< LhsScale, RhsScale >;
		using Type  = PhysicalQuantity< ValueType, typename LhsDim::template Div<RhsDim>, Scale >;
		
		PhysicalQuantityVec<Type, VecLength>	ret;
		for (int i = 0; i < VecLength; ++i) {
			ret[i] = Type{Scale::Get( lhs[i].GetNonScaled(), rhs.GetNonScaled() )};
		}
		return ret;
	}
	
	template <int VecLength, typename ValueType, typename Dimension, typename Scale>
	ND_ constexpr auto  operator / (const Vec<ValueType, VecLength> &lhs,
									PhysicalQuantity<ValueType, Dimension, Scale> rhs)
	{
		TPhysicalQuantityVec<VecLength, ValueType, Dimension, Scale>	ret{ lhs };
		for (int i = 0; i < VecLength; ++i) {
			ret[i] /= rhs[i];
		}
		return ret;
	}

/*
=================================================
	operator / (scalar, vec)
=================================================
*/
	template <int VecLength, typename ValueType, typename LhsDim, typename LhsScale, typename RhsDim, typename RhsScale
			 >
	ND_ constexpr auto  operator / (PhysicalQuantity<ValueType, LhsDim, RhsScale> lhs,
									const TPhysicalQuantityVec<VecLength, ValueType, RhsDim, LhsScale> &rhs)
	{
		using Scale = ValueScaleTempl::template Div< LhsScale, RhsScale >;
		using Type  = PhysicalQuantity< ValueType, typename LhsDim::template Div<RhsDim>, Scale >;
		
		PhysicalQuantityVec<Type, VecLength>	ret;
		for (int i = 0; i < VecLength; ++i) {
			ret[i] = Type{Scale::Get( lhs.GetNonScaled(), rhs[i].GetNonScaled() )};
		}
		return ret;
	}
	
	template <int VecLength, typename ValueType, typename Dimension, typename Scale>
	ND_ constexpr auto  operator / (PhysicalQuantity<ValueType, Dimension, Scale> lhs,
									const Vec<ValueType, VecLength> &rhs)
	{
		TPhysicalQuantityVec<VecLength, ValueType, Dimension, Scale>	ret{ lhs };
		for (int i = 0; i < VecLength; ++i) {
			ret[i] /= rhs[i];
		}
		return ret;
	}

/*
=================================================
	Normalize
=================================================
*/
	template <int VecLength, typename ValueType, typename Dimension, typename Scale>
	ND_ constexpr auto  Normalize (const TPhysicalQuantityVec<VecLength, ValueType, Dimension, Scale> &value)
	{
		using namespace _fgc_hidden_;
		return PhysicalQuantity<ValueType, Dimension, Scale>{ Normalize( PhysicalQuantityVec_GetNonScaled( value ))};
	}

/*
=================================================
	Length
=================================================
*/
	template <int VecLength, typename ValueType, typename Dimension, typename Scale>
	ND_ constexpr auto  Length (const TPhysicalQuantityVec<VecLength, ValueType, Dimension, Scale> &value)
	{
		using namespace _fgc_hidden_;
		return PhysicalQuantity<ValueType, Dimension, Scale>{ Length( PhysicalQuantityVec_GetNonScaled( value ))};
	}
	
/*
=================================================
	LengthSqr
=================================================
*/
	template <int VecLength, typename ValueType, typename Dimension, typename Scale>
	ND_ constexpr auto  LengthSqr (const TPhysicalQuantityVec<VecLength, ValueType, Dimension, Scale> &value)
	{
		using namespace _fgc_hidden_;
		using DstScale = ValueScaleTempl::template Pow< Scale, 2 >;
		return PhysicalQuantity<ValueType, Dimension, DstScale>{ LengthSqr( PhysicalQuantityVec_GetNonScaled( value ))};
	}

/*
=================================================
	Dot
=================================================
*/
	template <int VecLength, typename ValueType, typename Dimension, typename LhsScale, typename RhsScale
			 >
	ND_ constexpr auto  Dot (const TPhysicalQuantityVec<VecLength, ValueType, Dimension, LhsScale> &lhs,
							 const TPhysicalQuantityVec<VecLength, ValueType, Dimension, RhsScale> &rhs)
	{
		using namespace _fgc_hidden_;
		using Scale = ValueScaleTempl::template Mul< LhsScale, RhsScale >;
		using Type  = PhysicalQuantity< ValueType, Dimension, Scale >;

		return Type{ Dot( PhysicalQuantityVec_GetNonScaled( lhs ), PhysicalQuantityVec_GetNonScaled( rhs ) )};
	}

/*
=================================================
	Cross
=================================================
*/
	template <typename ValueType, typename Dimension, typename LhsScale, typename RhsScale
			 >
	ND_ constexpr auto  Cross (const TPhysicalQuantityVec<3, ValueType, Dimension, LhsScale> &lhs,
							   const TPhysicalQuantityVec<3, ValueType, Dimension, RhsScale> &rhs)
	{
		using namespace _fgc_hidden_;
		using Scale = ValueScaleTempl::template Mul< LhsScale, RhsScale >;
		using Type  = PhysicalQuantity< ValueType, Dimension, Scale >;

		return PhysicalQuantityVec<Type,3>{ Cross( PhysicalQuantityVec_GetNonScaled( lhs ), PhysicalQuantityVec_GetNonScaled( rhs ) )};
	}
	
/*
=================================================
	Distance
=================================================
*/
	template <int VecLength, typename ValueType, typename Dimension, typename LhsScale, typename RhsScale
			 >
	ND_ constexpr auto  Distance (const TPhysicalQuantityVec<VecLength, ValueType, Dimension, LhsScale> &lhs,
								  const TPhysicalQuantityVec<VecLength, ValueType, Dimension, RhsScale> &rhs)
	{
		using namespace _fgc_hidden_;
		using Scale = ValueScaleTempl::template Add< LhsScale, RhsScale >;
		using Type  = PhysicalQuantity< ValueType, Dimension, Scale >;

		return Type{ Distance( PhysicalQuantityVec_ToScale< Scale >( lhs ),
								PhysicalQuantityVec_ToScale< Scale >( rhs ) )};
	}
	
/*
=================================================
	DistanceSqr
=================================================
*/
	template <int VecLength, typename ValueType, typename Dimension, typename LhsScale, typename RhsScale
			 >
	ND_ constexpr auto  DistanceSqr (const TPhysicalQuantityVec<VecLength, ValueType, Dimension, LhsScale> &lhs,
								   const TPhysicalQuantityVec<VecLength, ValueType, Dimension, RhsScale> &rhs)
	{
		using namespace _fgc_hidden_;
		using Scale = ValueScaleTempl::template Add< LhsScale, RhsScale >;
		using Type	= PhysicalQuantity< ValueType, Dimension, ValueScaleTempl::template Pow< Scale, 2 > >;
		
		return Type{ DistanceSqr( PhysicalQuantityVec_ToScale< Scale >( lhs ),
								  PhysicalQuantityVec_ToScale< Scale >( rhs ) )};
	}
	
/*
=================================================
	Min
=================================================
*/
	template <int VecLength, typename ValueType, typename Dimension, typename Scale>
	ND_ constexpr auto  Min (const TPhysicalQuantityVec<VecLength, ValueType, Dimension, Scale> &lhs,
							 const TPhysicalQuantityVec<VecLength, ValueType, Dimension, Scale> &rhs)
	{
		using Type = PhysicalQuantity< ValueType, Dimension, Scale >;

		PhysicalQuantityVec<Type, VecLength>	ret;
		for (int i = 0; i < VecLength; ++i) {
			ret[i] = Type{ Min( lhs[i].GetNonScaled(), rhs[i].GetNonScaled() )};
		}
		return ret;
	}
	
/*
=================================================
	Max
=================================================
*/
	template <int VecLength, typename ValueType, typename Dimension, typename Scale>
	ND_ constexpr auto  Max (const TPhysicalQuantityVec<VecLength, ValueType, Dimension, Scale> &lhs,
							 const TPhysicalQuantityVec<VecLength, ValueType, Dimension, Scale> &rhs)
	{
		using Type = PhysicalQuantity< ValueType, Dimension, Scale >;

		PhysicalQuantityVec<Type, VecLength>	ret;
		for (int i = 0; i < VecLength; ++i) {
			ret[i] = Type{ Max( lhs[i].GetNonScaled(), rhs[i].GetNonScaled() )};
		}
		return ret;
	}
	
/*
=================================================
	Floor
=================================================
*/
	template <int VecLength, typename ValueType, typename Dimension, typename Scale>
	ND_ constexpr auto  Floor (const TPhysicalQuantityVec<VecLength, ValueType, Dimension, Scale> &value)
	{
		using Type = PhysicalQuantity< ValueType, Dimension, Scale >;

		PhysicalQuantityVec<Type, VecLength>	ret;
		for (int i = 0; i < VecLength; ++i) {
			ret[i] = Type{ Floor( value[i].GetNonScaled() )};
		}
		return ret;
	}
	
/*
=================================================
	Ceil
=================================================
*/
	template <int VecLength, typename ValueType, typename Dimension, typename Scale>
	ND_ constexpr auto  Ceil (const TPhysicalQuantityVec<VecLength, ValueType, Dimension, Scale> &value)
	{
		using Type = PhysicalQuantity< ValueType, Dimension, Scale >;

		PhysicalQuantityVec<Type, VecLength>	ret;
		for (int i = 0; i < VecLength; ++i) {
			ret[i] = Type{ Ceil( value[i].GetNonScaled() )};
		}
		return ret;
	}


}	// AE::Math
