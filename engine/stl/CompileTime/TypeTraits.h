// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

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
	static constexpr bool	IsStaticArray		= std::is_array_v<T>;

	template <typename T>
	static constexpr bool	IsScalar			= std::is_scalar_v<T>;
	
	template <typename T>
	static constexpr bool	IsEnum				= std::is_enum_v<T>;
	
	template <typename T>
	static constexpr bool	IsScalarOrEnum		= std::is_scalar_v<T> or std::is_enum_v<T>;

	template <typename T>
	static constexpr bool	IsPOD				= std::is_pod_v<T>;

	template <typename T>
	static constexpr bool	IsPointer			= std::is_pointer_v<T>;

	template <typename T>
	static constexpr bool	IsClass				= std::is_class_v<T>;

	template <typename T>
	static constexpr bool	IsUnion				= std::is_union_v<T>;

	template <typename T>
	static constexpr bool	IsConst				= std::is_const_v<T>;

	template <typename T1, typename T2>
	static constexpr bool	IsSameTypes			= std::is_same_v<T1, T2>;

	template <typename T>
	static constexpr bool	IsVoid				= std::is_void_v<T>;

	template <typename Base, typename Derived>
	static constexpr bool	IsBaseOf			= std::is_base_of_v< Base, Derived >;

	template <typename T>
	static constexpr bool	IsEmpty				= std::is_empty_v<T>;

	
	template <typename T>	using InPlaceType	= std::in_place_type_t<T>;
	template <typename T>	constexpr InPlaceType<T> InPlaceObj {};


	template <bool Test, typename Type = void>
	using EnableIf		= std::enable_if_t< Test, Type >;

	template <bool Test, typename Type = void>
	using DisableIf		= std::enable_if_t< !Test, Type >;


	template <bool Test, typename IfTrue, typename IfFalse>
	using Conditional	= std::conditional_t< Test, IfTrue, IfFalse >;


	template <size_t Bits>
	using BitSizeToUInt		= Conditional< Bits <= sizeof(uint8_t), uint8_t,
								Conditional< Bits <= sizeof(uint16_t), uint16_t,
									Conditional< Bits <= sizeof(uint32_t), uint32_t,
										Conditional< Bits <= sizeof(uint64_t), uint64_t,
											void >>>>;

	template <size_t Bits>
	using BitSizeToInt		= Conditional< Bits <= sizeof(int8_t), int8_t,
								Conditional< Bits <= sizeof(int16_t), int16_t,
									Conditional< Bits <= sizeof(int32_t), int32_t,
										Conditional< Bits <= sizeof(int64_t), int64_t,
											void >>>>;


	template <typename T>
	using ToUnsignedInteger	= BitSizeToUInt< sizeof(T) >;
	
	template <typename T>
	using ToSignedInteger	= BitSizeToInt< sizeof(T) >;

	
	namespace _ae_stl_hidden_
	{
		template <typename T, template <typename...> class Templ>
		struct is_specialization_of : std::bool_constant<false> {};

		template <template <typename...> class Templ, typename... Args>
		struct is_specialization_of< Templ<Args...>, Templ > : std::bool_constant<true> {};

	}	// _ae_stl_hidden_

	
	template <typename T, template <typename...> class Templ>
	static constexpr bool	IsSpecializationOf = _ae_stl_hidden_::is_specialization_of< T, Templ >::value;
	

}	// AE::STL
