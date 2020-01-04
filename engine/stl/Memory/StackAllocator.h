// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Math/Math.h"
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
			  uint MaxBlocks = 16
			 >
	struct StackAllocator final
	{
	// types
	public:
		using Self			= StackAllocator< AllocatorType, MaxBlocks >;
		using Allocator_t	= AllocatorType;

		enum class Bookmark : size_t {};

	private:
		struct Block
		{
			void *		ptr			= null;
			BytesU		size;		// used memory size
			BytesU		capacity;	// size of block
		};

		using Blocks_t			= FixedArray< Block, MaxBlocks >;
		using BookmarkStack_t	= FixedArray< Bookmark, 32 >;

		static constexpr uint	_PtrOffset		= (CT_IntLog2< MaxBlocks > + not IsPowerOfTwo( MaxBlocks ));
		static constexpr size_t	_BlockIndexMask	= (1u << _PtrOffset) - 1;


	// variables
	private:
		Blocks_t					_blocks;
		BookmarkStack_t				_bookmarks;
		BytesU						_blockSize	= 1024_b;
		Allocator_t					_alloc;
		static constexpr BytesU		_ptrAlign	= SizeOf<void *>;


	// methods
	public:
		StackAllocator () {}
		
		explicit StackAllocator (const Allocator_t &alloc) : _alloc{alloc}
		{}
		
		StackAllocator (Self &&other) :
			_blocks{ std::move(other._blocks) },
			_bookmarks{ std::move(other._bookmarks) },
			_blockSize{ other._blockSize },
			_alloc{ std::move(other._alloc) }
		{}

		StackAllocator (const Self &) = delete;

		Self& operator = (const Self &) = delete;


		Self& operator = (Self &&rhs)
		{
			Release();
			_blocks		= std::move(rhs._blocks);
			_bookmarks	= std::move(rhs._bookmarks);
			_blockSize	= rhs._blockSize;
			_alloc		= std::move(rhs._alloc);
			return *this;
		}


		~StackAllocator ()
		{
			Release();
		}


		void SetBlockSize (BytesU size)
		{
			_blockSize = size;
		}


		ND_ AE_ALLOCATOR void*  Alloc (const BytesU size, const BytesU align)
		{
			size_t	idx	= _blocks.size() ? 0u : UMax;

			if ( _bookmarks.size() )
			{
				size_t	off = 0;
				std::tie( idx, off ) = _UnpackBookmark( _bookmarks.back() );

				ASSERT( _blocks[idx].size >= off );
			}

			for (; idx < _blocks.size(); ++idx)
			{
				auto&	block	= _blocks[idx];
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


		ND_ Bookmark  Push ()
		{
			Bookmark	bm = Bookmark(0);

			if ( _blocks.size() )
			{
				bm = Bookmark( (_blocks.size()-1) | (size_t(_blocks.back().size) << _PtrOffset) );
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
			for (size_t i = 0; i < _bookmarks.size(); ++i)
			{
				if ( _bookmarks[i] == bm )
				{
					auto[idx, off] = _UnpackBookmark( bm );

					for (; idx < _blocks.size(); ++idx)
					{
						auto& block = _blocks[idx];
						block.size = BytesU{off};

						DEBUG_ONLY( std::memset( block.ptr + block.size, 0xCD, size_t(block.capacity - block.size) ));
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
				DEBUG_ONLY( std::memset( block.ptr, 0xCD, size_t(block.capacity) ));
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
		ND_ static Pair<size_t, size_t>  _UnpackBookmark (Bookmark bm)
		{
			size_t	u	= BitCast<size_t>( bm );
			size_t	idx	= u & _BlockIndexMask;
			size_t	off	= u >> _PtrOffset;
			return { idx, off };
		}
	};


}	// AE::STL
