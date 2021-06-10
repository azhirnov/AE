// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#ifdef AE_ENABLE_VULKAN
# include "graphics/Vulkan/RenderGraph/VTransferContext.h"

namespace AE::Graphics::_hidden_
{
	
/*
=================================================
	constructor
=================================================
*/
	_VBaseIndirectTrasferCtx::_VBaseIndirectTrasferCtx (RC<VCommandBatch>)
	{
		_allocator.SetBlockSize( 4_Kb );
	}
	
/*
=================================================
	destructor
=================================================
*/
	_VBaseIndirectTrasferCtx::~_VBaseIndirectTrasferCtx ()
	{
	}
	
/*
=================================================
	_CreateCmd
=================================================
*/
	template <typename CmdType, typename ...DynamicTypes>
	CmdType&  _VBaseIndirectTrasferCtx::_CreateCmd (usize dynamicArraySize)
	{
		Bytes	size	= AlignToLarger( _CalcCmdSize< 0, TypeList<DynamicTypes...> >( SizeOf<CmdType>, dynamicArraySize ), _MaxCmdAlign1 );
		auto*	cmd		= Cast<CmdType>( _allocator.Alloc( size, Bytes{_MaxCmdAlign1} ));
		DEBUG_ONLY(
			cmd->_magicNumber = BaseCmd::MAGIC;
		)

		cmd->_commandID = CheckCast<ushort>( TransferCommands_t::Index< CmdType >);
		cmd->_size		= CheckCast<ushort>( size );
		return *cmd;
	}
	
/*
=================================================
	_CalcCmdSize
=================================================
*/
	template <usize I, typename TL>
	constexpr Bytes  _VBaseIndirectTrasferCtx::_CalcCmdSize (Bytes size, usize dynamicArraySize)
	{
		if constexpr( I < TL::Count )
		{
			using T = typename TL::template Get<I>;
			return _CalcCmdSize< I+1, TL >( AlignToLarger( size, alignof(T) ) + sizeof(T) * dynamicArraySize, dynamicArraySize ); 
		}
		else
			return size;
	}

/*
=================================================
	_Prepare
=================================================
*/
	VBakedCommands  _VBaseIndirectTrasferCtx::_Prepare (VBakedCommands::ExecuteFn_t execFn)
	{
		ASSERT( _Validate( 100 ));

		Unused( _CreateCmd<EndCmd>() );

		Bytes	size;
		for (auto& block : _allocator.GetBlocks()) {
			size += block.size;
		}

		void*	ptr		= VBakedCommands::Allocator_t::Allocate( size );
		Bytes	offset;
		
		for (auto& block : _allocator.GetBlocks())
		{
			MemCopy( ptr + offset, block.ptr, block.size );
			offset += block.size;
			ASSERT( IsAligned( offset, _MaxCmdAlign1 ));
		}
		ASSERT( offset == size );
		
		_allocator.Release();

		ASSERT( _Validate( ptr, 100 ));

		return VBakedCommands{ ptr, execFn };
	}
	
/*
=================================================
	_Validate
=================================================
*/
	bool  _VBaseIndirectTrasferCtx::_Validate (uint maxCommandID) const
	{
		uint idx = 0;
		for (auto& block : _allocator.GetBlocks())
		{
			BaseCmd const*	base = Cast<BaseCmd>( block.ptr );

			if ( base == null )
				return false;

			for (; base; ++idx)
			{
				if ( base->_magicNumber != BaseCmd::MAGIC )
					return false;

				if ( base->_commandID > maxCommandID )
					return false;
				
				if ( base->_commandID == TransferCommands_t::Index<EndCmd> )
					break;

				base = static_cast<const BaseCmd *>( static_cast<const void *>(base) + Bytes{base->_size} );

				if ( base >= block.ptr + block.size )
					break;
			}
		}
		return true;
	}

	bool  _VBaseIndirectTrasferCtx::_Validate (const void* ptr, uint maxCommandID) const
	{
		BaseCmd const*	base = Cast<BaseCmd>( ptr );

		if ( base == null )
			return false;

		for (uint idx = 0; base; ++idx)
		{
			if ( base->_magicNumber != BaseCmd::MAGIC )
				return false;

			if ( base->_commandID > maxCommandID )
				return false;

			if ( base->_commandID == TransferCommands_t::Index<EndCmd> )
				break;

			base = static_cast<const BaseCmd *>( static_cast<const void *>(base) + Bytes{base->_size} );
		}
		return true;
	}

/*
=================================================
	ClearColorImage
=================================================
*/
	void  _VBaseIndirectTrasferCtx::ClearColorImage (VkImage image, const VkClearColorValue &color, ArrayView<VkImageSubresourceRange> ranges)
	{
		ASSERT( image != VK_NULL_HANDLE );
		ASSERT( ranges.size() );
		
		auto&	cmd			= _CreateCmd< ClearColorImageCmd, VkImageSubresourceRange >( ranges.size() );
		auto*	dst_ranges	= Cast<VkImageSubresourceRange>( AlignToLarger( static_cast< void *>(&cmd + 1), AlignOf<VkImageSubresourceRange> ));

		cmd.image		= image;
		cmd.layout		= VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		cmd.color		= color;
		cmd.rangeCount	= uint(ranges.size());
		MemCopy( OUT dst_ranges, ranges.data(), ArraySizeOf(ranges) );
	}
	
/*
=================================================
	ClearDepthStencilImage
=================================================
*/
	void  _VBaseIndirectTrasferCtx::ClearDepthStencilImage (VkImage image, const VkClearDepthStencilValue &depthStencil, ArrayView<VkImageSubresourceRange> ranges)
	{
		ASSERT( image != VK_NULL_HANDLE );
		ASSERT( ranges.size() );
		
		auto&	cmd			= _CreateCmd< ClearDepthStencilImageCmd, VkImageSubresourceRange >( ranges.size() );
		auto*	dst_ranges	= Cast<VkImageSubresourceRange>( AlignToLarger( static_cast< void *>(&cmd + 1), AlignOf<VkImageSubresourceRange> ));
		
		cmd.image			= image;
		cmd.layout			= VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		cmd.depthStencil	= depthStencil;
		cmd.rangeCount		= uint(ranges.size());
		MemCopy( OUT dst_ranges, ranges.data(), ArraySizeOf(ranges) );
	}
	
/*
=================================================
	FillBuffer
=================================================
*/
	void  _VBaseIndirectTrasferCtx::FillBuffer (VkBuffer buffer, Bytes offset, Bytes size, uint data)
	{
		ASSERT( buffer != VK_NULL_HANDLE );
		
		auto&	cmd	= _CreateCmd< FillBufferCmd >();
		cmd.buffer	= buffer;
		cmd.offset	= offset;
		cmd.size	= size;
		cmd.data	= data;
	}
	
/*
=================================================
	UpdateBuffer
=================================================
*/
	void  _VBaseIndirectTrasferCtx::UpdateBuffer (VkBuffer buffer, Bytes offset, Bytes size, const void* data)
	{
		ASSERT( buffer != VK_NULL_HANDLE );
		ASSERT(!"use UploadBuffer instead");
		
		auto&	cmd	= _CreateCmd< UpdateBufferCmd, ubyte >( usize(size) );
		auto*	dst	= static_cast< void *>(&cmd + 1);
		cmd.buffer	= buffer;
		cmd.offset	= offset;
		cmd.size	= CheckCast< TBytes<uint> >( size );
		MemCopy( OUT dst, data, size );

	}
	
/*
=================================================
	CopyBuffer
=================================================
*/
	void  _VBaseIndirectTrasferCtx::CopyBuffer (VkBuffer srcBuffer, VkBuffer dstBuffer, ArrayView<VkBufferCopy> ranges)
	{
		ASSERT( srcBuffer != VK_NULL_HANDLE );
		ASSERT( dstBuffer != VK_NULL_HANDLE );
		ASSERT( ranges.size() );
		
		auto&	cmd			= _CreateCmd< CopyBufferCmd, VkBufferCopy >( ranges.size() );
		auto*	dst_ranges	= Cast<VkBufferCopy>( AlignToLarger( static_cast< void *>(&cmd + 1), AlignOf<VkBufferCopy> ));

		cmd.srcBuffer	= srcBuffer;
		cmd.dstBuffer	= dstBuffer;
		cmd.regionCount	= uint(ranges.size());
		MemCopy( OUT dst_ranges, ranges.data(), ArraySizeOf(ranges) );
	}
	
/*
=================================================
	CopyImage
=================================================
*/
	void  _VBaseIndirectTrasferCtx::CopyImage (VkImage srcImage, VkImage dstImage, ArrayView<VkImageCopy> ranges)
	{
		ASSERT( srcImage != VK_NULL_HANDLE );
		ASSERT( dstImage != VK_NULL_HANDLE );
		ASSERT( ranges.size() );
		
		auto&	cmd			= _CreateCmd< CopyImageCmd, VkImageCopy >( ranges.size() );
		auto*	dst_ranges	= Cast<VkImageCopy>( AlignToLarger( static_cast< void *>(&cmd + 1), AlignOf<VkImageCopy> ));

		cmd.srcImage	= srcImage;
		cmd.srcLayout	= VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		cmd.dstImage	= dstImage;
		cmd.dstLayout	= VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		cmd.regionCount	= uint(ranges.size());
		MemCopy( OUT dst_ranges, ranges.data(), ArraySizeOf(ranges) );
	}
	
/*
=================================================
	CopyBufferToImage
=================================================
*/
	void  _VBaseIndirectTrasferCtx::CopyBufferToImage (VkBuffer srcBuffer, VkImage dstImage, ArrayView<VkBufferImageCopy> ranges)
	{
		ASSERT( srcBuffer != VK_NULL_HANDLE );
		ASSERT( dstImage != VK_NULL_HANDLE );
		ASSERT( ranges.size() );
		
		auto&	cmd			= _CreateCmd< CopyBufferToImageCmd, VkBufferImageCopy >( ranges.size() );
		auto*	dst_ranges	= Cast<VkBufferImageCopy>( AlignToLarger( static_cast< void *>(&cmd + 1), AlignOf<VkBufferImageCopy> ));

		cmd.srcBuffer	= srcBuffer;
		cmd.dstImage	= dstImage;
		cmd.dstLayout	= VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		cmd.regionCount	= uint(ranges.size());
		MemCopy( OUT dst_ranges, ranges.data(), ArraySizeOf(ranges) );
	}
	
/*
=================================================
	CopyImageToBuffer
=================================================
*/
	void  _VBaseIndirectTrasferCtx::CopyImageToBuffer (VkImage srcImage, VkBuffer dstBuffer, ArrayView<VkBufferImageCopy> ranges)
	{
		ASSERT( srcImage != VK_NULL_HANDLE );
		ASSERT( dstBuffer != VK_NULL_HANDLE );
		ASSERT( ranges.size() );
		
		auto&	cmd			= _CreateCmd< CopyImageToBufferCmd, VkBufferImageCopy >( ranges.size() );
		auto*	dst_ranges	= Cast<VkBufferImageCopy>( AlignToLarger( static_cast< void *>(&cmd + 1), AlignOf<VkBufferImageCopy> ));

		cmd.srcImage	= srcImage;
		cmd.srcLayout	= VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		cmd.dstBuffer	= dstBuffer;
		cmd.regionCount	= uint(ranges.size());
		MemCopy( OUT dst_ranges, ranges.data(), ArraySizeOf(ranges) );
	}

/*
=================================================
	_DebugMarker
=================================================
*/
	void  _VBaseIndirectTrasferCtx::_DebugMarker (StringView text, RGBA8u color)
	{
		auto&	cmd	= _CreateCmd< DebugMarkerCmd, char >( text.size() + 1 );
		auto*	str	= Cast<char>( &cmd + 1 );

		cmd.color = color;
		MemCopy( OUT str, text.data(), Bytes{text.size()} );
		str[text.size()] = '\0';
	}
	
/*
=================================================
	_PushDebugGroup
=================================================
*/
	void  _VBaseIndirectTrasferCtx::_PushDebugGroup (StringView text, RGBA8u color)
	{
		auto&	cmd	= _CreateCmd< PushDebugGroupCmd, char >( text.size() + 1 );
		auto*	str	= Cast<char>( &cmd + 1 );

		cmd.color = color;
		MemCopy( OUT str, text.data(), Bytes{text.size()} );
		str[text.size()] = '\0';
	}
	
/*
=================================================
	_PopDebugGroup
=================================================
*/
	void  _VBaseIndirectTrasferCtx::_PopDebugGroup ()
	{
		Unused( _CreateCmd< PopDebugGroupCmd >());
	}
	
/*
=================================================
	_CommitBarriers
=================================================
*/
	void  _VBaseIndirectTrasferCtx::_CommitBarriers (VBarrierManager::PipelineBarrier &barrier)
	{
		constexpr usize		align = Max( alignof(VkMemoryBarrier), alignof(VkBufferMemoryBarrier), alignof(VkImageMemoryBarrier) );

		Bytes	size = SizeOf<PipelineBarrierCmd>;
		size = barrier.memoryBarrierCount ? AlignToLarger( size, align ) + sizeof(VkMemoryBarrier)       * barrier.memoryBarrierCount : size;
		size = barrier.bufferBarrierCount ? AlignToLarger( size, align ) + sizeof(VkBufferMemoryBarrier) * barrier.bufferBarrierCount : size;
		size = barrier.imageBarrierCount  ? AlignToLarger( size, align ) + sizeof(VkImageMemoryBarrier)  * barrier.imageBarrierCount  : size;
		size = AlignToLarger( size, _MaxCmdAlign1 );

		auto*	cmd = Cast<PipelineBarrierCmd>( _allocator.Alloc( size, Bytes{_MaxCmdAlign1} ));
		void*	ptr = AlignToLarger( static_cast< void *>(cmd + 1), align );

		DEBUG_ONLY(
			cmd->_magicNumber = BaseCmd::MAGIC;
		)

		cmd->_commandID = CheckCast<ushort>( TransferCommands_t::Index< PipelineBarrierCmd >);
		cmd->_size		= CheckCast<ushort>( size );

		cmd->srcStageMask		= barrier.srcStageMask;
		cmd->dstStageMask		= barrier.dstStageMask;
		cmd->memoryBarrierCount	= barrier.memoryBarrierCount;
		cmd->bufferBarrierCount	= barrier.bufferBarrierCount;
		cmd->imageBarrierCount	= barrier.imageBarrierCount;
		
		MemCopy( OUT ptr, barrier.memoryBarriers, SizeOf<VkMemoryBarrier> * barrier.memoryBarrierCount );
		ptr = AlignToLarger( ptr + SizeOf<VkMemoryBarrier> * barrier.memoryBarrierCount, align );

		MemCopy( OUT ptr, barrier.bufferBarriers, SizeOf<VkBufferMemoryBarrier> * barrier.bufferBarrierCount );
		ptr = AlignToLarger( ptr + SizeOf<VkBufferMemoryBarrier> * barrier.bufferBarrierCount, align );

		MemCopy( OUT ptr, barrier.imageBarriers, SizeOf<VkImageMemoryBarrier> * barrier.imageBarrierCount );
		ptr = AlignToLarger( ptr + SizeOf<VkImageMemoryBarrier> * barrier.imageBarrierCount, align );
		
		ASSERT( _Validate( 100 ));
	}
	
/*
=================================================
	_Execute
=================================================
*/
	bool  _VBaseIndirectTrasferCtx::_Execute (VulkanDeviceFn fn, VkCommandBuffer cmdbuf, void* root)
	{
		BaseCmd const*	base = Cast<BaseCmd>( root );

		if ( base == null )
			return false;

		for (; base;)
		{
			if ( _ProcessTransferCmd( fn, cmdbuf, base ))
			{
				base = static_cast<const BaseCmd *>( static_cast<const void *>(base) + Bytes{base->_size} );
			}
			else
			if ( base->_commandID == TransferCommands_t::Index<EndCmd> )
			{
				break;
			}
			else
			{
				AE_LOGE( "unknown indirect buffer command" );
				return false;
			}
		}
		return true;
	}
	
/*
=================================================
	_ProcessTransferCmd
=================================================
*/
	bool  _VBaseIndirectTrasferCtx::_ProcessTransferCmd (VulkanDeviceFn fn, VkCommandBuffer cmdbuf, const BaseCmd* base)
	{
		ASSERT( base->_magicNumber == BaseCmd::MAGIC );

		switch ( base->_commandID )
		{
			case TransferCommands_t::Index<ClearColorImageCmd> :
			{
				auto&	cmd		= *Cast<ClearColorImageCmd>( base );
				auto*	ranges	= Cast<VkImageSubresourceRange>( AlignToLarger( static_cast< const void *>(&cmd + 1), AlignOf<VkImageSubresourceRange> ));
				fn.vkCmdClearColorImage( cmdbuf, cmd.image, cmd.layout, &cmd.color, cmd.rangeCount, ranges );
				return true;
			}

			case TransferCommands_t::Index<ClearDepthStencilImageCmd> :
			{
				auto&	cmd		= *Cast<ClearDepthStencilImageCmd>( base );
				auto*	ranges	= Cast<VkImageSubresourceRange>( AlignToLarger( static_cast< const void *>(&cmd + 1), AlignOf<VkImageSubresourceRange> ));
				fn.vkCmdClearDepthStencilImage( cmdbuf, cmd.image, cmd.layout, &cmd.depthStencil, cmd.rangeCount, ranges );
				return true;
			}

			case TransferCommands_t::Index<FillBufferCmd> :
			{
				auto&	cmd = *Cast<FillBufferCmd>( base );
				fn.vkCmdFillBuffer( cmdbuf, cmd.buffer, VkDeviceSize(cmd.offset), VkDeviceSize(cmd.size), cmd.data );
				return true;
			}

			case TransferCommands_t::Index<UpdateBufferCmd> :
			{
				auto&	cmd		= *Cast<UpdateBufferCmd>( base );
				auto*	data	= Cast<ubyte>( AlignToLarger( static_cast< const void *>(&cmd + 1), AlignOf<ubyte> ));
				fn.vkCmdUpdateBuffer( cmdbuf, cmd.buffer, VkDeviceSize(cmd.offset), VkDeviceSize(cmd.size), data );
				return true;
			}

			case TransferCommands_t::Index<CopyBufferCmd> :
			{
				auto&	cmd		= *Cast<CopyBufferCmd>( base );
				auto*	regions	= Cast<VkBufferCopy>( AlignToLarger( static_cast< const void *>(&cmd + 1), AlignOf<VkBufferCopy> ));
				fn.vkCmdCopyBuffer( cmdbuf, cmd.srcBuffer, cmd.dstBuffer, cmd.regionCount, regions );
				return true;
			}

			case TransferCommands_t::Index<CopyImageCmd> :
			{
				auto&	cmd		= *Cast<CopyImageCmd>( base );
				auto*	regions	= Cast<VkImageCopy>( AlignToLarger( static_cast<const void *>(&cmd + 1), AlignOf<VkImageCopy> ));
				fn.vkCmdCopyImage( cmdbuf, cmd.srcImage, cmd.srcLayout, cmd.dstImage, cmd.dstLayout, cmd.regionCount, regions );
				return true;
			}

			case TransferCommands_t::Index<CopyBufferToImageCmd> :
			{
				auto&	cmd		= *Cast<CopyBufferToImageCmd>( base );
				auto*	regions	= Cast<VkBufferImageCopy>( AlignToLarger( static_cast< const void *>(&cmd + 1), AlignOf<VkBufferImageCopy> ));
				fn.vkCmdCopyBufferToImage( cmdbuf, cmd.srcBuffer, cmd.dstImage, cmd.dstLayout, cmd.regionCount, regions );
				return true;
			}

			case TransferCommands_t::Index<CopyImageToBufferCmd> :
			{
				auto&	cmd		= *Cast<CopyImageToBufferCmd>( base );
				auto*	regions	= Cast<VkBufferImageCopy>( AlignToLarger( static_cast< const void *>(&cmd + 1), AlignOf<VkBufferImageCopy> ));
				fn.vkCmdCopyImageToBuffer( cmdbuf, cmd.srcImage, cmd.srcLayout, cmd.dstBuffer, cmd.regionCount, regions );
				return true;
			}

			case TransferCommands_t::Index<DebugMarkerCmd> :
			{
				auto&	cmd		= *Cast<DebugMarkerCmd>( base );
				auto*	text	= Cast<char>( AlignToLarger( static_cast< const void *>(&cmd + 1), AlignOf<char> ));
				
				VkDebugUtilsLabelEXT	info = {};
				info.sType		= VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
				info.pLabelName	= text;
				MemCopy( info.color, RGBA32f{cmd.color} );
				fn.vkCmdInsertDebugUtilsLabelEXT( cmdbuf, &info );
				return true;
			}

			case TransferCommands_t::Index<PushDebugGroupCmd> :
			{
				auto&	cmd		= *Cast<PushDebugGroupCmd>( base );
				auto*	text	= Cast<char>( AlignToLarger( static_cast< const void *>(&cmd + 1), AlignOf<char> ));
				
				VkDebugUtilsLabelEXT	info = {};
				info.sType		= VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
				info.pLabelName	= text;
				MemCopy( info.color, RGBA32f{cmd.color} );
				fn.vkCmdBeginDebugUtilsLabelEXT( cmdbuf, &info );
				return true;
			}

			case TransferCommands_t::Index<PopDebugGroupCmd> :
			{
				fn.vkCmdEndDebugUtilsLabelEXT( cmdbuf );
				return true;
			}

			case TransferCommands_t::Index<PipelineBarrierCmd> :
			{
				constexpr usize		align = Max( alignof(VkMemoryBarrier), alignof(VkBufferMemoryBarrier), alignof(VkImageMemoryBarrier) );

				auto&	cmd		= *Cast<PipelineBarrierCmd>( base );
				auto*	memory	= Cast<VkMemoryBarrier>(       AlignToLarger( static_cast< const void *>(&cmd + 1),                         align ));
				auto*	buffers	= Cast<VkBufferMemoryBarrier>( AlignToLarger( static_cast< const void *>(memory + cmd.memoryBarrierCount),  align ));
				auto*	images	= Cast<VkImageMemoryBarrier>(  AlignToLarger( static_cast< const void *>(buffers + cmd.bufferBarrierCount), align ));
				fn.vkCmdPipelineBarrier( cmdbuf, cmd.srcStageMask, cmd.dstStageMask, 0,
										 cmd.memoryBarrierCount, cmd.memoryBarrierCount ? memory  : null,
										 cmd.bufferBarrierCount, cmd.bufferBarrierCount ? buffers : null,
										 cmd.imageBarrierCount,  cmd.imageBarrierCount  ? images  : null );
				return true;
			}
		}
		return false;
	}


}	// AE::Graphics::_hidden_

#endif	// AE_ENABLE_VULKAN
