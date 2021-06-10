// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include <type_traits>
#include <cstdint>
#include <utility>

namespace AE::STL
{

	template <typename T>
	static constexpr bool	IsFloatPoint		= std::is_floating_point_v<T>;

	template <typename T>
	static constexpr bool	IsInteger			= std::is_integral_v<T>;
	
	template <typename T>
	static constexpr bool	IsSignedInteger		= std::is_integral_v<T> and std::is_signed_v<T>;
	
	template <typename T>
	static constexpr bool	IsUnsignedInteger	= std::is_integral_v<T> and std::is_unsigned_v<T>;

	template <typename T>
	static constexpr bool	IsSigned			= std::is_signed_v<T>;

	template <typename T>
	static constexpr bool	IsUnsigned			= std::is_unsigned_v<T>;

	template <typename T>
	static constexpr bool	IsScalar			= std::is_scalar_v<T>;
	
	template <typename T>
	static constexpr bool	IsEnum				= std::is_enum_v<T>;
	
	template <typename T>
	static constexpr bool	IsScalarOrEnum		= std::is_scalar_v<T> or std::is_enum_v<T>;

	template <typename T>
	static constexpr bool	IsPOD				= std::is_trivially_destructible_v<T>;		// TODO: deprecated

	template <typename T>
	static constexpr bool	IsPointer			= std::is_pointer_v<T>;

	template <typename T>
	static constexpr bool	IsNullPtr			= std::is_null_pointer_v<T>;

	template <typename T>
	static constexpr bool	IsLValueRef			= std::is_lvalue_reference_v<T>;
	
	template <typename T>
	static constexpr bool	IsRValueRef			= std::is_rvalue_reference_v<T>;
	
	template <typename T>
	static constexpr bool	IsReference			= std::is_reference_v<T>;

	template <typename T>
	static constexpr bool	IsClass				= std::is_class_v<T>;

	template <typename T>
	static constexpr bool	IsUnion				= std::is_union_v<T>;

	template <typename T>
	static constexpr bool	IsConst				= std::is_const_v<T>;
	
	template <typename T>
	static constexpr bool	IsVolatile			= std::is_volatile_v<T>;

	template <typename T1, typename T2>
	static constexpr bool	IsSameTypes			= std::is_same_v<T1, T2>;

	template <typename T>
	static constexpr bool	IsVoid				= std::is_void_v<T>;

	template <typename Base, typename Derived>
	static constexpr bool	IsBaseOf			= std::is_base_of_v< Base, Derived >;

	template <typename T>
	static constexpr bool	IsEmpty				= std::is_empty_v<T>;

	template <typename From, typename To>
	static constexpr bool	IsConvertible		= std::is_convertible_v<From, To>;

	template <typename T>
	static constexpr bool	IsArithmetic		= std::is_arithmetic_v<T>;

	
	template <typename T>
	using RemovePointer		= std::remove_pointer_t<T>;
	
	template <typename T>
	using RemoveReference	= std::remove_reference_t<T>;

	template <typename T>
	using RemoveCV			= std::remove_cv_t<T>;	// remove 'const', 'volatile', 'const volatile'

	template <typename T>
	using AddConst			= std::add_const_t<T>;

	template <typename T>
	using RemoveConst		= std::remove_const_t<T>;

	template <typename T>
	using RemoveAllExtents	= std::remove_all_extents_t<T>;

	
	template <typename T>	using InPlaceType	= std::in_place_type_t<T>;
	template <typename T>	constexpr InPlaceType<T> InPlaceObj {};


	template <bool Test, typename Type = void>
	using EnableIf		= std::enable_if_t< Test, Type >;

	template <bool Test, typename Type = void>
	using DisableIf		= std::enable_if_t< !Test, Type >;


	template <bool Test, typename IfTrue, typename IfFalse>
	using Conditional	= std::conditional_t< Test, IfTrue, IfFalse >;


	template <usize Bits>
	using BitSizeToUInt		= Conditional< Bits <= sizeof(ubyte), ubyte,
								Conditional< Bits <= sizeof(ushort), ushort,
									Conditional< Bits <= sizeof(uint), uint,
										Conditional< Bits <= sizeof(ulong), ulong,
											void >>>>;

	template <usize Bits>
	using BitSizeToInt		= Conditional< Bits <= sizeof(sbyte), sbyte,
								Conditional< Bits <= sizeof(sshort), sshort,
									Conditional< Bits <= sizeof(sint), sint,
										Conditional< Bits <= sizeof(slong), slong,
											void >>>>;


	template <typename T>
	using ToUnsignedInteger	= BitSizeToUInt< sizeof(T) >;
	
	template <typename T>
	using ToSignedInteger	= BitSizeToInt< sizeof(T) >;

	
	namespace _hidden_
	{
		template <typename T, template <typename...> class Templ>
		struct is_specialization_of : std::bool_constant<false> {};

		template <template <typename...> class Templ, typename... Args>
		struct is_specialization_of< Templ<Args...>, Templ > : std::bool_constant<true> {};
		
		template <template <typename ...> class Left, template <typename ...> class Right>
		struct IsSameTemplates			{ static const bool  value = false; };

		template <template <typename ...> class T>
		struct IsSameTemplates< T, T >	{ static const bool  value = true; };

	}	// _hidden_

	
	template <typename T, template <typename...> class Templ>
	static constexpr bool	IsSpecializationOf	= STL::_hidden_::is_specialization_of< T, Templ >::value;
	
	template <template <typename ...> class Left, template <typename ...> class Right>
	static constexpr bool	IsSameTemplates		= STL::_hidden_::IsSameTemplates< Left, Right >::value;


}	// AE::STL
