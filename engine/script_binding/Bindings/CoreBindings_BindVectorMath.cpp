// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#include "script_binding/Bindings/CoreBindings.h"
#include "script_binding/Impl/ClassBinder.h"
#include "script_binding/Impl/ScriptEngine.inl.h"

namespace FGScript
{

	template <typename T>
	struct InitVecFields
	{};

/*
=================================================
	BaseVecCtors
=================================================
*/
	template <typename T>
	struct BaseVecCtors
	{
		static void Ctor1 (void *mem, typename T::value_type value)
		{
			new( mem ) T( value );
		}
			
		static void Ctor2 (void *mem, Vec< typename T::value_type, 2 > value)
		{
			new( mem ) T( value );
		}
			
		static void Ctor3 (void *mem, Vec< typename T::value_type, 3 > value)
		{
			new( mem ) T( value );
		}
			
		static void Ctor4 (void *mem, Vec< typename T::value_type, 4 > value)
		{
			new( mem ) T( value );
		}
	};
		
/*
=================================================
	InitVecFields (Vec2)
=================================================
*/
	template <typename T>
	struct InitVecFields< Vec<T,2> > : BaseVecCtors< Vec<T,2> >
	{
		using Base_t = BaseVecCtors< Vec<T,2> >;

		static void CtorArg2 (void *mem, T x, T y)
		{
			new( mem ) Vec<T,2>( x, y );
		}
			
		static void Init (ClassBinder< Vec<T,2> > &binder)
		{
			using Vec_t		= Vec< T, 2 >;
			using Value_t	= T;

			binder.AddProperty( &Vec_t::x, "x" );
			binder.AddProperty( &Vec_t::y, "y" );

			binder.AddConstructor( &Base_t::Ctor1 );
			binder.AddConstructor( &Base_t::Ctor3 );
			binder.AddConstructor( &Base_t::Ctor4 );
			binder.AddConstructor( &CtorArg2 );
		}
	};
		
/*
=================================================
	InitVecFields (Vec3)
=================================================
*/
	template <typename T>
	struct InitVecFields< Vec<T,3> > : BaseVecCtors< Vec<T,3> >
	{
		using Base_t = BaseVecCtors< Vec<T,3> >;

		static void CtorArg2 (void *mem, const Vec<T,2> &xy, T z)
		{
			new( mem ) Vec<T,3>( xy, z );
		}

		static void CtorArg3 (void *mem, T x, T y, T z)
		{
			new( mem ) Vec<T,3>( x, y, z );
		}

		static void Init (ClassBinder< Vec<T,3> > &binder)
		{
			using Vec_t		= Vec< T, 3 >;
			using Value_t	= T;

			binder.AddProperty( &Vec_t::x, "x" );
			binder.AddProperty( &Vec_t::y, "y" );
			binder.AddProperty( &Vec_t::z, "z" );

			binder.AddConstructor( &Base_t::Ctor1 );
			binder.AddConstructor( &Base_t::Ctor2 );
			binder.AddConstructor( &Base_t::Ctor4 );
			binder.AddConstructor( &CtorArg2 );
			binder.AddConstructor( &CtorArg3 );
		}
	};
		
/*
=================================================
	InitVecFields (Vec4)
=================================================
*/
	template <typename T>
	struct InitVecFields< Vec<T,4> > : BaseVecCtors< Vec<T,4> >
	{
		using Base_t = BaseVecCtors< Vec<T,4> >;

		static void CtorArg2 (void *mem, const Vec<T,2> &xy, const Vec<T,2> &zw)
		{
			new( mem ) Vec<T,4>( xy, zw );
		}
			
		static void CtorArg3 (void *mem, const Vec<T,3> &xyz, T w)
		{
			new( mem ) Vec<T,4>( xyz, w );
		}

		static void CtorArg4 (void *mem, T x, T y, T z, T w)
		{
			new( mem ) Vec<T,4>( x, y, z, w );
		}

		static void Init (ClassBinder< Vec<T,4> > &binder)
		{
			using Vec_t		= Vec< T, 4 >;
			using Value_t	= T;

			binder.AddProperty( &Vec_t::x, "x" );
			binder.AddProperty( &Vec_t::y, "y" );
			binder.AddProperty( &Vec_t::z, "z" );
			binder.AddProperty( &Vec_t::w, "w" );

			binder.AddConstructor( &Base_t::Ctor1 );
			binder.AddConstructor( &Base_t::Ctor2 );
			binder.AddConstructor( &Base_t::Ctor3 );
			binder.AddConstructor( &CtorArg2 );
			binder.AddConstructor( &CtorArg3 );
			binder.AddConstructor( &CtorArg4 );
		}
	};
	
/*
=================================================
	VecFunc
=================================================
*/
	struct VecFunc
	{
		template <typename V>
		static bool Equal (const V &lhs, const V &rhs) {
			return FGC::All( lhs == rhs );
		}

		template <typename V>
		static int Cmp (const V &lhs, const V &rhs) {
			for ( size_t i = 0; i < V::size(); ++i ) {
				if ( lhs[i] > rhs[i] )	return +1;
				if ( lhs[i] < rhs[i] )	return -1;
			}
			return 0;
		}
		
		template <typename V> static V&  Add_a_v (V& lhs, const V &rhs)							{ return lhs += rhs; }
		template <typename V> static V&  Add_a_s (V& lhs, typename V::value_type rhs)			{ return lhs += rhs; }
		template <typename V> static V   Add_v_v (const V& lhs, const V &rhs)					{ return lhs + rhs; }
		template <typename V> static V   Add_v_s (const V &lhs, typename V::value_type rhs)		{ return lhs + rhs; }
		template <typename V> static V   Add_s_v (typename V::value_type lhs, const V &rhs)		{ return lhs + rhs; }
		
		template <typename V> static V&  Sub_a_v (V& lhs, const V &rhs)							{ return lhs -= rhs; }
		template <typename V> static V&  Sub_a_s (V& lhs, typename V::value_type rhs)			{ return lhs -= rhs; }
		template <typename V> static V   Sub_v_v (const V& lhs, const V &rhs)					{ return lhs - rhs; }
		template <typename V> static V   Sub_v_s (const V &lhs, typename V::value_type rhs)		{ return lhs - rhs; }
		template <typename V> static V   Sub_s_v (typename V::value_type lhs, const V &rhs)		{ return lhs - rhs; }
		
		template <typename V> static V&  Mul_a_v (V& lhs, const V &rhs)							{ return lhs *= rhs; }
		template <typename V> static V&  Mul_a_s (V& lhs, typename V::value_type rhs)			{ return lhs *= rhs; }
		template <typename V> static V   Mul_v_v (const V& lhs, const V &rhs)					{ return lhs * rhs; }
		template <typename V> static V   Mul_v_s (const V &lhs, typename V::value_type rhs)		{ return lhs * rhs; }
		template <typename V> static V   Mul_s_v (typename V::value_type lhs, const V &rhs)		{ return lhs * rhs; }
		
		template <typename V> static V&  Div_a_v (V& lhs, const V &rhs)							{ return lhs /= rhs; }
		template <typename V> static V&  Div_a_s (V& lhs, typename V::value_type rhs)			{ return lhs /= rhs; }
		template <typename V> static V   Div_v_v (const V& lhs, const V &rhs)					{ return lhs / rhs; }
		template <typename V> static V   Div_v_s (const V &lhs, typename V::value_type rhs)		{ return lhs / rhs; }
		template <typename V> static V   Div_s_v (typename V::value_type lhs, const V &rhs)		{ return lhs / rhs; }
		
		template <typename V> static V&  Mod_a_v (V& lhs, const V &rhs)							{ return lhs %= rhs; }
		template <typename V> static V&  Mod_a_s (V& lhs, typename V::value_type rhs)			{ return lhs %= rhs; }
		template <typename V> static V   Mod_v_v (const V& lhs, const V &rhs)					{ return lhs % rhs; }
		template <typename V> static V   Mod_v_s (const V &lhs, typename V::value_type rhs)		{ return lhs % rhs; }
		template <typename V> static V   Mod_s_v (typename V::value_type lhs, const V &rhs)		{ return lhs % rhs; }
		
		template <typename V> static V&  And_a_v (V& lhs, const V &rhs)							{ return lhs &= rhs; }
		template <typename V> static V&  And_a_s (V& lhs, typename V::value_type rhs)			{ return lhs &= rhs; }
		template <typename V> static V   And_v_v (const V& lhs, const V &rhs)					{ return lhs & rhs; }
		template <typename V> static V   And_v_s (const V &lhs, typename V::value_type rhs)		{ return lhs & rhs; }
		template <typename V> static V   And_s_v (typename V::value_type lhs, const V &rhs)		{ return lhs & rhs; }
		
		template <typename V> static V&  Or_a_v (V& lhs, const V &rhs)							{ return lhs |= rhs; }
		template <typename V> static V&  Or_a_s (V& lhs, typename V::value_type rhs)			{ return lhs |= rhs; }
		template <typename V> static V   Or_v_v (const V& lhs, const V &rhs)					{ return lhs | rhs; }
		template <typename V> static V   Or_v_s (const V &lhs, typename V::value_type rhs)		{ return lhs | rhs; }
		template <typename V> static V   Or_s_v (typename V::value_type lhs, const V &rhs)		{ return lhs | rhs; }
		
		template <typename V> static V&  Xor_a_v (V& lhs, const V &rhs)							{ return lhs ^= rhs; }
		template <typename V> static V&  Xor_a_s (V& lhs, typename V::value_type rhs)			{ return lhs ^= rhs; }
		template <typename V> static V   Xor_v_v (const V& lhs, const V &rhs)					{ return lhs ^ rhs; }
		template <typename V> static V   Xor_v_s (const V &lhs, typename V::value_type rhs)		{ return lhs ^ rhs; }
		template <typename V> static V   Xor_s_v (typename V::value_type lhs, const V &rhs)		{ return lhs ^ rhs; }
		
		template <typename V> static V&  ShiftL_a_v (V& lhs, const V &rhs)						{ return lhs <<= rhs; }
		template <typename V> static V&  ShiftL_a_s (V& lhs, typename V::value_type rhs)		{ return lhs <<= rhs; }
		template <typename V> static V   ShiftL_v_v (const V& lhs, const V &rhs)				{ return lhs << rhs; }
		template <typename V> static V   ShiftL_v_s (const V &lhs, typename V::value_type rhs)	{ return lhs << rhs; }
		template <typename V> static V   ShiftL_s_v (typename V::value_type lhs, const V &rhs)	{ return lhs << rhs; }
		
		template <typename V> static V&  ShiftR_a_v (V& lhs, const V &rhs)						{ return lhs >>= rhs; }
		template <typename V> static V&  ShiftR_a_s (V& lhs, typename V::value_type rhs)		{ return lhs >>= rhs; }
		template <typename V> static V   ShiftR_v_v (const V& lhs, const V &rhs)				{ return lhs >> rhs; }
		template <typename V> static V   ShiftR_v_s (const V &lhs, typename V::value_type rhs)	{ return lhs >> rhs; }
		template <typename V> static V   ShiftR_s_v (typename V::value_type lhs, const V &rhs)	{ return lhs >> rhs; }

		template <typename V> static bool All (const V &x)										{ return FGC::All( x ); }
		template <typename V> static bool Any (const V &x)										{ return FGC::Any( x ); }

		template <typename V>
		static typename V::value_type Dot (const V &x, const V &y) {
			return FGC::Dot( x, y );
		}

		template <typename V>
		static V Cross (const V &x, const V &y) {
			return FGC::Cross( x, y );
		}

		/*template <typename V>
		static V Sin (const V &x) {
			return GXMath::Sin( x.template To< RadiansVec<typename V::value_type, V::STATIC_COUNT> >() );
		}
		
		template <typename V>
		static V Cos (const V &x) {
			return GXMath::Cos( x.template To< RadiansVec<typename V::value_type, V::STATIC_COUNT> >() );
		}

		template <typename V>
		static V Tan (const V &x) {
			return GXMath::Tan( x.template To< RadiansVec<typename V::value_type, V::STATIC_COUNT> >() );
		}

		template <typename V>
		static V Square (const V &x) {
			return GXMath::Square( x );
		}

		template <typename V>
		static V Sqrt (const V &x) {
			return GXMath::Sqrt( x );
		}

		template <typename V>
		static V Lerp (const V &x, const V &y, typename V::value_type factor) {
			return GXMath::Lerp( x, y, factor );
		}*/
	};

/*
=================================================
	BindBoolVec
=================================================
*/
	template <typename T>
	static void BindBoolVec (ClassBinder<T> &binder, const ScriptEnginePtr &se)
	{
		using Vec_t		= T;
		using Value_t	= typename Vec_t::value_type;

		binder.Operators()
			.Unary( EUnaryOperator::Not, static_cast<T (*)(const T&) >(&operator !) )
			.Equals( &VecFunc::template Equal< Vec_t > )
			.Compare( &VecFunc::template Cmp< Vec_t > );

		se->AddFunction( &VecFunc::template All< Vec_t >,	"All" );
		se->AddFunction( &VecFunc::template Any< Vec_t >,	"Any" );

	}
	
/*
=================================================
	BindIntVec
=================================================
*/
	template <typename T>
	static void BindIntVec (ClassBinder<T> &binder, const ScriptEnginePtr &)
	{
		using Vec_t		= T;
		using Value_t	= typename Vec_t::value_type;

		binder.Operators()
			.BinaryAssign(	EBinaryOperator::Add, &VecFunc::template Add_a_v< Vec_t > )
			.BinaryAssign(	EBinaryOperator::Add, &VecFunc::template Add_a_s< Vec_t > )
			.Binary(		EBinaryOperator::Add, &VecFunc::template Add_v_v< Vec_t > )
			.Binary(		EBinaryOperator::Add, &VecFunc::template Add_v_s< Vec_t > )
			.BinaryRH(		EBinaryOperator::Add, &VecFunc::template Add_s_v< Vec_t > )
			
			.BinaryAssign(	EBinaryOperator::Sub, &VecFunc::template Sub_a_v< Vec_t > )
			.BinaryAssign(	EBinaryOperator::Sub, &VecFunc::template Sub_a_s< Vec_t > )
			.Binary(		EBinaryOperator::Sub, &VecFunc::template Sub_v_v< Vec_t > )
			.Binary(		EBinaryOperator::Sub, &VecFunc::template Sub_v_s< Vec_t > )
			.BinaryRH(		EBinaryOperator::Sub, &VecFunc::template Sub_s_v< Vec_t > )
			
			.BinaryAssign(	EBinaryOperator::Mul, &VecFunc::template Mul_a_v< Vec_t > )
			.BinaryAssign(	EBinaryOperator::Mul, &VecFunc::template Mul_a_s< Vec_t > )
			.Binary(		EBinaryOperator::Mul, &VecFunc::template Mul_v_v< Vec_t > )
			.Binary(		EBinaryOperator::Mul, &VecFunc::template Mul_v_s< Vec_t > )
			.BinaryRH(		EBinaryOperator::Mul, &VecFunc::template Mul_s_v< Vec_t > )
			
			.BinaryAssign(	EBinaryOperator::Div, &VecFunc::template Div_a_v< Vec_t > )
			.BinaryAssign(	EBinaryOperator::Div, &VecFunc::template Div_a_s< Vec_t > )
			.Binary(		EBinaryOperator::Div, &VecFunc::template Div_v_v< Vec_t > )
			.Binary(		EBinaryOperator::Div, &VecFunc::template Div_v_s< Vec_t > )
			.BinaryRH(		EBinaryOperator::Div, &VecFunc::template Div_s_v< Vec_t > )
			
			.BinaryAssign(	EBinaryOperator::Mod, &VecFunc::template Mod_a_v< Vec_t > )
			.BinaryAssign(	EBinaryOperator::Mod, &VecFunc::template Mod_a_s< Vec_t > )
			.Binary(		EBinaryOperator::Mod, &VecFunc::template Mod_v_v< Vec_t > )
			.Binary(		EBinaryOperator::Mod, &VecFunc::template Mod_v_s< Vec_t > )
			.BinaryRH(		EBinaryOperator::Mod, &VecFunc::template Mod_s_v< Vec_t > )
			
			.BinaryAssign(	EBinaryOperator::And, &VecFunc::template And_a_v< Vec_t > )
			.BinaryAssign(	EBinaryOperator::And, &VecFunc::template And_a_s< Vec_t > )
			.Binary(		EBinaryOperator::And, &VecFunc::template And_v_v< Vec_t > )
			.Binary(		EBinaryOperator::And, &VecFunc::template And_v_s< Vec_t > )
			.BinaryRH(		EBinaryOperator::And, &VecFunc::template And_s_v< Vec_t > )
			
			.BinaryAssign(	EBinaryOperator::Or,  &VecFunc::template Or_a_v< Vec_t > )
			.BinaryAssign(	EBinaryOperator::Or,  &VecFunc::template Or_a_s< Vec_t > )
			.Binary(		EBinaryOperator::Or,  &VecFunc::template Or_v_v< Vec_t > )
			.Binary(		EBinaryOperator::Or,  &VecFunc::template Or_v_s< Vec_t > )
			.BinaryRH(		EBinaryOperator::Or,  &VecFunc::template Or_s_v< Vec_t > )
			
			.BinaryAssign(	EBinaryOperator::Xor, &VecFunc::template Xor_a_v< Vec_t > )
			.BinaryAssign(	EBinaryOperator::Xor, &VecFunc::template Xor_a_s< Vec_t > )
			.Binary(		EBinaryOperator::Xor, &VecFunc::template Xor_v_v< Vec_t > )
			.Binary(		EBinaryOperator::Xor, &VecFunc::template Xor_v_s< Vec_t > )
			.BinaryRH(		EBinaryOperator::Xor, &VecFunc::template Xor_s_v< Vec_t > )
			
			.BinaryAssign(	EBinaryOperator::ShiftLeft, &VecFunc::template ShiftL_a_v< Vec_t > )
			.BinaryAssign(	EBinaryOperator::ShiftLeft, &VecFunc::template ShiftL_a_s< Vec_t > )
			.Binary(		EBinaryOperator::ShiftLeft, &VecFunc::template ShiftL_v_v< Vec_t > )
			.Binary(		EBinaryOperator::ShiftLeft, &VecFunc::template ShiftL_v_s< Vec_t > )
			.BinaryRH(		EBinaryOperator::ShiftLeft, &VecFunc::template ShiftL_s_v< Vec_t > )
			
			.BinaryAssign(	EBinaryOperator::ShiftRight, &VecFunc::template ShiftR_a_v< Vec_t > )
			.BinaryAssign(	EBinaryOperator::ShiftRight, &VecFunc::template ShiftR_a_s< Vec_t > )
			.Binary(		EBinaryOperator::ShiftRight, &VecFunc::template ShiftR_v_v< Vec_t > )
			.Binary(		EBinaryOperator::ShiftRight, &VecFunc::template ShiftR_v_s< Vec_t > )
			.BinaryRH(		EBinaryOperator::ShiftRight, &VecFunc::template ShiftR_s_v< Vec_t > )

			.Equals( &VecFunc::template Equal< Vec_t > )
			.Compare( &VecFunc::template Cmp< Vec_t > );
	}
	
/*
=================================================
	BindFloatVec
=================================================
*/
	template <typename T>
	static void BindFloatVec (ClassBinder<T> &binder, const ScriptEnginePtr &se)
	{
		using Vec_t		= T;
		using Value_t	= typename Vec_t::value_type;

		binder.Operators()
			.BinaryAssign(	EBinaryOperator::Add, &VecFunc::template Add_a_v< Vec_t > )
			.BinaryAssign(	EBinaryOperator::Add, &VecFunc::template Add_a_s< Vec_t > )
			.Binary(		EBinaryOperator::Add, &VecFunc::template Add_v_v< Vec_t > )
			.Binary(		EBinaryOperator::Add, &VecFunc::template Add_v_s< Vec_t > )
			.BinaryRH(		EBinaryOperator::Add, &VecFunc::template Add_s_v< Vec_t > )
			
			.BinaryAssign(	EBinaryOperator::Sub, &VecFunc::template Sub_a_v< Vec_t > )
			.BinaryAssign(	EBinaryOperator::Sub, &VecFunc::template Sub_a_s< Vec_t > )
			.Binary(		EBinaryOperator::Sub, &VecFunc::template Sub_v_v< Vec_t > )
			.Binary(		EBinaryOperator::Sub, &VecFunc::template Sub_v_s< Vec_t > )
			.BinaryRH(		EBinaryOperator::Sub, &VecFunc::template Sub_s_v< Vec_t > )
			
			.BinaryAssign(	EBinaryOperator::Mul, &VecFunc::template Mul_a_v< Vec_t > )
			.BinaryAssign(	EBinaryOperator::Mul, &VecFunc::template Mul_a_s< Vec_t > )
			.Binary(		EBinaryOperator::Mul, &VecFunc::template Mul_v_v< Vec_t > )
			.Binary(		EBinaryOperator::Mul, &VecFunc::template Mul_v_s< Vec_t > )
			.BinaryRH(		EBinaryOperator::Mul, &VecFunc::template Mul_s_v< Vec_t > )
			
			.BinaryAssign(	EBinaryOperator::Div, &VecFunc::template Div_a_v< Vec_t > )
			.BinaryAssign(	EBinaryOperator::Div, &VecFunc::template Div_a_s< Vec_t > )
			.Binary(		EBinaryOperator::Div, &VecFunc::template Div_v_v< Vec_t > )
			.Binary(		EBinaryOperator::Div, &VecFunc::template Div_v_s< Vec_t > )
			.BinaryRH(		EBinaryOperator::Div, &VecFunc::template Div_s_v< Vec_t > )
			
			.BinaryAssign(	EBinaryOperator::Mod, &VecFunc::template Mod_a_v< Vec_t > )
			.BinaryAssign(	EBinaryOperator::Mod, &VecFunc::template Mod_a_s< Vec_t > )
			.Binary(		EBinaryOperator::Mod, &VecFunc::template Mod_v_v< Vec_t > )
			.Binary(		EBinaryOperator::Mod, &VecFunc::template Mod_v_s< Vec_t > )
			.BinaryRH(		EBinaryOperator::Mod, &VecFunc::template Mod_s_v< Vec_t > )

			.Equals( &VecFunc::template Equal< Vec_t > )
			.Compare( &VecFunc::template Cmp< Vec_t > );

		if constexpr( Vec_t::size() == 3 ) {
			se->AddFunction( &VecFunc::template Cross<Vec_t>,	"Cross" );
		}
	}

/*
=================================================
	DefineVector_Func
=================================================
*/
	struct DefineVector_Func
	{
		ScriptEnginePtr	_se;

		explicit DefineVector_Func (const ScriptEnginePtr &se) : _se{se}
		{}

		template <typename T, size_t Index>
		void operator () ()
		{
			ClassBinder<T>	binder( _se );
			
			binder.CreateClassValue();
		}
	};
	
/*
=================================================
	BindVector_Func
=================================================
*/
	struct BindVector_Func
	{
		ScriptEnginePtr	_se;

		explicit BindVector_Func (const ScriptEnginePtr &se) : _se{se}
		{}

		template <typename T, size_t Index>
		void operator () ()
		{
			using Value_t = typename T::value_type;

			ClassBinder<T>	binder( _se );

			InitVecFields<T>::Init( binder );

			if constexpr( IsSameTypes< Value_t, bool > ) {
				BindBoolVec( binder, _se );
			} else
			if constexpr( IsInteger< Value_t > ) {
				BindIntVec( binder, _se );
			} else
			if constexpr( IsFloatPoint< Value_t > ) {
				BindFloatVec( binder, _se );
			}
		}
	};

/*
=================================================
	BindVectorMath
=================================================
*/
	void CoreBindings::BindVectorMath (const ScriptEnginePtr &se)
	{
		using VecTypes = TypeList<
							bool2, bool3, bool4,
							int2, int3, int4,
							uint2, uint3, uint4,
							float2, float3, float4,
							double2, double3, double4
						>;

		// declare
		{
			DefineVector_Func	func{ se };
		
			VecTypes::Visit( func );
		}

		// bind
		{
			BindVector_Func	func{ se };
		
			VecTypes::Visit( func );
		}
	}
	

}	// FGScript
