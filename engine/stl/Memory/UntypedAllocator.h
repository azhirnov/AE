// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

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
		ND_ AE_ALLOCATOR static void*  Allocate (Bytes size)
		{
			return ::operator new ( usize(size), std::nothrow_t{} );
		}
		
		static void  Deallocate (void *ptr)
		{
			::operator delete ( ptr, std::nothrow_t() );
		}
		
		// deallocation with explicit size may be faster
		static void  Deallocate (void *ptr, Bytes size)
		{
			::operator delete ( ptr, usize(size) );
		}

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
		ND_ AE_ALLOCATOR static void*  Allocate (Bytes size, Bytes align)
		{
			return ::operator new ( usize(size), std::align_val_t(usize(align)), std::nothrow_t{} );
		}
		
		static void  Deallocate (void *ptr, Bytes align)
		{
			::operator delete ( ptr, std::align_val_t(usize(align)), std::nothrow_t() );
		}
		
		// deallocation with explicit size may be faster
		static void  Deallocate (void *ptr, Bytes size, Bytes align)
		{
			::operator delete ( ptr, usize(size), std::align_val_t(usize(align)) );
		}

		ND_ bool  operator == (const UntypedAlignedAllocator &) const
		{
			return true;
		}
	};


	
	//
	// Untyped Allocator with base alignment
	//

	template <usize BaseAlign>
	struct UntypedAllocatorBaseAlign
	{
	// types
		static constexpr usize	Align			= BaseAlign;
		static constexpr bool	IsThreadSafe	= true;
		
	// methods
		ND_ AE_ALLOCATOR static void*  Allocate (Bytes size)
		{
			return ::operator new ( usize(size), std::align_val_t(BaseAlign), std::nothrow_t{} );
		}
		
		static void  Deallocate (void *ptr)
		{
			::operator delete ( ptr, std::align_val_t(BaseAlign), std::nothrow_t() );
		}
		
		// deallocation with explicit size may be faster
		static void  Deallocate (void *ptr, Bytes size)
		{
			::operator delete ( ptr, usize(size), std::align_val_t(BaseAlign) );
		}

		ND_ bool  operator == (const UntypedAllocatorBaseAlign<BaseAlign> &) const
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


}	// AE::STL
