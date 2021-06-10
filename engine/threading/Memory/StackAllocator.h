// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Memory/StackAllocator.h"
#include "threading/Common.h"

namespace AE::STL
{

	//
	// Thread-Safe Stack Allocator
	//
	
	template <typename AllocatorType, uint MaxBlocks>
	struct StackAllocator< AllocatorType, MaxBlocks, true > final
	{
	// types
	private:
		using BaseAllocator_t	= StackAllocator< AllocatorType, MaxBlocks, false >;
	public:
		using Self				= StackAllocator< AllocatorType, MaxBlocks, true >;
		using Allocator_t		= AllocatorType;
		using Bookmark			= typename BaseAllocator_t::Bookmark;
		
		static constexpr bool	IsThreadSafe = true;


	// variables
	private:
		Threading::Mutex	_guard;
		BaseAllocator_t		_base;


	// methods
	public:
		StackAllocator () {}
		
		explicit StackAllocator (const Allocator_t &alloc) : _base{ alloc }
		{}
		
		StackAllocator (Self &&other)
		{
			std::scoped_lock	lock{ _guard, other._guard };
			_base = RVRef(other._base);
		}

		StackAllocator (const Self &) = delete;

		Self& operator = (const Self &) = delete;


		Self& operator = (Self &&rhs)
		{
			std::scoped_lock	lock{ _guard, rhs._guard };
			_base = RVRef( rhs._base );
			return *this;
		}


		~StackAllocator ()
		{
			Release();
		}


		void SetBlockSize (Bytes size)
		{
			EXLOCK( _guard );
			return _base.SetBlockSize( size );
		}


		ND_ AE_ALLOCATOR void*  Alloc (const Bytes size, const Bytes align)
		{
			EXLOCK( _guard );
			return _base.Alloc( size, align );
		}


		template <typename T>
		ND_ AE_ALLOCATOR T*  Alloc (usize count = 1)
		{
			EXLOCK( _guard );
			return _base.Alloc<T>( count );
		}


		ND_ Bookmark  Push ()
		{
			EXLOCK( _guard );
			return _base.Push();
		}


		void Pop (Bookmark bm)
		{
			EXLOCK( _guard );
			return _base.Pop( bm );
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
