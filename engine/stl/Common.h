// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Defines.h"

#include <vector>
#include <string>
#include <array>
#include <memory>		// shared_ptr, weak_ptr, unique_ptr
#include <deque>
#include <unordered_set>
#include <unordered_map>
#include <bitset>
#include <cstring>
#include <cmath>
#include <optional>
#include <string_view>
#include <typeindex>
#include <type_traits>
#include <chrono>
#include <algorithm>

namespace AE
{
	namespace STL
	{
		using sbyte		= int8_t;
		using ubyte		= uint8_t;
		using sshort	= int16_t;
		using ushort	= uint16_t;
		using sint		= int32_t;
		using uint 		= uint32_t;
		using slong		= int64_t;
		using ulong		= uint64_t;
		using ssize		= intptr_t;
		using usize		= size_t;

	} // STL

	namespace Math {}
}

namespace AE::Math
{
	using namespace AE::STL;
}

#include "stl/Log/Log.h"
#include "stl/Algorithms/Hash.h"
#include "stl/CompileTime/TypeTraits.h"
#include "stl/CompileTime/Constants.h"
#include "stl/CompileTime/DefaultType.h"
#include "stl/Containers/Tuple.h"

namespace AE::STL
{
	using namespace AE::Math;

							using String		= std::string;
							using WString		= std::wstring;
	template <typename T>	using BasicString	= std::basic_string< T >;

	template <typename T>	using Array			= std::vector< T >;

	template <typename T>	using SharedPtr		= std::shared_ptr< T >;
	template <typename T>	using WeakPtr		= std::weak_ptr< T >;

	template <typename T>	using Deque			= std::deque< T >;

	template <usize N>		using BitSet		= std::bitset< N >;
	
	template <typename T>	using Optional		= std::optional< T >;
	
							using StringView		= std::string_view;
							using WStringView		= std::wstring_view;
	template <typename T>	using BasicStringView	= std::basic_string_view<T>;

	template <typename T>	using Function		= std::function< T >;


	template <typename T,
			  typename Deleter = std::default_delete<T>>
	using UniquePtr		= std::unique_ptr< T, Deleter >;

	template <typename T,
			  usize ArraySize>
	using StaticArray	= std::array< T, ArraySize >;


	template <typename FirstT,
			  typename SecondT>
	using Pair			= std::pair< FirstT, SecondT >;
	

	template <typename T,
			  typename Hasher = std::hash<T>,
			  typename KeyEq  = std::equal_to<T>,
			  typename Alloc  = std::allocator<T>>
	using HashSet		= std::unordered_set< T, Hasher, KeyEq, Alloc >;


	template <typename Key,
			  typename Value,
			  typename Hasher = std::hash<Key>,
			  typename KeyEq  = std::equal_to<Key>,
			  typename Alloc  = std::allocator<std::pair<const Key, Value>>>
	using HashMap		= std::unordered_map< Key, Value, Hasher, KeyEq, Alloc >;
	
	template <typename Key,
			  typename Value,
			  typename Hasher = std::hash<Key>,
			  typename KeyEq  = std::equal_to<Key>,
			  typename Alloc  = std::allocator<std::pair<const Key, Value>>>
	using HashMultiMap	= std::unordered_multimap< Key, Value, Hasher, KeyEq, Alloc >;
	

	// Uppercase names reserved by physical quantity wrappers
	using seconds		= std::chrono::seconds;
	using milliseconds	= std::chrono::milliseconds;
	using microseconds	= std::chrono::microseconds;
	using nanoseconds	= std::chrono::nanoseconds;
	
	
/*
=================================================
	Unused
=================================================
*/
	template <typename... Args>
	constexpr void  Unused (Args&& ...) {}
	
/*
=================================================
	ArgRef (same as std::ref)
=================================================
*/
	template <typename T>
	ND_ forceinline constexpr std::reference_wrapper<T>  ArgRef (T& arg) noexcept
	{
		return std::reference_wrapper<T>{ arg };
	}
	
/*
=================================================
	RVRef (same as std::move)
=================================================
*/
	template <typename T>
	ND_ forceinline constexpr RemoveReference<T> &&  RVRef (T& arg) noexcept
	{
		return static_cast< RemoveReference<T> && >( arg );
	}

/*
=================================================
	FwdArg (same as std::forward)
=================================================
*/
	template <typename T>
	ND_ forceinline constexpr T &&  FwdArg (RemoveReference<T>& arg) noexcept
	{
		return static_cast<T &&>( arg );
	}

	template <typename T>
	ND_ forceinline constexpr T &&  FwdArg (RemoveReference<T>&& arg) noexcept
	{
		STATIC_ASSERT( not IsLValueReference<T> );
		return static_cast<T &&>( arg );
	}

/*
=================================================
	MakeShared
=================================================
*/
	template <typename T, typename ...Types>
	ND_ forceinline SharedPtr<T>  MakeShared (Types&&... args)
	{
		return std::make_shared<T>( FwdArg<Types>( args )... );
	}

/*
=================================================
	MakeUnique
=================================================
*/
	template <typename T, typename ...Types>
	ND_ forceinline UniquePtr<T>  MakeUnique (Types&&... args)
	{
		return std::make_unique<T>( FwdArg<Types>( args )... );
	}
	
}	// AE::STL

namespace AE
{
#	ifndef AE_NO_EXCEPTIONS
	class Exception final : public std::runtime_error
	{
	public:
		explicit Exception (STL::StringView sv) : runtime_error{ STL::String{sv} } {}
		explicit Exception (STL::String str) : runtime_error{ STL::RVRef(str) } {}
		explicit Exception (const char *str) : runtime_error{ STL::String{str} } {}
	};
#	endif

}	// AE
