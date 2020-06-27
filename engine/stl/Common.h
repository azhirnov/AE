// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Defines.h"

// mem leak check
#if defined(COMPILER_MSVC) && defined(AE_ENABLE_MEMLEAK_CHECKS)
#	define _CRTDBG_MAP_ALLOC
#	include <stdlib.h>
#	include <crtdbg.h>

	// call at exit
	// returns 'true' if no mem leaks
#	define AE_CHECK_MEMLEAKS()	(::_CrtDumpMemoryLeaks() != TRUE)
#else

#	define AE_DUMP_MEMLEAKS()	(true)
#endif

// Config
#ifndef AE_FAST_HASH
#	define AE_FAST_HASH		0
#endif

#ifndef AE_OPTIMIZE_IDS
# ifdef AE_DEBUG
#	define AE_OPTIMIZE_IDS	0
# else
#	define AE_OPTIMIZE_IDS	1
# endif
#endif

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
#include <malloc.h>
#include <optional>
#include <string_view>
#include <typeindex>
#include <type_traits>
#include <chrono>

#include "stl/Log/Log.h"
#include "stl/Algorithms/Hash.h"
#include "stl/CompileTime/TypeTraits.h"
#include "stl/CompileTime/Constants.h"
#include "stl/CompileTime/DefaultType.h"
#include "stl/Containers/Tuple.h"

namespace AE
{
	namespace STL {}
	namespace Math {}
}

namespace AE::Math
{
	using namespace AE::STL;
}

namespace AE::STL
{
	using namespace AE::Math;

	using uint 		= uint32_t;

							using String		= std::string;
							using WString		= std::wstring;
	template <typename T>	using BasicString	= std::basic_string< T >;

	template <typename T>	using Array			= std::vector< T >;

	template <typename T>	using SharedPtr		= std::shared_ptr< T >;
	template <typename T>	using WeakPtr		= std::weak_ptr< T >;

	template <typename T>	using Deque			= std::deque< T >;

	template <size_t N>		using BitSet		= std::bitset< N >;
	
	template <typename T>	using Optional		= std::optional< T >;
	
							using StringView		= std::string_view;
							using WStringView		= std::wstring_view;
	template <typename T>	using BasicStringView	= std::basic_string_view<T>;

	template <typename T>	using Function		= std::function< T >;


	template <typename T,
			  typename Deleter = std::default_delete<T>>
	using UniquePtr		= std::unique_ptr< T, Deleter >;

	template <typename T,
			  size_t ArraySize>
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
	constexpr void Unused (Args&& ...) {}
	
/*
=================================================
	MakeShared
=================================================
*/
	template <typename T, typename ...Types>
	ND_ forceinline SharedPtr<T>  MakeShared (Types&&... args)
	{
		return std::make_shared<T>( std::forward<Types>( args )... );
	}

/*
=================================================
	MakeUnique
=================================================
*/
	template <typename T, typename ...Types>
	ND_ forceinline UniquePtr<T>  MakeUnique (Types&&... args)
	{
		return std::make_unique<T>( std::forward<Types>( args )... );
	}

}	// AE::STL
