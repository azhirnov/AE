// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Config.h"


#ifdef COMPILER_MSVC
#	define and		&&
#	define or		||
#	define not		!
#endif


// mark output and input-output function arguments
#ifndef OUT
#	define OUT
#endif

#ifndef INOUT
#	define INOUT
#endif


// no discard
#ifndef ND_
# ifdef COMPILER_MSVC
#  if _MSC_VER >= 1917
#	define ND_				[[nodiscard]]
#  else
#	define ND_
#  endif
# endif	// COMPILER_MSVC

# ifdef COMPILER_CLANG
#  if __has_feature( cxx_attributes )
#	define ND_				[[nodiscard]]
#  else
#	define ND_
#  endif
# endif	// COMPILER_CLANG

# ifdef COMPILER_GCC
#  if __has_cpp_attribute( nodiscard )
#	define ND_				[[nodiscard]]
#  else
#	define ND_
#  endif
# endif	// COMPILER_GCC

#endif	// ND_


// null pointer
#ifndef null
#	define null				nullptr
#endif


// force inline
#ifndef forceinline
# if defined(AE_DEBUG) or defined(AE_DEVELOP)
#	define forceinline		inline

# elif defined(COMPILER_MSVC)
#	define forceinline		__forceinline

# elif defined(COMPILER_CLANG) or defined(COMPILER_GCC)
#	define forceinline		__inline__ __attribute__((always_inline))

# else
#	pragma warning ("forceinline is not supported")
#	define forceinline		inline
# endif
#endif


// debug break
#ifndef AE_PRIVATE_BREAK_POINT
# if defined(COMPILER_MSVC) and (defined(AE_DEBUG) or defined(AE_DEVELOP))
#	define AE_PRIVATE_BREAK_POINT()		__debugbreak()

# elif defined(PLATFORM_ANDROID) and defined(AE_DEBUG)
#	include <csignal>
#	define AE_PRIVATE_BREAK_POINT()		std::raise( SIGINT )

# elif (defined(COMPILER_CLANG) or defined(COMPILER_GCC)) and defined(AE_DEBUG)
#  if 1
#	include <exception>
#	define AE_PRIVATE_BREAK_POINT() 	throw std::runtime_error("breakpoint")
#  else
#	include <csignal>
#	define AE_PRIVATE_BREAK_POINT()		std::raise(SIGINT)
#  endif

# else
#	define AE_PRIVATE_BREAK_POINT()		{}
# endif
#endif


// exit
#ifndef AE_PRIVATE_EXIT
# if defined(PLATFORM_ANDROID)
#	define AE_PRIVATE_EXIT()	std::terminate()
# else
#	define AE_PRIVATE_EXIT()	::exit( EXIT_FAILURE )
# endif
#endif


// helper macro
#define AE_PRIVATE_GETARG_0( _0_, ... )				_0_
#define AE_PRIVATE_GETARG_1( _0_, _1_, ... )		_1_
#define AE_PRIVATE_GETARG_2( _0_, _1_, _2_, ... )	_2_
#define AE_PRIVATE_GETRAW( _value_ )				_value_
#define AE_PRIVATE_TOSTRING( ... )					#__VA_ARGS__
#define AE_PRIVATE_UNITE_RAW( _arg0_, _arg1_ )		AE_PRIVATE_UNITE( _arg0_, _arg1_ )
#define AE_PRIVATE_UNITE( _arg0_, _arg1_ )			_arg0_ ## _arg1_


// debug only check
#ifndef ASSERT
# ifdef AE_DEBUG
#	define ASSERT				CHECK
# else
#	define ASSERT( ... )		{}
# endif
#endif


// function name
#ifdef COMPILER_MSVC
#	define AE_FUNCTION_NAME		__FUNCTION__

#elif defined(COMPILER_CLANG) or defined(COMPILER_GCC)
#	define AE_FUNCTION_NAME		__func__

#else
#	define AE_FUNCTION_NAME		"unknown function"
#endif


// branch prediction optimization
#ifdef COMPILER_MSVC
# if _MSC_VER >= 1928
#	define AE_PRIVATE_HAS_CPP_ATTRIBS
# endif
#endif

#ifdef AE_PRIVATE_HAS_CPP_ATTRIBS
#	define if_likely( ... )		[[likely]] if ( __VA_ARGS__ )
#	define if_unlikely( ... )	[[unlikely]] if ( __VA_ARGS__ )

#elif defined(COMPILER_CLANG) or defined(COMPILER_GCC)
#	define if_likely( ... )		if ( __builtin_expect( !!(__VA_ARGS__), 1 ))
#	define if_unlikely( ... )	if ( __builtin_expect( !!(__VA_ARGS__), 0 ))
#else
	// not supported
#	define if_likely( ... )		if ( __VA_ARGS__ )
#	define if_unlikely( ... )	if ( __VA_ARGS__ )
#endif


// debug only scope
#ifndef DEBUG_ONLY
# if defined(AE_DEBUG)
#	define DEBUG_ONLY( ... )		__VA_ARGS__
# else
#	define DEBUG_ONLY( ... )
# endif
#endif


// log
// (text, file, line)
#ifndef AE_LOGD
# if defined(AE_DEBUG)
#	define AE_LOGD				AE_LOGI
# else
#	define AE_LOGD( ... )		{}
# endif
#endif

#ifndef AE_LOGI
#	define AE_LOGI( ... ) \
			AE_PRIVATE_LOGI( AE_PRIVATE_GETARG_0( __VA_ARGS__, "" ), \
							 AE_PRIVATE_GETARG_1( __VA_ARGS__, __FILE__ ), \
							 AE_PRIVATE_GETARG_2( __VA_ARGS__, __FILE__, __LINE__ ))
#endif

#ifndef AE_LOGE
#	define AE_LOGE( ... ) \
			AE_PRIVATE_LOGE( AE_PRIVATE_GETARG_0( __VA_ARGS__, "" ), \
							 AE_PRIVATE_GETARG_1( __VA_ARGS__, __FILE__ ), \
							 AE_PRIVATE_GETARG_2( __VA_ARGS__, __FILE__, __LINE__ ))
#endif


// check function return value
#ifndef CHECK
#	define AE_PRIVATE_CHECK( _expr_, _text_ ) \
		{if_likely (( _expr_ )) {} \
		 else { \
			AE_LOGE( _text_ ); \
		}}

#   define CHECK( ... ) \
		AE_PRIVATE_CHECK( (__VA_ARGS__), AE_PRIVATE_TOSTRING( __VA_ARGS__ ))
#endif


// check function return value and return error code
#ifndef CHECK_ERR
#	define AE_PRIVATE_CHECK_ERR( _expr_, _ret_ ) \
		{if_likely (( _expr_ )) {}\
		  else { \
			AE_LOGE( AE_PRIVATE_TOSTRING( _expr_ )); \
			return (_ret_); \
		}}

#	define CHECK_ERR( ... ) \
		AE_PRIVATE_CHECK_ERR( AE_PRIVATE_GETARG_0( __VA_ARGS__ ), AE_PRIVATE_GETARG_1( __VA_ARGS__, ::AE::STL::Default ))
#endif

#ifndef CHECK_ERRV
#	define CHECK_ERRV( _expr_ ) \
		AE_PRIVATE_CHECK_ERR( (_expr_), void() )
#endif


// check function return value and exit
#ifndef CHECK_FATAL
#	define CHECK_FATAL( ... ) \
		{if_likely (( __VA_ARGS__ )) {}\
		  else { \
			AE_LOGE( AE_PRIVATE_TOSTRING( __VA_ARGS__ )); \
			AE_PRIVATE_EXIT(); \
		}}
#endif


// return error code
#ifndef RETURN_ERR
#	define AE_PRIVATE_RETURN_ERR( _text_, _ret_ ) \
		{ AE_LOGE( _text_ );  return (_ret_); }

#	define RETURN_ERR( ... ) \
		AE_PRIVATE_RETURN_ERR( AE_PRIVATE_GETARG_0( __VA_ARGS__ ), AE_PRIVATE_GETARG_1( __VA_ARGS__, ::AE::STL::Default ))
#endif


// compile time assert
#ifndef STATIC_ASSERT
#	define STATIC_ASSERT( ... ) \
		static_assert(	AE_PRIVATE_GETRAW( AE_PRIVATE_GETARG_0( __VA_ARGS__ )), \
						AE_PRIVATE_GETRAW( AE_PRIVATE_GETARG_1( __VA_ARGS__, AE_PRIVATE_TOSTRING(__VA_ARGS__))) )
#endif


// bit operators
#define AE_BIT_OPERATORS( _type_ ) \
	ND_ constexpr _type_  operator |  (_type_ lhs, _type_ rhs)	{ return _type_( AE::STL::ToNearUInt(lhs) | AE::STL::ToNearUInt(rhs) ); } \
	ND_ constexpr _type_  operator &  (_type_ lhs, _type_ rhs)	{ return _type_( AE::STL::ToNearUInt(lhs) & AE::STL::ToNearUInt(rhs) ); } \
	\
	constexpr _type_&  operator |= (_type_ &lhs, _type_ rhs)	{ return lhs = _type_( AE::STL::ToNearUInt(lhs) | AE::STL::ToNearUInt(rhs) ); } \
	constexpr _type_&  operator &= (_type_ &lhs, _type_ rhs)	{ return lhs = _type_( AE::STL::ToNearUInt(lhs) & AE::STL::ToNearUInt(rhs) ); } \
	\
	ND_ constexpr _type_  operator ~ (_type_ lhs)				{ return _type_(~AE::STL::ToNearUInt(lhs)); } \
	ND_ constexpr bool   operator ! (_type_ lhs)				{ return not AE::STL::ToNearUInt(lhs); } \
	

// enable/disable checks for enums
#if defined(COMPILER_MSVC)
#	define BEGIN_ENUM_CHECKS() \
		__pragma (warning (push)) \
		__pragma (warning (error: 4061)) /*enumerator 'identifier' in switch of enum 'enumeration' is not explicitly handled by a case label*/ \
		__pragma (warning (error: 4062)) /*enumerator 'identifier' in switch of enum 'enumeration' is not handled*/ \
		__pragma (warning (error: 4063)) /*case 'number' is not a valid value for switch of enum 'type'*/ \

#	define END_ENUM_CHECKS() \
		__pragma (warning (pop)) \

#elif defined(COMPILER_CLANG)
#	define BEGIN_ENUM_CHECKS() \
		 _Pragma( "clang diagnostic error \"-Wswitch\"" )

#	define END_ENUM_CHECKS() \
		 _Pragma( "clang diagnostic ignored \"-Wswitch\"" )

#else
#	define BEGIN_ENUM_CHECKS()
#	define END_ENUM_CHECKS()

#endif


// allocator
#ifdef COMPILER_MSVC
#	define AE_ALLOCATOR		__declspec( allocator )
#else
#	define AE_ALLOCATOR
#endif


// thiscall, cdecl
#ifdef COMPILER_MSVC
#	define AE_CDECL		__cdecl
#	define AE_THISCALL	__thiscall

#elif defined(COMPILER_CLANG) or defined(COMPILER_GCC)
#	define AE_CDECL		//__attribute__((cdecl))
#	define AE_THISCALL	//__attribute__((thiscall))
#endif


// to fix compiler error C2338
#ifdef COMPILER_MSVC
#	define _ENABLE_EXTENDED_ALIGNED_STORAGE
#endif


// compile time messages
#ifndef AE_COMPILATION_MESSAGE
#	if defined(COMPILER_CLANG)
#		define AE_PRIVATE_MESSAGE_TOSTR(x)	#x
#		define AE_COMPILATION_MESSAGE( _message_ )	_Pragma(AE_PRIVATE_MESSAGE_TOSTR( GCC warning ("" _message_) ))

#	elif defined(COMPILER_MSVC)
#		define AE_COMPILATION_MESSAGE( _message_ )	__pragma(message( _message_ ))

#	else
#		define AE_COMPILATION_MESSAGE( _message_ )	// not supported
#	endif
#endif


// DLL import/export
#if not defined(AE_DLL_EXPORT) or not defined(AE_DLL_IMPORT)
# if defined(COMPILER_MSVC)
#	define AE_DLL_EXPORT			__declspec( dllexport )
#	define AE_DLL_IMPORT			__declspec( dllimport )

# elif defined(COMPILER_GCC) or defined(COMPILER_CLANG)
#  ifdef PLATFORM_WINDOWS
#	define AE_DLL_EXPORT			__attribute__ (dllexport)
#	define AE_DLL_IMPORT			__attribute__ (dllimport)
#  else
#	define AE_DLL_EXPORT			__attribute__((visibility("default")))
#	define AE_DLL_IMPORT			__attribute__((visibility("default")))
#  endif

# else
#	error define AE_DLL_EXPORT and AE_DLL_IMPORT for you compiler
# endif
#endif


// replace assertions by exceptions
#ifndef AE_NO_EXCEPTIONS

#	include <stdexcept>

#	undef  AE_PRIVATE_BREAK_POINT
#	define AE_PRIVATE_BREAK_POINT()	{}

#	undef  AE_LOGE
#	define AE_LOGE	AE_LOGI

	// keep ASSERT and CHECK behaviour because they may be used in destructor
	// but override CHECK_ERR, CHECK_FATAL and RETURN_ERR to allow user to handle this errors

#	undef  AE_PRIVATE_CHECK_ERR
#	define AE_PRIVATE_CHECK_ERR( _expr_, _ret_ ) \
		{if ( !(_expr_) ) { \
			throw AE::Exception{ AE_PRIVATE_TOSTRING( _expr_ )}; \
		}}

#	undef  CHECK_FATAL
#	define CHECK_FATAL( _expr_ ) \
		{if ( !(_expr_) ) { \
			throw AE::Exception{ AE_PRIVATE_TOSTRING( _expr_ )}; \
		}}

#	undef  AE_PRIVATE_RETURN_ERR
#	define AE_PRIVATE_RETURN_ERR( _text_, _ret_ ) \
		{throw AE::Exception{ _text_ };}

#endif


// setup for build on CI
#ifdef AE_CI_BUILD

#	undef  AE_PRIVATE_BREAK_POINT
#	define AE_PRIVATE_BREAK_POINT()	{}

#	undef  AE_PRIVATE_CHECK
#	define AE_PRIVATE_CHECK( _expr_, _text_ ) \
		{if ( !(_expr_) ) { \
			AE_LOGI( _text_ ); \
			AE_PRIVATE_EXIT(); \
		}}

#	undef  AE_PRIVATE_CHECK_ERR
#	define AE_PRIVATE_CHECK_ERR( _expr_, _ret_ ) \
		{if ( !(_expr_) ) { \
			AE_LOGI( AE_PRIVATE_TOSTRING( _expr_ )); \
			AE_PRIVATE_EXIT(); \
		}}

#	undef  CHECK_FATAL
#	define CHECK_FATAL( ... ) \
		{if ( !(__VA_ARGS__) ) { \
			AE_LOGI( AE_PRIVATE_TOSTRING( __VA_ARGS__ )); \
			AE_PRIVATE_EXIT(); \
		}}

#	undef  AE_PRIVATE_RETURN_ERR
#	define AE_PRIVATE_RETURN_ERR( _text_, _ret_ ) \
		{AE_LOGI( _text_ ); \
		 AE_PRIVATE_EXIT(); \
		}

# ifdef AE_DEBUG
#	undef  ASSERT
#	define ASSERT	CHECK
# endif

#	include <cassert>
#	undef  assert
#	define assert( ... ) \
		AE_PRIVATE_CHECK( (__VA_ARGS__), AE_PRIVATE_TOSTRING( __VA_ARGS__ ))

#endif	// AE_CI_BUILD


#if defined(AE_DEBUG)
#  if defined(AE_DEVELOP) or defined(AE_PROFILE) or defined(AE_RELEASE)
#	error only one configuration must be enabled!
#  endif
#elif defined(AE_DEVELOP)
#  if defined(AE_DEBUG) or defined(AE_PROFILE) or defined(AE_RELEASE)
#	error only one configuration must be enabled!
#  endif
#elif defined(AE_PROFILE)
#  if defined(AE_DEBUG) or defined(AE_DEVELOP) or defined(AE_RELEASE)
#	error only one configuration must be enabled!
#  endif
#elif defined(AE_RELEASE)
#  if defined(AE_DEBUG) or defined(AE_DEVELOP) or defined(AE_PROFILE)
#	error only one configuration must be enabled!
#  endif
#else
#	error unknown configuration!
#endif


// check definitions
#ifdef AE_CPP_DETECT_MISSMATCH

#  ifdef AE_DEBUG
#	pragma detect_mismatch( "AE_DEBUG", "1" )
#  else
#	pragma detect_mismatch( "AE_DEBUG", "0" )
#  endif

#  ifdef AE_DEVELOP
#	pragma detect_mismatch( "AE_DEVELOP", "1" )
#  else
#	pragma detect_mismatch( "AE_DEVELOP", "0" )
#  endif

#  ifdef AE_PROFILE
#	pragma detect_mismatch( "AE_PROFILE", "1" )
#  else
#	pragma detect_mismatch( "AE_PROFILE", "0" )
#  endif

#  ifdef AE_RELEASE
#	pragma detect_mismatch( "AE_RELEASE", "1" )
#  else
#	pragma detect_mismatch( "AE_RELEASE", "0" )
#  endif

#  if defined(AE_FAST_HASH) && AE_FAST_HASH
#	pragma detect_mismatch( "AE_FAST_HASH", "1" )
#  else
#	pragma detect_mismatch( "AE_FAST_HASH", "0" )
#  endif

#  ifdef AE_CI_BUILD
#	pragma detect_mismatch( "AE_CI_BUILD", "1" )
#  else
#	pragma detect_mismatch( "AE_CI_BUILD", "0" )
#  endif

#  ifdef AE_ENABLE_MEMLEAK_CHECKS
#	pragma detect_mismatch( "AE_ENABLE_MEMLEAK_CHECKS", "1" )
#  else
#	pragma detect_mismatch( "AE_ENABLE_MEMLEAK_CHECKS", "0" )
#  endif

#  ifdef AE_NO_EXCEPTIONS
#	pragma detect_mismatch( "AE_NO_EXCEPTIONS", "1" )
#  else
#	pragma detect_mismatch( "AE_NO_EXCEPTIONS", "0" )
#  endif

// platforms
#  ifdef PLATFORM_WINDOWS
#	pragma detect_mismatch( "PLATFORM_WINDOWS", "1" )
#  else
#	pragma detect_mismatch( "PLATFORM_WINDOWS", "0" )
#  endif

#  ifdef PLATFORM_LINUX
#	pragma detect_mismatch( "PLATFORM_LINUX", "1" )
#  else
#	pragma detect_mismatch( "PLATFORM_LINUX", "0" )
#  endif

#  ifdef PLATFORM_ANDROID
#	pragma detect_mismatch( "PLATFORM_ANDROID", "1" )
#  else
#	pragma detect_mismatch( "PLATFORM_ANDROID", "0" )
#  endif

// compilers
#  ifdef COMPILER_MSVC
#	pragma detect_mismatch( "COMPILER_MSVC", "1" )
#  else
#	pragma detect_mismatch( "COMPILER_MSVC", "0" )
#  endif

#  ifdef COMPILER_CLANG
#	pragma detect_mismatch( "COMPILER_CLANG", "1" )
#  else
#	pragma detect_mismatch( "COMPILER_CLANG", "0" )
#  endif

#  ifdef COMPILER_GCC
#	pragma detect_mismatch( "COMPILER_GCC", "1" )
#  else
#	pragma detect_mismatch( "COMPILER_GCC", "0" )
#  endif

#endif	// AE_CPP_DETECT_MISSMATCH
