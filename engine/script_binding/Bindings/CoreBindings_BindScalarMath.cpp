// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#include "script_binding/Bindings/CoreBindings.h"
#include "script_binding/Impl/ClassBinder.h"
#include "script_binding/Impl/ScriptEngine.inl.h"

#include "stl/Math/BitMath.h"
#include "stl/Math/Radians.h"

namespace FGScript
{

/*
=================================================
	ScalarFunc
=================================================
*/
	struct ScalarFunc
	{
		template <typename T>	static T Abs (T value)						{ return FGC::Abs( value ); }
		template <typename T>	static T Floor (T value)					{ return FGC::Floor( value ); }
		template <typename T>	static T Ceil (T value)						{ return FGC::Ceil( value ); }
		template <typename T>	static T Trunc (T value)					{ return FGC::Trunc( value ); }
		template <typename T>	static T Fract (T value)					{ return FGC::Fract( value ); }

		template <typename T>	static T Round (T value)					{ return FGC::Round( value ); }
		template <typename T>	static auto RoundToInt (T value)			{ return FGC::RoundToInt( value ); }
		template <typename T>	static auto RoundToUint (T value)			{ return FGC::RoundToUint( value ); }

		template <typename T>	static T AlignToSmaller (T value, T align)	{ return FGC::AlignToSmaller( value. align ); }
		template <typename T>	static T AlignToLarger (T value, T align)	{ return FGC::AlignToLarger( value. align ); }

		template <typename T>	static T SafeDiv (T lhs, T rhs, T def)		{ return FGC::SafeDiv( lhs. rhs, def ); }
		
		template <typename T>	static bool All (T value)					{ return FGC::All( value ); }
		template <typename T>	static bool Any (T value)					{ return FGC::Any( value ); }

		template <typename T>	static T Pow (T x, T y)						{ return std::pow( x, y ); }
		template <typename T>	static T Ln (T x)							{ return FGC::Ln( x ); }
		template <typename T>	static T Log (T x, T base)					{ return std::log( x ) / std::log( base ); }
		template <typename T>	static T Log2 (T x)							{ return FGC::Log2( x ); }
		template <typename T>	static T Log10 (T x)						{ return FGC::Log10( x ); }
		template <typename T>	static T Exp (T x)							{ return std::exp( x ); }
		template <typename T>	static T Exp2 (T x)							{ return std::pow( T(2), x ); }
		template <typename T>	static T Exp10 (T x)						{ return std::pow( T(10), x ); }

		template <typename T>	static int IntLog2 (T x)					{ return FGC::IntLog2( x ); }
		template <typename T>	static int BitScanReverse (T x)				{ return FGC::BitScanReverse( x ); }
		template <typename T>	static int BitScanForward (T x)				{ return FGC::BitScanForward( x ); }
		template <typename T>	static uint BitCount (T x)					{ using U = ToUnsignedInteger<T>; return uint(FGC::BitCount( U(x) )); }
		template <typename T>	static bool IsPowerOfTwo (T x)				{ return FGC::IsPowerOfTwo( x ); }

		template <typename T>	static T Sin (T value)						{ return FGC::Sin( RadiansTempl<T>{value} ); }
		template <typename T>	static T SinH (T value)						{ return FGC::SinH( RadiansTempl<T>{value} ); }
		template <typename T>	static T ASin (T value)						{ return T(FGC::ASin( value )); }
		template <typename T>	static T Cos (T value)						{ return FGC::Cos( RadiansTempl<T>{value} ); }
		template <typename T>	static T CosH (T value)						{ return FGC::CosH( RadiansTempl<T>{value} ); }
		template <typename T>	static T ACos (T value)						{ return T(FGC::ACos( value )); }
		template <typename T>	static T Tan (T value)						{ return FGC::Tan( RadiansTempl<T>{value} ); }
		template <typename T>	static T TanH (T value)						{ return FGC::TanH( RadiansTempl<T>{value} ); }
		template <typename T>	static T ATan (T y, T x)					{ return T(FGC::ATan( y, x )); }

		template <typename T>	static T Square (T value)					{ return FGC::Square( value ); }
		template <typename T>	static T Sqrt (T value)						{ return FGC::Sqrt( value ); }
		template <typename T>	static T Mod (T x, T y)						{ return std::fmod( x, y ); }

		template <typename T>	static T Lerp (T x, T y, T factor)			{ return FGC::Lerp( x, y, factor ); }

		template <typename T>	static T Min (T x, T y)						{ return FGC::Min( x, y ); }
		template <typename T>	static T Max (T x, T y)						{ return FGC::Max( x, y ); }
		template <typename T>	static T Clamp (T x, T min, T max)			{ return FGC::Clamp( x, min, max ); }
		template <typename T>	static T Wrap (T x, T min, T max)			{ return FGC::Wrap( x, min, max ); }
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
	

}	// FGScript
