// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Memory/MemUtils.h"
#include "stl/Memory/AllocatorFwdDecl.h"

namespace AE::STL
{

	//
	// Untyped Default Allocator
	//

	struct UntypedAllocator
	{
	// types
		template <typename T>	using StdAllocator_t = StdAllocator<T>;
		
		static constexpr bool	IsThreadSafe = true;

	// methods
	#if defined(COMPILER_MSVC) and defined(AE_ENABLE_MEMLEAK_CHECKS)

		ND_ AE_ALLOCATOR static void*  Allocate (BytesU size, const char *file, int line)
		{
			return _malloc_dbg( size_t(size), _NORMAL_BLOCK, file, line );
		}
		
		static void  Deallocate (void *ptr)
		{
			_free_dbg( ptr, _NORMAL_BLOCK );
		}
		
		static void  Deallocate (void *ptr, BytesU size)
		{
			Unused( size );
			_free_dbg( ptr, _NORMAL_BLOCK );
		}

	#else

		ND_ AE_ALLOCATOR static void*  Allocate (BytesU size)
		{
			return ::operator new ( size_t(size), std::nothrow_t{} );
		}
		
		static void  Deallocate (void *ptr)
		{
			::operator delete ( ptr, std::nothrow_t() );
		}
		
		// deallocation with explicit size may be faster
		static void  Deallocate (void *ptr, BytesU size)
		{
			::operator delete ( ptr, size_t(size) );
		}

	#endif	// AE_ENABLE_MEMLEAK_CHECKS

		ND_ bool  operator == (const UntypedAllocator &) const
		{
			return true;
		}
	};
	


	//
	// Untyped Aligned Allocator
	//

	struct UntypedAlignedAllocator
	{
	// types
		template <typename T>	using StdAllocator_t = StdAllocator<T>;

		static constexpr bool	IsThreadSafe = true;

	// methods
	#if defined(COMPILER_MSVC) and defined(AE_ENABLE_MEMLEAK_CHECKS)

		ND_ AE_ALLOCATOR static void*  Allocate (BytesU size, BytesU align, const char *file, int line)
		{
			return _aligned_malloc_dbg( size_t(size), size_t(align), file, line );
		}
		
		static void  Deallocate (void *ptr, BytesU align)
		{
			Unused( align );
			_aligned_free_dbg( ptr );
		}
		
		// deallocation with explicit size may be faster
		static void  Deallocate (void *ptr, BytesU size, BytesU align)
		{
			Unused( size, align );
			_aligned_free_dbg( ptr );
		}

	#else

		ND_ AE_ALLOCATOR static void*  Allocate (BytesU size, BytesU align)
		{
			return ::operator new ( size_t(size), std::align_val_t(size_t(align)), std::nothrow_t{} );
		}
		
		static void  Deallocate (void *ptr, BytesU align)
		{
			::operator delete ( ptr, std::align_val_t(size_t(align)), std::nothrow_t() );
		}
		
		// deallocation with explicit size may be faster
		static void  Deallocate (void *ptr, BytesU size, BytesU align)
		{
			::operator delete ( ptr, size_t(size), std::align_val_t(size_t(align)) );
		}

	#endif

		ND_ bool  operator == (const UntypedAlignedAllocator &) const
		{
			return true;
		}
	};

	

	//
	// STD Allocator
	//

	template <typename T>
	struct StdAllocator final : std::allocator<T>
	{
		StdAllocator () {}
		StdAllocator (const StdAllocator &) = default;
		StdAllocator (StdAllocator &&) = default;

		StdAllocator (const UntypedAllocator &) {}
		StdAllocator (const UntypedAlignedAllocator &) {}
	};

	
#ifdef AE_ENABLE_MEMLEAK_CHECKS
#	define Allocate( ... )		Allocate( __VA_ARGS__, __FILE__, __LINE__ )
#endif

}	// AE::STL
