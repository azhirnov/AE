// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef AE_ENABLE_VULKAN
# include "graphics/Vulkan/RenderGraph/VComputeContext.h"

namespace AE::Graphics::_hidden_
{
	
	
	//
	// Vulkan Direct Graphics Context implementation
	//

	template <typename ResMngrType>
	class _VDirectGraphicsCtx : public _VDirectComputeCtx<ResMngrType>
	{
	// methods
	public:
		explicit _VDirectGraphicsCtx (RC<VCommandBatch> batch) : _VDirectComputeCtx<ResMngrType>{batch} {}
		~_VDirectGraphicsCtx () override {}

		void  BlitImage (VkImage srcImage, VkImage dstImage, VkFilter filter, ArrayView<VkImageBlit> regions);
		void  ResolveImage (VkImage srcImage, VkImage dstImage, ArrayView<VkImageResolve> regions);
		
	protected:
		//void  _BeginRenderPass ();
		//void  _NextSubpass ();
		//void  _EndRenderPass ();
	};



	//
	// Vulkan Indirect Graphics Context implementation
	//
	
	class _VBaseIndirectGraphicsCtx : public _VBaseIndirectComputeCtx
	{
	// types
	protected:
		struct BlitImageCmd : BaseCmd
		{
			VkImage				srcImage;
			VkImageLayout		srcLayout;
			VkImage				dstImage;
			VkImageLayout		dstLayout;
			VkFilter			filter;
			uint				regionCount;
			//VkImageBlit		regions[];
		};
		
		struct ResolveImageCmd : BaseCmd
		{
			VkImage				srcImage;
			VkImageLayout		srcLayout;
			VkImage				dstImage;
			VkImageLayout		dstLayout;
			uint				regionCount;
			//VkImageResolve	regions[];
		};
		
		using GraphicsCommands_t = TypeListUtils::Merge< ComputeCommands_t, TypeList< BlitImageCmd, ResolveImageCmd >>;

		static constexpr usize	_MaxCmdAlign3	= GraphicsCommands_t::ForEach_Max< TypeListUtils::GetTypeAlign >();
		STATIC_ASSERT( _MaxCmdAlign2 == _MaxCmdAlign3 );
		STATIC_ASSERT( VBakedCommands::Allocator_t::Align >= _MaxCmdAlign3 );


	// methods
	public:
		explicit _VBaseIndirectGraphicsCtx (RC<VCommandBatch> batch) : _VBaseIndirectComputeCtx{batch} {}
		~_VBaseIndirectGraphicsCtx () override {}
		
		void  BlitImage (VkImage srcImage, VkImage dstImage, VkFilter filter, ArrayView<VkImageBlit> regions);
		void  ResolveImage (VkImage srcImage, VkImage dstImage, ArrayView<VkImageResolve> regions);


	protected:
		//void  _BeginRenderPass ();
		//void  _NextSubpass ();
		//void  _EndRenderPass ();

		static bool  _Execute (VulkanDeviceFn fn, VkCommandBuffer cmdbuf, void* root);
		static bool  _ProcessGraphicsCmd (VulkanDeviceFn fn, VkCommandBuffer cmdbuf, const BaseCmd* cmd);

		template <typename CmdType, typename ...DynamicTypes>
		ND_ CmdType&  _CreateCmd (usize dynamicArraySize = 0);
		
		template <usize I, typename TL>
		static constexpr Bytes  _CalcCmdSize (Bytes size, usize dynamicArraySize);
	};



	//
	// Vulkan Graphics Context implementation
	//
	
	template <typename CtxImpl, typename BaseInterface>
	class _VGraphicsContextImpl : public _VComputeContextImpl<CtxImpl, BaseInterface>
	{
	// types
	private:
		using BaseCtx	= _VComputeContextImpl< CtxImpl, BaseInterface >;
		using RawCtx	= CtxImpl;
		
	protected:
		static constexpr uint	_LocalArraySize			= BaseCtx::_LocalArraySize;

	public:
		static constexpr bool	IsGraphicsContext		= true;
		static constexpr bool	IsVulkanGraphicsContext	= true;
		static constexpr bool	HasAutoSync				= CtxImpl::HasAutoSync;
		

	// methods
	public:
		explicit _VGraphicsContextImpl (RC<VCommandBatch> batch) : BaseCtx{batch} {}
		~_VGraphicsContextImpl () override {}
		
		using RawCtx::BlitImage;
		using RawCtx::ResolveImage;

		void  BlitImage (ImageID srcImage, ImageID dstImage, EBlitFilter filter, ArrayView<ImageBlit> regions) override final;
		void  ResolveImage (ImageID srcImage, ImageID dstImage, ArrayView<ImageResolve> regions) override final;

		//void  BeginRenderPass (ArrayView<RenderPassDesc> passes) override final;
		//void  NextSubpass () override final;
		//void  EndRenderPass () override final;
	};

	
/*
=================================================
	BlitImage
=================================================
*/
	template <typename C, typename I>
	void  _VGraphicsContextImpl<C,I>::BlitImage (ImageID srcImage, ImageID dstImage, EBlitFilter blitFilter, ArrayView<ImageBlit> regions)
	{
		auto*	src_img = this->_resMngr.Get( srcImage );
		auto*	dst_img = this->_resMngr.Get( dstImage );
		CHECK_ERRV( src_img and dst_img );
		ASSERT( regions.size() );

		if constexpr( HasAutoSync )
		{
			this->_resMngr.AddImage( src_img, EResourceState::TransferSrc, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
									 StructView<ImageSubresourceLayers>( regions, &ImageBlit::srcSubresource ));
			
			this->_resMngr.AddImage( dst_img, EResourceState::TransferDst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
									 StructView<ImageSubresourceLayers>( regions, &ImageBlit::dstSubresource ));

			this->_resMngr.FlushBarriers();
		}
		this->CommitBarriers();

		FixedArray<VkImageBlit, _LocalArraySize>	vk_regions;
		const ImageDesc &							src_desc	= src_img->Description();
		const ImageDesc &							dst_desc	= dst_img->Description();
		const VkFilter								filter		= VEnumCast( blitFilter );

		for (usize i = 0; i < regions.size(); ++i)
		{
			auto&	src = regions[i];
			auto&	dst = vk_regions.emplace_back();

			ASSERT( All( src.srcOffset0 <= src_desc.dimension ));
			ASSERT( All( src.srcOffset1 <= src_desc.dimension ));
			ASSERT( All( src.dstOffset0 <= dst_desc.dimension ));
			ASSERT( All( src.dstOffset1 <= dst_desc.dimension ));

			dst.srcOffsets[0] = { int(src.srcOffset0.x), int(src.srcOffset0.y), int(src.srcOffset0.z) };
			dst.srcOffsets[1] = { int(src.srcOffset1.x), int(src.srcOffset1.y), int(src.srcOffset1.z) };
			dst.dstOffsets[0] = { int(src.dstOffset0.x), int(src.dstOffset0.y), int(src.dstOffset0.z) };
			dst.dstOffsets[1] = { int(src.dstOffset1.x), int(src.dstOffset1.y), int(src.dstOffset1.z) };
			this->_ConvertImageSubresourceLayer( OUT dst.srcSubresource, src.srcSubresource, src_desc );
			this->_ConvertImageSubresourceLayer( OUT dst.dstSubresource, src.dstSubresource, dst_desc );

			if ( vk_regions.size() == vk_regions.capacity() )
			{
				RawCtx::BlitImage( src_img->Handle(), dst_img->Handle(), filter, vk_regions );
				vk_regions.clear();
			}
		}
		
		if ( vk_regions.size() )
			RawCtx::BlitImage( src_img->Handle(), dst_img->Handle(), filter, vk_regions );
	}

/*
=================================================
	ResolveImage
=================================================
*/
	template <typename C, typename I>
	void  _VGraphicsContextImpl<C,I>::ResolveImage (ImageID srcImage, ImageID dstImage, ArrayView<ImageResolve> regions)
	{
		auto*	src_img = this->_resMngr.Get( srcImage );
		auto*	dst_img = this->_resMngr.Get( dstImage );
		CHECK_ERRV( src_img and dst_img );
		ASSERT( regions.size() );
		
		if constexpr( HasAutoSync )
		{
			this->_resMngr.AddImage( src_img, EResourceState::TransferSrc, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
									 StructView<ImageSubresourceLayers>( regions, &ImageResolve::srcSubresource ));

			this->_resMngr.AddImage( dst_img, EResourceState::TransferDst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
									 StructView<ImageSubresourceLayers>( regions, &ImageResolve::dstSubresource ));

			this->_resMngr.FlushBarriers();
		}
		this->CommitBarriers();

		FixedArray<VkImageResolve, _LocalArraySize>		vk_regions;
		const ImageDesc &								src_desc	= src_img->Description();
		const ImageDesc &								dst_desc	= dst_img->Description();

		for (usize i = 0; i < regions.size(); ++i)
		{
			auto&	src = regions[i];
			auto&	dst = vk_regions.emplace_back();
			
			ASSERT( All( src.srcOffset < src_desc.dimension ));
			ASSERT( All( (src.srcOffset + src.extent) <= src_desc.dimension ));
			ASSERT( All( src.dstOffset < dst_desc.dimension ));
			ASSERT( All( (src.dstOffset + src.extent) <= dst_desc.dimension ));

			dst.srcOffset	= { int(src.srcOffset.x), int(src.srcOffset.y), int(src.srcOffset.z) };
			dst.dstOffset	= { int(src.dstOffset.x), int(src.dstOffset.y), int(src.dstOffset.z) };
			dst.extent		= { src.extent.x,         src.extent.y,         src.extent.z };

			this->_ConvertImageSubresourceLayer( OUT dst.srcSubresource, src.srcSubresource, src_desc );
			this->_ConvertImageSubresourceLayer( OUT dst.dstSubresource, src.dstSubresource, dst_desc );
			
			if ( vk_regions.size() == vk_regions.capacity() )
			{
				RawCtx::ResolveImage( src_img->Handle(), dst_img->Handle(), vk_regions );
				vk_regions.clear();
			}
		}
		
		if ( vk_regions.size() )
			RawCtx::ResolveImage( src_img->Handle(), dst_img->Handle(), vk_regions );
	}
	
/*
=================================================
	BeginRenderPass
=================================================
*
	template <typename C, typename I>
	void  _VGraphicsContextImpl<C,I>::BeginRenderPass (ArrayView<RenderPassDesc>)
	{
		RawCtx::_BeginRenderPass();
	}
	
/*
=================================================
	NextSubpass
=================================================
*
	template <typename C, typename I>
	void  _VGraphicsContextImpl<C,I>::NextSubpass ()
	{
		RawCtx::_NextSubpass();
	}
	
/*
=================================================
	EndRenderPass
=================================================
*
	template <typename C, typename I>
	void  _VGraphicsContextImpl<C,I>::EndRenderPass ()
	{
		RawCtx::_EndRenderPass();
	}
//-----------------------------------------------------------------------------


	
/*
=================================================
	BlitImage
=================================================
*/
	template <typename RM>
	void  _VDirectGraphicsCtx<RM>::BlitImage (VkImage srcImage, VkImage dstImage, VkFilter filter, ArrayView<VkImageBlit> regions)
	{
		ASSERT( srcImage != VK_NULL_HANDLE );
		ASSERT( dstImage != VK_NULL_HANDLE );
		ASSERT( regions.size() );

		this->vkCmdBlitImage( this->_cmdbuf.Get(), srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
							  dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
							  uint(regions.size()), regions.data(), filter );
	}
	
/*
=================================================
	ResolveImage
=================================================
*/
	template <typename RM>
	void  _VDirectGraphicsCtx<RM>::ResolveImage (VkImage srcImage, VkImage dstImage, ArrayView<VkImageResolve> regions)
	{
		ASSERT( srcImage != VK_NULL_HANDLE );
		ASSERT( dstImage != VK_NULL_HANDLE );
		ASSERT( regions.size() );

		this->vkCmdResolveImage( this->_cmdbuf.Get(), srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
								 dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
								 uint(regions.size()), regions.data() );
	}

	
}	// AE::Graphics::_hidden_

#endif	// AE_ENABLE_VULKAN
