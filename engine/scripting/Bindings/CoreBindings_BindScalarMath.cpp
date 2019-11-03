// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#include "scripting/Bindings/CoreBindings.h"
#include "scripting/Impl/ClassBinder.h"
#include "scripting/Impl/ScriptEngine.inl.h"

#include "stl/Math/BitMath.h"
#include "stl/Math/Radians.h"

namespace AE::Scripting
{

/*
=================================================
	ScalarFunc
=================================================
*/
	struct ScalarFunc
	{
		template <typename T>	static T Abs (T value)						{ return AE::STL::Abs( value ); }
		template <typename T>	static T Floor (T value)					{ return AE::STL::Floor( value ); }
		template <typename T>	static T Ceil (T value)						{ return AE::STL::Ceil( value ); }
		template <typename T>	static T Trunc (T value)					{ return AE::STL::Trunc( value ); }
		template <typename T>	static T Fract (T value)					{ return AE::STL::Fract( value ); }

		template <typename T>	static T Round (T value)					{ return AE::STL::Round( value ); }
		template <typename T>	static auto RoundToInt (T value)			{ return AE::STL::RoundToInt( value ); }
		template <typename T>	static auto RoundToUint (T value)			{ return AE::STL::RoundToUint( value ); }

		template <typename T>	static T AlignToSmaller (T value, T align)	{ return AE::STL::AlignToSmaller( value. align ); }
		template <typename T>	static T AlignToLarger (T value, T align)	{ return AE::STL::AlignToLarger( value. align ); }

		template <typename T>	static T SafeDiv (T lhs, T rhs, T def)		{ return AE::STL::SafeDiv( lhs. rhs, def ); }
		
		template <typename T>	static bool All (T value)					{ return AE::STL::All( value ); }
		template <typename T>	static bool Any (T value)					{ return AE::STL::Any( value ); }

		template <typename T>	static T Pow (T x, T y)						{ return std::pow( x, y ); }
		template <typename T>	static T Ln (T x)							{ return AE::STL::Ln( x ); }
		template <typename T>	static T Log (T x, T base)					{ return std::log( x ) / std::log( base ); }
		template <typename T>	static T Log2 (T x)							{ return AE::STL::Log2( x ); }
		template <typename T>	static T Log10 (T x)						{ return AE::STL::Log10( x ); }
		template <typename T>	static T Exp (T x)							{ return std::exp( x ); }
		template <typename T>	static T Exp2 (T x)							{ return std::pow( T(2), x ); }
		template <typename T>	static T Exp10 (T x)						{ return std::pow( T(10), x ); }

		template <typename T>	static int IntLog2 (T x)					{ return AE::STL::IntLog2( x ); }
		template <typename T>	static int BitScanReverse (T x)				{ return AE::STL::BitScanReverse( x ); }
		template <typename T>	static int BitScanForward (T x)				{ return AE::STL::BitScanForward( x ); }
		template <typename T>	static uint BitCount (T x)					{ using U = ToUnsignedInteger<T>; return uint(AE::STL::BitCount( U(x) )); }
		template <typename T>	static bool IsPowerOfTwo (T x)				{ return AE::STL::IsPowerOfTwo( x ); }

		template <typename T>	static T Sin (T value)						{ return AE::STL::Sin( RadiansTempl<T>{value} ); }
		template <typename T>	static T SinH (T value)						{ return AE::STL::SinH( RadiansTempl<T>{value} ); }
		template <typename T>	static T ASin (T value)						{ return T(AE::STL::ASin( value )); }
		template <typename T>	static T Cos (T value)						{ return AE::STL::Cos( RadiansTempl<T>{value} ); }
		template <typename T>	static T CosH (T value)						{ return AE::STL::CosH( RadiansTempl<T>{value} ); }
		template <typename T>	static T ACos (T value)						{ return T(AE::STL::ACos( value )); }
		template <typename T>	static T Tan (T value)						{ return AE::STL::Tan( RadiansTempl<T>{value} ); }
		template <typename T>	static T TanH (T value)						{ return AE::STL::TanH( RadiansTempl<T>{value} ); }
		template <typename T>	static T ATan (T y, T x)					{ return T(AE::STL::ATan( y, x )); }

		template <typename T>	static T Square (T value)					{ return AE::STL::Square( value ); }
		template <typename T>	static T Sqrt (T value)						{ return AE::STL::Sqrt( value ); }
		template <typename T>	static T Mod (T x, T y)						{ return std::fmod( x, y ); }

		template <typename T>	static T Lerp (T x, T y, T factor)			{ return AE::STL::Lerp( x, y, factor ); }

		template <typename T>	static T Min (T x, T y)						{ return AE::STL::Min( x, y ); }
		template <typename T>	static T Max (T x, T y)						{ return AE::STL::Max( x, y ); }
		template <typename T>	static T Clamp (T x, T min, T max)			{ return AE::STL::Clamp( x, min, max ); }
		template <typename T>	static T Wrap (T x, T min, T max)			{ return AE::STL::Wrap( x, min, max ); }
	};
	
/*
=================================================
	BindBoolScalar
=================================================
*/
	template <typename T>
	static void BindBoolScalar (const ScriptEnginePtr &)
	{
	}
	
/*
=================================================
	BindIntScalar
=================================================
*/
	template <typename T>
	static void BindIntScalar (const ScriptEnginePtr &se)
	{
		se->AddFunction( &ScalarFunc::IntLog2<T>,			"IntLog2" );
		se->AddFunction( &ScalarFunc::BitScanReverse<T>,	"BitScanReverse" );
		se->AddFunction( &ScalarFunc::BitScanForward<T>,	"BitScanForward" );
		se->AddFunction( &ScalarFunc::BitCount<T>,			"BitCount" );
		se->AddFunction( &ScalarFunc::IsPowerOfTwo<T>,		"IsPowerOfTwo" );
		
	}
	
/*
=================================================
	BindFloatScalar
=================================================
*/
	template <typename T>
	static void BindFloatScalar (const ScriptEnginePtr &se)
	{
		const String	suffix = IsSameTypes<T,float> ? "" : "D";
		
		//
		se->AddFunction( &ScalarFunc::Floor<T>,		"Floor" );
		se->AddFunction( &ScalarFunc::Ceil<T>,		"Ceil" );
		se->AddFunction( &ScalarFunc::Trunc<T>,		"Trunc" );
		se->AddFunction( &ScalarFunc::Fract<T>,		"Fract" );
		se->AddFunction( &ScalarFunc::Round<T>,		"Round" );
		se->AddFunction( &ScalarFunc::Mod<T>,		"Mod" );
		se->AddFunction( &ScalarFunc::Wrap<T>,		"Wrap" );
		
		//
		se->AddFunction( &ScalarFunc::Sqrt<T>,		"Sqrt" );
		se->AddFunction( &ScalarFunc::Pow<T>,		"Pow" );
		se->AddFunction( &ScalarFunc::Ln<T>,		"Ln" );
		se->AddFunction( &ScalarFunc::Log<T>,		"Log" );
		se->AddFunction( &ScalarFunc::Log2<T>,		"Log2" );
		se->AddFunction( &ScalarFunc::Log10<T>,		"Log10" );
		se->AddFunction( &ScalarFunc::Exp<T>,		"Exp" );
		se->AddFunction( &ScalarFunc::Exp2<T>,		"Exp2" );
		se->AddFunction( &ScalarFunc::Exp10<T>,		"Exp10" );

		// trigonometry
		se->AddFunction( &ScalarFunc::Sin<T>,		"Sin" );
		se->AddFunction( &ScalarFunc::SinH<T>,		"SinH" );
		se->AddFunction( &ScalarFunc::ASin<T>,		"ASin" );
		se->AddFunction( &ScalarFunc::Cos<T>,		"Cos" );
		se->AddFunction( &ScalarFunc::CosH<T>,		"CosH" );
		se->AddFunction( &ScalarFunc::ACos<T>,		"ACos" );
		se->AddFunction( &ScalarFunc::Tan<T>,		"Tan" );
		se->AddFunction( &ScalarFunc::TanH<T>,		"TanH" );
		se->AddFunction( &ScalarFunc::ATan<T>,		"ATan" );

		// interpolation
		se->AddFunction( &ScalarFunc::Lerp<T>,		"Lerp" );
	}
	
/*
=================================================
	BindIntFloatScalar
=================================================
*/
	template <typename T>
	static void BindIntFloatScalar (const ScriptEnginePtr &se)
	{
		if constexpr( IsSignedInteger<T> or IsFloatPoint<T> )
		{
			se->AddFunction( &ScalarFunc::Abs<T>,	"Abs" );
		}

		se->AddFunction( &ScalarFunc::Square<T>,	"Square" );

		se->AddFunction( &ScalarFunc::Min<T>,		"Min" );
		se->AddFunction( &ScalarFunc::Max<T>,		"Max" );
		se->AddFunction( &ScalarFunc::Clamp<T>,		"Clamp" );
	}

/*
=================================================
	BindScalarMath
=================================================
*/
	void CoreBindings::BindScalarMath (const ScriptEnginePtr &se)
	{
		BindBoolScalar<bool>( se );

		BindIntFloatScalar<int>( se );
		BindIntScalar<int>( se );
		
		BindIntFloatScalar<uint>( se );
		BindIntScalar<uint>( se );
		
		BindIntFloatScalar<float>( se );
		BindFloatScalar<float>( se );

		//se->AddConstProperty( Pi<real>,	"Pi" );
	}
	

}	// AE::Scripting
