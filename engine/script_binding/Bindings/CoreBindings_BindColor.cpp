// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#include "script_binding/Bindings/CoreBindings.h"
#include "script_binding/Impl/ClassBinder.h"
#include "script_binding/Impl/ScriptEngine.inl.h"

namespace AE::Script
{
	
/*
=================================================
	constructors
=================================================
*/
	template <typename T>
	static void RGBAColor_Ctor1 (void *mem, T val)
	{
		new( mem ) RGBAColor<T>( val );
	}

	template <typename T>
	static void RGBAColor_Ctor4 (void *mem, T r, T g, T b, T a)
	{
		new( mem ) RGBAColor<T>( r, g, b, a );
	}
	
	static void RGBA32f_Ctor_HSV_Alpha (void *mem, const HSVColor &c, float alpha)
	{
		new( mem ) RGBA32f( c, alpha );
	}

	template <typename T>
	static void RGBA32f_Ctor_RGBA (void *mem, const RGBAColor<T> &c)
	{
		new( mem ) RGBA32f( c );
	}
	
	template <typename T>
	static void RGBA32i_Ctor_RGBA (void *mem, const RGBAColor<T> &c)
	{
		new( mem ) RGBA32i( c );
	}
	
	template <typename T>
	static void RGBA32u_Ctor_RGBA (void *mem, const RGBAColor<T> &c)
	{
		new( mem ) RGBA32u( c );
	}
	
	template <typename T>
	static void RGBA8u_Ctor_RGBA (void *mem, const RGBAColor<T> &c)
	{
		new( mem ) RGBA8u( c );
	}

/*
=================================================
	BindRGBAColor
=================================================
*/
	template <typename T>
	static void BindRGBAColor (const ScriptEnginePtr &se)
	{
		ClassBinder<T>	binder{ se };

		binder.AddProperty( &T::r, "r" );
		binder.AddProperty( &T::g, "g" );
		binder.AddProperty( &T::b, "b" );
		binder.AddProperty( &T::a, "a" );

		binder.AddConstructor( &RGBAColor_Ctor1< typename T::value_type > );
		binder.AddConstructor( &RGBAColor_Ctor4< typename T::value_type > );

		//binder.AddMethod( static_cast< T (*)(const T&, const T&) >(&Min) );
		//binder.AddMethod( static_cast< T (*)(const T&, const T&) >(&Max) );
		//binder.AddMethod( static_cast< T (*)(const T&, const T&, const T&) >(&Clamp) );

		binder.Operators()
			.Equals( &T::operator== );

		if constexpr( IsSameTypes< T, RGBA32f > )
		{
			binder.AddConstructor( &RGBA32f_Ctor_HSV_Alpha );
			binder.AddConstructor( &RGBA32f_Ctor_RGBA<uint8_t> );
			binder.AddConstructor( &RGBA32f_Ctor_RGBA<uint> );
		}
		if constexpr( IsSameTypes< T, RGBA32i > )
		{
			binder.AddConstructor( &RGBA32i_Ctor_RGBA<uint8_t> );
			binder.AddConstructor( &RGBA32i_Ctor_RGBA<uint> );
		}
		if constexpr( IsSameTypes< T, RGBA32u > )
		{
			binder.AddConstructor( &RGBA32u_Ctor_RGBA<uint8_t> );
			binder.AddConstructor( &RGBA32u_Ctor_RGBA<int> );
		}
		if constexpr( IsSameTypes< T, RGBA8u > )
		{
			binder.AddConstructor( &RGBA8u_Ctor_RGBA<int> );
			binder.AddConstructor( &RGBA8u_Ctor_RGBA<uint> );
			binder.AddConstructor( &RGBA8u_Ctor_RGBA<float> );
			binder.AddMethodFromGlobal( static_cast< RGBA8u (*)(const RGBA8u&, const RGBA8u&, float) >(&Lerp), "Lerp" );
		}
	}

/*
=================================================
	constructors
=================================================
*/
	static void HSVColor_Ctor1 (void *mem, float h)
	{
		new( mem ) HSVColor( h );
	}
	
	static void HSVColor_Ctor2 (void *mem, float h, float s)
	{
		new( mem ) HSVColor( h, s );
	}
	
	static void HSVColor_Ctor3 (void *mem, float h, float s, float v)
	{
		new( mem ) HSVColor( h, s, v );
	}

	static void HSVColor_Ctor_RGBA32f (void *mem, const RGBA32f &c)
	{
		new( mem ) HSVColor( c );
	}

/*
=================================================
	BindHSVColor
=================================================
*/
	static void BindHSVColor (const ScriptEnginePtr &se)
	{
		ClassBinder<HSVColor>	binder{ se };

		binder.AddProperty( &HSVColor::h, "h" );
		binder.AddProperty( &HSVColor::s, "s" );
		binder.AddProperty( &HSVColor::v, "v" );
		
		binder.AddConstructor( &HSVColor_Ctor1 );
		binder.AddConstructor( &HSVColor_Ctor2 );
		binder.AddConstructor( &HSVColor_Ctor3 );
		binder.AddConstructor( &HSVColor_Ctor_RGBA32f );

		binder.Operators()
			.Equals( &HSVColor::operator== );
	}
	
/*
=================================================
	constructors
=================================================
*/
	static void DepthStencil_Ctor (void *mem, float d, int s)
	{
		new( mem ) DepthStencil( d, s );
	}

/*
=================================================
	BindDepthStencil
=================================================
*/
	static void BindDepthStencil (const ScriptEnginePtr &se)
	{
		ClassBinder<DepthStencil>	binder{ se };

		binder.AddProperty( &DepthStencil::depth,	"depth" );
		binder.AddProperty( &DepthStencil::stencil,	"stencil" );

		binder.AddConstructor( &DepthStencil_Ctor );

		binder.Operators()
			.Equals( &HSVColor::operator== );
	}

/*
=================================================
	BindColor
=================================================
*/
	void CoreBindings::BindColor (const ScriptEnginePtr &se)
	{
		BindRGBAColor<RGBA32f>( se );
		BindRGBAColor<RGBA32i>( se );
		BindRGBAColor<RGBA32u>( se );
		BindRGBAColor<RGBA8u>( se );
		BindHSVColor( se );
		BindDepthStencil( se );
	}

}	// AE::Script
