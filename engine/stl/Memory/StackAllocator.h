// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Math/BitMath.h"
#include "stl/Math/Bytes.h"
#include "stl/Memory/UntypedAllocator.h"
#include "stl/Containers/FixedArray.h"
#include "stl/CompileTime/Math.h"

namespace AE::STL
{

	//
	// Stack Allocator
	//

	template <typename AllocatorType = UntypedAlignedAllocator,
			  uint MaxBlocks = 16,
			  bool ThreadSafe = false
			 >
	struct StackAllocator final
	{
		STATIC_ASSERT( not ThreadSafe );

	// types
	public:
		using Self			= StackAllocator< AllocatorType, MaxBlocks, false >;
		using Allocator_t	= AllocatorType;

		enum class Bookmark : usize {};
		
		static constexpr bool	IsThreadSafe = false;

	private:
		struct Block
		{
			void *		ptr			= null;
			Bytes		size;		// used memory size
			Bytes		capacity;	// size of block
		};

		using Blocks_t			= FixedArray< Block, MaxBlocks >;
		using BookmarkStack_t	= FixedArray< Bookmark, 32 >;

		static constexpr uint	_PtrOffset		= (CT_IntLog2< MaxBlocks > + not IsPowerOfTwo( MaxBlocks ));
		static constexpr usize	_BlockIndexMask	= (1u << _PtrOffset) - 1;


	// variables
	private:
		Blocks_t					_blocks;
		BookmarkStack_t				_bookmarks;
		Bytes						_blockSize	= 1024_b;
		Allocator_t					_alloc;
		static constexpr Bytes		_ptrAlign	= SizeOf<void *>;


	// methods
	public:
		StackAllocator () {}
		
		explicit StackAllocator (const Allocator_t &alloc) : _alloc{alloc}
		{}
		
		StackAllocator (Self &&other) :
			_blocks{ RVRef(other._blocks) },
			_bookmarks{ RVRef(other._bookmarks) },
			_blockSize{ other._blockSize },
			_alloc{ RVRef(other._alloc) }
		{}

		StackAllocator (const Self &) = delete;

		Self& operator = (const Self &) = delete;


		Self& operator = (Self &&rhs)
		{
			Release();
			_blocks		= RVRef(rhs._blocks);
			_bookmarks	= RVRef(rhs._bookmarks);
			_blockSize	= rhs._blockSize;
			_alloc		= RVRef(rhs._alloc);
			return *this;
		}


		~StackAllocator ()
		{
			Release();
		}


		void SetBlockSize (Bytes size)
		{
			_blockSize = size;
		}


		ND_ AE_ALLOCATOR void*  Alloc (const Bytes size, const Bytes align)
		{
			usize	idx	= _blocks.size() ? 0u : UMax;

			if ( _bookmarks.size() )
			{
				usize	off = 0;
				std::tie( idx, off ) = _UnpackBookmark( _bookmarks.back() );

				ASSERT( idx >= _blocks.size() or _blocks[idx].size >= off );
			}

			for (; idx < _blocks.size(); ++idx)
			{
				auto&	block	= _blocks[idx];
				Bytes	offset	= AlignToLarger( usize(block.ptr) + block.size, align ) - usize(block.ptr);
				
				if ( size <= (block.capacity - offset) )
				{
					block.size = offset + size;
					return block.ptr + offset;
				}
			}
			
			if ( _blocks.size() == _blocks.capacity() )
			{
				//ASSERT( !"overflow" );
				return null;
			}

			Bytes	block_size	= _blockSize * (1 + _blocks.size()/2);
					block_size	= size*2 < block_size ? block_size : block_size*2;
			void*	ptr			= _alloc.Allocate( block_size, _ptrAlign );

			if ( not ptr )
			{
				ASSERT( !"failed to allocate memory" );
				return null;
			}

			auto&	block		= _blocks.emplace_back(Block{ ptr, 0_b, block_size });
			Bytes	offset		= AlignToLarger( usize(block.ptr) + block.size, align ) - usize(block.ptr);

			DEBUG_ONLY( std::memset( block.ptr, 0xCD, usize(block.capacity) ));
			
			block.size = offset + size;
			return block.ptr + offset;
		}


		template <typename T>
		ND_ AE_ALLOCATOR T*  Alloc (usize count = 1)
		{
			return Cast<T>( Alloc( SizeOf<T> * count, AlignOf<T> ));
		}


		ND_ Bookmark  Push ()
		{
			Bookmark	bm = Bookmark(0);

			if ( _blocks.size() )
			{
				bm = Bookmark( (_blocks.size()-1) | (usize(_blocks.back().size) << _PtrOffset) );
			}

			if ( _bookmarks.size() == _bookmarks.capacity() )
			{
				ASSERT( !"overflow" );
				return Bookmark(~0ull);
			}

			_bookmarks.push_back( bm );
			return bm;
		}


		void Pop (Bookmark bm)
		{
			for (usize i = 0; i < _bookmarks.size(); ++i)
			{
				if ( _bookmarks[i] == bm )
				{
					auto[idx, off] = _UnpackBookmark( bm );

					for (; idx < _blocks.size(); ++idx)
					{
						auto& block = _blocks[idx];
						block.size = Bytes{off};

						DEBUG_ONLY( std::memset( block.ptr + block.size, 0xCD, usize(block.capacity - block.size) ));
						off = 0;
					}

					_bookmarks.resize( i );
					return;
				}
			}
		}


		void Discard ()
		{
			for (auto& block : _blocks)
			{
				block.size = 0_b;
				DEBUG_ONLY( std::memset( block.ptr, 0xCD, usize(block.capacity) ));
			}
			_bookmarks.clear();
		}

		void Release ()
		{
			for (auto& block : _blocks) {
				_alloc.Deallocate( block.ptr, block.capacity, _ptrAlign );
			}
			_blocks.clear();
			_bookmarks.clear();
		}


	private:
		ND_ static Pair<usize, usize>  _UnpackBookmark (Bookmark bm)
		{
			usize	u	= BitCast<usize>( bm );
			usize	idx	= u & _BlockIndexMask;
			usize	off	= u >> _PtrOffset;
			return { idx, off };
		}
	};


}	// AE::STL
