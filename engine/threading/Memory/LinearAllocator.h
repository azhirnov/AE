// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Memory/LinearAllocator.h"
#include "threading/Common.h"

namespace AE::STL
{

	//
	// Linear Allocator
	//

	template <typename AllocatorType, uint MaxBlocks>
	struct LinearAllocator< AllocatorType, MaxBlocks, true > final
	{
	// types
	private:
		using BaseAlloc_t	= LinearAllocator< AllocatorType, MaxBlocks, false >;
	public:
		using Allocator_t	= AllocatorType;
		using Self			= LinearAllocator< AllocatorType, MaxBlocks, true >;
		
		static constexpr bool	IsThreadSafe = true;


	// variables
	private:
		Threading::Mutex	_guard;
		BaseAlloc_t			_base;


	// methods
	public:
		LinearAllocator () {}
		
		explicit LinearAllocator (const Allocator_t &alloc) : _base{ alloc }
		{}
		
		LinearAllocator (Self &&other)
		{
			std::scoped_lock	lock{ _guard, other._guard };
			_base = std::move(other._base);
		}

		LinearAllocator (const Self &) = delete;

		Self& operator = (const Self &) = delete;


		Self& operator = (Self &&rhs)
		{
			std::scoped_lock	lock{ _guard, other._guard };
			_base = std::move(other._base);
			return *this;
		}


		~LinearAllocator ()
		{
			Release();
		}


		void SetBlockSize (BytesU size)
		{
			EXLOCK( _guard );
			return _base.SetBlockSize( size );
		}


		ND_ AE_ALLOCATOR void*  Alloc (const BytesU size, const BytesU align)
		{
			EXLOCK( _guard );
			return _base.Alloc( size, align );
		}


		template <typename T>
		ND_ AE_ALLOCATOR T*  Alloc (size_t count = 1)
		{
			EXLOCK( _guard );
			return _base.Alloc<T>( count );
		}


		void Discard ()
		{
			EXLOCK( _guard );
			return _base.Discard();
		}

		void Release ()
		{
			EXLOCK( _guard );
			return _base.Release();
		}
	};


}	// AE::STL
