// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Math/Math.h"
#include "stl/Math/Bytes.h"
#include "stl/Memory/UntypedAllocator.h"
#include "stl/Containers/FixedArray.h"
#include "stl/CompileTime/TemplateUtils.h"

namespace AE::STL
{

	//
	// Linear Allocator
	//

	template <typename AllocatorType = UntypedAlignedAllocator,
			  uint MaxBlocks = 16,
			  bool ThreadSafe = false
			 >
	struct LinearAllocator final
	{
		STATIC_ASSERT( not ThreadSafe );

	// types
	public:
		using Allocator_t	= AllocatorType;
		using Self			= LinearAllocator< AllocatorType, MaxBlocks, false >;
		
		static constexpr bool	IsThreadSafe = false;

	private:
		struct Block
		{
			void *		ptr			= null;
			BytesU		size;		// used memory size
			BytesU		capacity;	// size of block
		};
		using Blocks_t = FixedArray< Block, MaxBlocks >;


	// variables
	private:
		Blocks_t					_blocks;
		BytesU						_blockSize	= 1024_b;
		Allocator_t					_alloc;
		static constexpr BytesU		_ptrAlign	= SizeOf<void *>;


	// methods
	public:
		LinearAllocator () {}
		
		explicit LinearAllocator (const Allocator_t &alloc) : _alloc{alloc}
		{}
		
		LinearAllocator (Self &&other) :
			_blocks{ std::move(other._blocks) },
			_blockSize{ other._blockSize },
			_alloc{ std::move(other._alloc) }
		{}

		LinearAllocator (const Self &) = delete;

		Self& operator = (const Self &) = delete;


		Self& operator = (Self &&rhs)
		{
			Release();
			_blocks		= std::move(rhs._blocks);
			_blockSize	= rhs._blockSize;
			_alloc		= std::move(rhs._alloc);
			return *this;
		}


		~LinearAllocator ()
		{
			Release();
		}


		void SetBlockSize (BytesU size)
		{
			_blockSize = size;
		}


		ND_ AE_ALLOCATOR void*  Alloc (const BytesU size, const BytesU align)
		{
			for (auto& block : _blocks)
			{
				BytesU	offset	= AlignToLarger( size_t(block.ptr) + block.size, align ) - size_t(block.ptr);

				if ( size <= (block.capacity - offset) )
				{
					block.size = offset + size;
					return block.ptr + offset;
				}
			}

			if ( _blocks.size() == _blocks.capacity() )
			{
				ASSERT( !"overflow" );
				return null;
			}

			BytesU	block_size	= _blockSize * (1 + _blocks.size()/2);
					block_size	= size*2 < block_size ? block_size : size*2;
			void*	ptr			= _alloc.Allocate( block_size, _ptrAlign );

			if ( not ptr )
			{
				ASSERT( !"failed to allocate memory" );
				return null;
			}

			auto&	block		= _blocks.emplace_back(Block{ ptr, 0_b, block_size });
			BytesU	offset		= AlignToLarger( size_t(block.ptr) + block.size, align ) - size_t(block.ptr);

			DEBUG_ONLY( std::memset( block.ptr, 0xCD, size_t(block.capacity) ));
			
			block.size = offset + size;
			return block.ptr + offset;
		}


		template <typename T>
		ND_ AE_ALLOCATOR T*  Alloc (size_t count = 1)
		{
			return Cast<T>( Alloc( SizeOf<T> * count, AlignOf<T> ));
		}


		void Discard ()
		{
			for (auto& block : _blocks)
			{
				block.size = 0_b;
				DEBUG_ONLY( std::memset( block.ptr, 0xCD, size_t(block.capacity) ));
			}
		}

		void Release ()
		{
			for (auto& block : _blocks) {
				_alloc.Deallocate( block.ptr, block.capacity, _ptrAlign );
			}
			_blocks.clear();
		}
	};



	//
	// Untyped Linear Allocator
	//
	
	template <typename AllocatorType = UntypedAlignedAllocator,
			  uint MaxBlocks = 16,
			  bool ThreadSafe = false
			 >
	struct UntypedLinearAllocator final
	{
		STATIC_ASSERT( not ThreadSafe );

	// types
	public:
		using Allocator_t		= AllocatorType;
		using LinearAllocator_t	= LinearAllocator< AllocatorType, MaxBlocks, false >;
		using Self				= UntypedLinearAllocator< AllocatorType, MaxBlocks, false >;

		template <typename T>
		using StdAllocator_t	= StdLinearAllocator< T, AllocatorType, ValueToType<MaxBlocks> >;
		
		static constexpr bool	IsThreadSafe = false;


	// variables
	private:
		LinearAllocator_t&	_alloc;
		

	// methods
	public:
		UntypedLinearAllocator (Self &&other) : _alloc{other._alloc} {}
		UntypedLinearAllocator (const Self &other) : _alloc{other._alloc} {}
		UntypedLinearAllocator (LinearAllocator_t &alloc) : _alloc{alloc} {}


		ND_ AE_ALLOCATOR void*  Allocate (BytesU size, BytesU align)
		{
			return _alloc.Alloc( size, align );
		}

		void  Deallocate (void *, BytesU)
		{}

		void  Deallocate (void *, BytesU, BytesU)
		{}

		ND_ LinearAllocator_t&  GetAllocatorRef () const
		{
			return _alloc;
		}

		ND_ bool operator == (const Self &rhs) const
		{
			return &_alloc == &rhs._alloc;
		}
	};
	


	//
	// Std Linear Allocator
	//

	template <typename T,
			  typename AllocatorType = UntypedAlignedAllocator,
			  typename MaxBlocksT = ValueToType<16u>
			 >
	struct StdLinearAllocator
	{
	// types
	public:
		static constexpr uint	MaxBlocks = MaxBlocksT::value;
		static constexpr bool	IsThreadSafe = false;

		using value_type			= T;
		using Allocator_t			= AllocatorType;
		using LinearAllocator_t		= LinearAllocator< AllocatorType, MaxBlocks, IsThreadSafe >;
		using Self					= StdLinearAllocator< T, AllocatorType, MaxBlocksT >;
		using UntypedAllocator_t	= UntypedLinearAllocator< AllocatorType, MaxBlocks, IsThreadSafe >;
		

	// variables
	private:
		LinearAllocator_t&	_alloc;


	// methods
	public:
		StdLinearAllocator (LinearAllocator_t &alloc) : _alloc{alloc} {}
		StdLinearAllocator (const UntypedAllocator_t &alloc) : _alloc{alloc.GetAllocatorRef()} {}

		StdLinearAllocator (Self &&other) : _alloc{other._alloc} {}
		StdLinearAllocator (const Self &other) : _alloc{other._alloc} {}
		
		template <typename B>
		StdLinearAllocator (const StdLinearAllocator<B,Allocator_t>& other) : _alloc{other.GetAllocatorRef()} {}

		Self& operator = (const Self &) = delete;

		
		ND_ AE_ALLOCATOR T*  allocate (const size_t count)
		{
			return _alloc.template Alloc<T>( count );
		}

		void deallocate (T * const, const size_t)
		{
		}

		ND_ Self  select_on_container_copy_construction () const
		{
			return Self{ _alloc };
		}

		ND_ LinearAllocator_t&  GetAllocatorRef () const
		{
			return _alloc;
		}

		ND_ bool operator == (const Self &rhs) const
		{
			return &_alloc == &rhs._alloc;
		}
	};


}	// AE::STL
