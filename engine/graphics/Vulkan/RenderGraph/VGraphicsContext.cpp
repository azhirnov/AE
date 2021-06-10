// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#ifdef AE_ENABLE_VULKAN
# include "graphics/Vulkan/RenderGraph/VGraphicsContext.h"

namespace AE::Graphics::_hidden_
{
	
/*
=================================================
	_CreateCmd
=================================================
*/
	template <typename CmdType, typename ...DynamicTypes>
	CmdType&  _VBaseIndirectGraphicsCtx::_CreateCmd (usize dynamicArraySize)
	{
		Bytes	size	= AlignToLarger( _CalcCmdSize< 0, TypeList<DynamicTypes...> >( SizeOf<CmdType>, dynamicArraySize ), _MaxCmdAlign3 );
		auto*	cmd		= Cast<CmdType>( _allocator.Alloc( size, Bytes{_MaxCmdAlign3} ));
		DEBUG_ONLY(
			cmd->_magicNumber = BaseCmd::MAGIC;
		)

		cmd->_commandID = CheckCast<ushort>( GraphicsCommands_t::Index< CmdType >);
		cmd->_size		= CheckCast<ushort>( size );
		return *cmd;
	}
	
/*
=================================================
	_CalcCmdSize
=================================================
*/
	template <usize I, typename TL>
	constexpr Bytes  _VBaseIndirectGraphicsCtx::_CalcCmdSize (Bytes size, usize dynamicArraySize)
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
	BlitImage
=================================================
*/
	void  _VBaseIndirectGraphicsCtx::BlitImage (VkImage srcImage, VkImage dstImage, VkFilter filter, ArrayView<VkImageBlit> regions)
	{
		ASSERT( srcImage != VK_NULL_HANDLE );
		ASSERT( dstImage != VK_NULL_HANDLE );
		ASSERT( regions.size() );
		
		auto&	cmd	= _CreateCmd< BlitImageCmd, VkImageBlit >( regions.size() );
		auto*	dst	= Cast<VkImageBlit>( AlignToLarger( static_cast<void*>(&cmd + 1), AlignOf<VkImageBlit> ));

		cmd.srcImage	= srcImage;
		cmd.srcLayout	= VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		cmd.dstImage	= dstImage;
		cmd.dstLayout	= VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		cmd.regionCount	= uint(regions.size());
		cmd.filter		= filter;
		MemCopy( OUT dst, regions.data(), ArraySizeOf(regions) );
	}
	
/*
=================================================
	ResolveImage
=================================================
*/
	void  _VBaseIndirectGraphicsCtx::ResolveImage (VkImage srcImage, VkImage dstImage, ArrayView<VkImageResolve> regions)
	{
		ASSERT( srcImage != VK_NULL_HANDLE );
		ASSERT( dstImage != VK_NULL_HANDLE );
		ASSERT( regions.size() );
		
		auto&	cmd	= _CreateCmd< ResolveImageCmd, VkImageBlit >( regions.size() );
		auto*	dst	= Cast<VkImageResolve>( AlignToLarger( static_cast<void*>(&cmd + 1), AlignOf<VkImageResolve> ));
		
		cmd.srcImage	= srcImage;
		cmd.srcLayout	= VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		cmd.dstImage	= dstImage;
		cmd.dstLayout	= VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		cmd.regionCount	= uint(regions.size());
		MemCopy( OUT dst, regions.data(), ArraySizeOf(regions) );
	}

/*
=================================================
	_Execute
=================================================
*/
	bool  _VBaseIndirectGraphicsCtx::_Execute (VulkanDeviceFn fn, VkCommandBuffer cmdbuf, void* root)
	{
		BaseCmd const*	base = Cast<BaseCmd>( root );

		if ( base == null )
			return false;

		for (; base;)
		{
			if ( _ProcessTransferCmd( fn, cmdbuf, base ) or
				 _ProcessComputeCmd(  fn, cmdbuf, base ) or
				 _ProcessGraphicsCmd( fn, cmdbuf, base ))
			{
				base = static_cast<const BaseCmd *>( static_cast<const void *>(base) + Bytes{base->_size} );
			}
			else
			if ( base->_commandID == GraphicsCommands_t::Index<EndCmd> )
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
	_ProcessGraphicsCmd
=================================================
*/
	bool  _VBaseIndirectGraphicsCtx::_ProcessGraphicsCmd (VulkanDeviceFn fn, VkCommandBuffer cmdbuf, const BaseCmd* base)
	{
		switch ( base->_commandID )
		{
			case GraphicsCommands_t::Index<BlitImageCmd> :
			{
				auto&	cmd		= *Cast<BlitImageCmd>( base );
				auto*	regions = Cast<VkImageBlit>( AlignToLarger( static_cast<const void *>(&cmd + 1), AlignOf<VkImageBlit> ));
				fn.vkCmdBlitImage( cmdbuf, cmd.srcImage, cmd.srcLayout, cmd.dstImage, cmd.dstLayout, cmd.regionCount, regions, cmd.filter );
				return true;
			}

			case GraphicsCommands_t::Index<ResolveImageCmd> :
			{
				auto&	cmd = *Cast<ResolveImageCmd>( base );
				auto*	regions = Cast<VkImageResolve>( AlignToLarger( static_cast<const void *>(&cmd + 1), AlignOf<VkImageResolve> ));
				fn.vkCmdResolveImage( cmdbuf, cmd.srcImage, cmd.srcLayout, cmd.dstImage, cmd.dstLayout, cmd.regionCount, regions );
				return true;
			}
		}
		return false;
	}


}	// AE::Graphics::_hidden_

#endif	// AE_ENABLE_VULKAN
