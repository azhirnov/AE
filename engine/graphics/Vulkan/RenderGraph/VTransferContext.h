// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef AE_ENABLE_VULKAN
# include "graphics/Public/CommandBuffer.h"
# include "graphics/Vulkan/VRenderGraph.h"
# include "graphics/Vulkan/VEnumCast.h"
# include "graphics/Vulkan/RenderGraph/VBakedCommands.h"
# include "graphics/Vulkan/RenderGraph/VBarrierManager.h"

namespace AE::Graphics::_hidden_
{

	//
	// Vulkan Direct Transfer Context implementation
	//
	
	template <typename ResMngrType>
	class _VDirectTransferCtx : protected VulkanDeviceFn
	{
	// types
	public:
		static constexpr bool	IsIndirectContext	= false;
		static constexpr bool	HasAutoSync			= ResMngrType::HasAutoSync;
		
	// variables
	protected:
		VCommandBuffer	_cmdbuf;
		ResMngrType		_resMngr;


	// methods
	public:
		explicit _VDirectTransferCtx (RC<VCommandBatch> batch);
		virtual ~_VDirectTransferCtx ();

		ND_ bool			IsValid ()				const	{ return _cmdbuf.IsValid(); }
		ND_ VkCommandBuffer	ReleaseCommandBuffer ();

		void  ClearColorImage (VkImage image, const VkClearColorValue &color, ArrayView<VkImageSubresourceRange> ranges);
		void  ClearDepthStencilImage (VkImage image, const VkClearDepthStencilValue &depthStencil, ArrayView<VkImageSubresourceRange> ranges);

		void  FillBuffer (VkBuffer buffer, Bytes offset, Bytes size, uint data);
		void  UpdateBuffer (VkBuffer buffer, Bytes offset, Bytes size, const void* data);

		void  CopyBuffer (VkBuffer srcBuffer, VkBuffer dstBuffer, ArrayView<VkBufferCopy> ranges);
		void  CopyImage (VkImage srcImage, VkImage dstImage, ArrayView<VkImageCopy> ranges);

		void  CopyBufferToImage (VkBuffer srcBuffer, VkImage dstImage, ArrayView<VkBufferImageCopy> ranges);
		void  CopyImageToBuffer (VkImage srcImage, VkBuffer dstBuffer, ArrayView<VkBufferImageCopy> ranges);

		void  CommitBarriers ();
		
	protected:
		// for debugging
		void  _DebugMarker (StringView text, RGBA8u color);
		void  _PushDebugGroup (StringView text, RGBA8u color);
		void  _PopDebugGroup ();
	};



	//
	// Vulkan Indirect Transfer Context implementation
	//
	
	class _VBaseIndirectTrasferCtx
	{
	// types
	public:
		static constexpr bool	IsIndirectContext	= true;
		
	protected:
		struct BaseCmd
		{
			DEBUG_ONLY(
				static constexpr uint	MAGIC = 0x762a3cf0;
				uint	_magicNumber = MAGIC;
			)
			ushort	_commandID	= 0;
			ushort	_size		= 0;
		};

		struct EndCmd : BaseCmd
		{};
		
		struct ClearColorImageCmd : BaseCmd
		{
			VkImage						image;
			VkImageLayout				layout;
			VkClearColorValue			color;
			uint						rangeCount;
			//VkImageSubresourceRange	ranges[];
		};
		
		struct ClearDepthStencilImageCmd : BaseCmd
		{
			VkImage						image;
			VkImageLayout				layout;
			VkClearDepthStencilValue	depthStencil;
			uint						rangeCount;
			//VkImageSubresourceRange	ranges[];
		};
		
		struct FillBufferCmd : BaseCmd
		{
			VkBuffer			buffer;
			Bytes				offset;
			Bytes				size;
			uint				data;
		};
		
		struct UpdateBufferCmd : BaseCmd
		{
			VkBuffer			buffer;
			Bytes				offset;
			TBytes<uint>		size;
			//ubyte				data[];
		};
		
		struct CopyBufferCmd : BaseCmd
		{
			VkBuffer			srcBuffer;
			VkBuffer			dstBuffer;
			uint				regionCount;
			//VkBufferCopy		regions[];
		};
		
		struct CopyImageCmd : BaseCmd
		{
			VkImage				srcImage;
			VkImageLayout		srcLayout;
			VkImage				dstImage;
			VkImageLayout		dstLayout;
			uint				regionCount;
			//VkImageCopy		regions[];
		};
		
		struct CopyBufferToImageCmd : BaseCmd
		{
			VkBuffer			srcBuffer;
			VkImage				dstImage;
			VkImageLayout		dstLayout;
			uint				regionCount;
			//VkBufferImageCopy	regions[];
		};
		
		struct CopyImageToBufferCmd : BaseCmd
		{
			VkImage				srcImage;
			VkImageLayout		srcLayout;
			VkBuffer			dstBuffer;
			uint				regionCount;
			//VkBufferImageCopy	regions[];
		};
		
		struct DebugMarkerCmd : BaseCmd
		{
			RGBA8u		color;
			//char		text[];
		};
		
		struct PushDebugGroupCmd : BaseCmd
		{
			RGBA8u		color;
			//char		text[];
		};
		
		struct PopDebugGroupCmd : BaseCmd
		{
		};
		
		struct PipelineBarrierCmd : BaseCmd
		{
			VkPipelineStageFlagBits		srcStageMask;
			VkPipelineStageFlagBits		dstStageMask;
			uint						memoryBarrierCount;
			uint						bufferBarrierCount;
			uint						imageBarrierCount;
			//VkMemoryBarrier			memoryBarriers[];
			//VkBufferMemoryBarrier		bufferBarriers[];
			//VkImageMemoryBarrier		imageBarriers[];
		};

		using TransferCommands_t = TypeList< EndCmd, ClearColorImageCmd, ClearDepthStencilImageCmd, FillBufferCmd, UpdateBufferCmd, CopyBufferCmd, CopyImageCmd,
											 CopyBufferToImageCmd, CopyImageToBufferCmd, DebugMarkerCmd, PushDebugGroupCmd, PopDebugGroupCmd, PipelineBarrierCmd >;
		
		static constexpr usize	_MaxCmdAlign1	= TransferCommands_t::ForEach_Max< TypeListUtils::GetTypeAlign >();
		STATIC_ASSERT( VBakedCommands::Allocator_t::Align >= _MaxCmdAlign1 );


	// variables
	protected:
		LinearAllocator<>	_allocator;


	// methods
	public:
		explicit _VBaseIndirectTrasferCtx (RC<VCommandBatch> batch);
		virtual ~_VBaseIndirectTrasferCtx ();
		
		ND_ bool  IsValid ()	const	{ return true; }

		void  ClearColorImage (VkImage image, const VkClearColorValue &color, ArrayView<VkImageSubresourceRange> ranges);
		void  ClearDepthStencilImage (VkImage image, const VkClearDepthStencilValue &depthStencil, ArrayView<VkImageSubresourceRange> ranges);

		void  FillBuffer (VkBuffer buffer, Bytes offset, Bytes size, uint data);
		void  UpdateBuffer (VkBuffer buffer, Bytes offset, Bytes size, const void* data);

		void  CopyBuffer (VkBuffer srcBuffer, VkBuffer dstBuffer, ArrayView<VkBufferCopy> ranges);
		void  CopyImage (VkImage srcImage, VkImage dstImage, ArrayView<VkImageCopy> ranges);

		void  CopyBufferToImage (VkBuffer srcBuffer, VkImage dstImage, ArrayView<VkBufferImageCopy> ranges);
		void  CopyImageToBuffer (VkImage srcImage, VkBuffer dstBuffer, ArrayView<VkBufferImageCopy> ranges);


	protected:
		ND_ VBakedCommands	_Prepare (VBakedCommands::ExecuteFn_t fn);

		bool  _Validate (const void* ptr, uint maxCommandID) const;
		bool  _Validate (uint maxCommandID = 100) const;

		void  _DebugMarker (StringView text, RGBA8u color);
		void  _PushDebugGroup (StringView text, RGBA8u color);
		void  _PopDebugGroup ();
		void  _CommitBarriers (VBarrierManager::PipelineBarrier &);

		static bool  _Execute (VulkanDeviceFn fn, VkCommandBuffer cmdbuf, void* root);
		static bool  _ProcessTransferCmd (VulkanDeviceFn fn, VkCommandBuffer cmdbuf, const BaseCmd* cmd);

		template <typename CmdType, typename ...DynamicTypes>
		ND_ CmdType&  _CreateCmd (usize dynamicArraySize = 0);
		
		template <usize I, typename TL>
		static constexpr Bytes  _CalcCmdSize (Bytes size, usize dynamicArraySize);
	};
	

	template <typename ResMngrType, typename BaseCtx>
	class _VIndirectCtxHelper : public BaseCtx
	{
	// types
	public:
		static constexpr bool	HasAutoSync	= ResMngrType::HasAutoSync;

	// variables
	protected:
		ResMngrType		_resMngr;
	
	// methods
	public:
		explicit _VIndirectCtxHelper (RC<VCommandBatch> batch) : BaseCtx{batch}, _resMngr{*batch} {}

		void  CommitBarriers ();
		
		ND_ VBakedCommands	Prepare ();
	};


	
	//
	// Vulkan Transfer Context implementation
	//

	template <typename CtxImpl, typename BaseInterface>
	class _VTransferContextImpl : public CtxImpl, public BaseInterface
	{
	// types
	public:
		static constexpr bool	IsTransferContext		= true;
		static constexpr bool	IsVulkanTransferContext	= true;
		
		static constexpr bool	HasAutoSync				= CtxImpl::HasAutoSync;

	protected:
		static constexpr uint	_LocalArraySize			= 16;

	private:
		using RawCtx	= CtxImpl;
		

	// variables
	private:
		RC<VCommandBatch>	_batch;


	// methods
	public:
		explicit _VTransferContextImpl (RC<VCommandBatch> batch);
		_VTransferContextImpl () = delete;
		_VTransferContextImpl (const _VTransferContextImpl &) = delete;
		~_VTransferContextImpl () override {}

		// call CommitBarriers() to flush accumulated barriers
		void  ImageBarrier (ImageID image, EResourceState srcState, EResourceState dstState);
		void  ImageBarrier (ImageID image, EResourceState srcState, EResourceState dstState, const ImageSubresourceRange &subRes);
		void  ImageBarrier (VkPipelineStageFlagBits srcStage, VkPipelineStageFlagBits dstStage, const VkImageMemoryBarrier &barrier);
		
		void  BufferBarrier (BufferID buffer, EResourceState srcState, EResourceState dstState);
		void  BufferBarrier (BufferID buffer, EResourceState srcState, EResourceState dstState, Bytes offset, Bytes size);
		void  BufferBarrier (VkPipelineStageFlagBits srcStage, VkPipelineStageFlagBits dstStage, const VkBufferMemoryBarrier &barrier);

		using RawCtx::CommitBarriers;

		void  SetInitialState (BufferID id, EResourceState state) override final;
		void  SetInitialState (ImageID id, EResourceState state) override final;
		void  SetInitialState (ImageID id, EResourceState state, EImageLayout layout) override final;
		
		using RawCtx::ClearColorImage;
		using RawCtx::ClearDepthStencilImage;

		void  ClearColorImage (ImageID image, const ClearColor_t &color, ArrayView<ImageSubresourceRange> ranges) override final;
		void  ClearDepthStencilImage (ImageID image, const DepthStencil &depthStencil, ArrayView<ImageSubresourceRange> ranges) override final;

		using RawCtx::FillBuffer;
		using RawCtx::UpdateBuffer;

		void  FillBuffer (BufferID buffer, Bytes offset, Bytes size, uint data) override final;
		void  UpdateBuffer (BufferID buffer, Bytes offset, Bytes size, const void* data) override final;

		using RawCtx::CopyBuffer;
		using RawCtx::CopyImage;

		void  CopyBuffer (BufferID srcBuffer, BufferID dstBuffer, ArrayView<BufferCopy> ranges) override final;
		void  CopyImage (ImageID srcImage, ImageID dstImage, ArrayView<ImageCopy> ranges) override final;

		using RawCtx::CopyBufferToImage;
		using RawCtx::CopyImageToBuffer;

		void  CopyBufferToImage (BufferID srcBuffer, ImageID dstImage, ArrayView<BufferImageCopy> ranges) override final;
		void  CopyImageToBuffer (ImageID srcImage, BufferID dstBuffer, ArrayView<BufferImageCopy> ranges) override final;
		
		bool  UploadBuffer (BufferID buffer, Bytes offset, Bytes size, const void* data, EStagingHeapType heapType = EStagingHeapType::Static) override final;
		bool  UploadBuffer (BufferID buffer, Bytes offset, Bytes size, OUT BufferView &ranges, EStagingHeapType heapType = EStagingHeapType::Static) override final;
		
		//bool  UploadBuffer (BufferStream &stream, OUT BufferView &ranges, EStagingHeapType heapType = EStagingHeapType::Static) override final;
		//bool  UploadImage (ImageStream &stream, OUT ImageView &ranges, EStagingHeapType heapType = EStagingHeapType::Static) override final;

		ND_ Promise<BufferView>  ReadBuffer (BufferID buffer, Bytes offset, Bytes size, EStagingHeapType heapType = EStagingHeapType::Static) override final;

		//ND_ Promise<ImageView>   ReadImage (ImageID image, const uint3 &offset, const uint3 &size, MipmapLevel mipLevel,
		//									ImageLayer arrayLayer, EImageAspect aspectMask, EStagingHeapType heapType = EStagingHeapType::Static) override final;
		
		bool  UpdateHostBuffer (BufferID buffer, Bytes offset, Bytes size, const void* data) override final;
		bool  MapHostBuffer (BufferID buffer, Bytes offset, INOUT Bytes &size, OUT void* &mapped) override final;

		void  GlobalBarrier ();
		
		// for debugging
		void  DebugMarker (StringView text, RGBA8u color = RGBA8u{255});
		void  PushDebugGroup (StringView text, RGBA8u color = RGBA8u{255});
		void  PopDebugGroup ();

	protected:
		static void  _ConvertImageSubresourceRange (OUT VkImageSubresourceRange& dst, const ImageSubresourceRange& src, const ImageDesc &desc);
		static void  _ConvertImageSubresourceLayer (OUT VkImageSubresourceLayers &dst, const ImageSubresourceLayers &src, const ImageDesc &desc);
		static void  _ConvertBufferImageCopy (OUT VkBufferImageCopy& dst, const BufferImageCopy& src, const ImageDesc &desc);
	};
//-----------------------------------------------------------------------------


	
/*
=================================================
	constructor
=================================================
*/
	template <typename C, typename I>
	_VTransferContextImpl<C,I>::_VTransferContextImpl (RC<VCommandBatch> batch) :
		RawCtx{ batch },
		_batch{ batch }
	{}

/*
=================================================
	ImageBarrier
=================================================
*/
	template <typename C, typename I>
	void  _VTransferContextImpl<C,I>::ImageBarrier (ImageID image, EResourceState srcState, EResourceState dstState)
	{
		auto*	img = this->_resMngr.Get( image );
		CHECK_ERRV( img );
		
		VkImageMemoryBarrier	barrier = {};
		barrier.subresourceRange	= { img->AspectMask(), 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS };
		barrier.sType				= VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		barrier.image				= img->Handle();
		barrier.oldLayout			= EResourceState_ToImageLayout( srcState, barrier.subresourceRange.aspectMask );
		barrier.newLayout			= EResourceState_ToImageLayout( dstState, barrier.subresourceRange.aspectMask );
		barrier.srcQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
		barrier.srcAccessMask		= EResourceState_ToAccess( srcState );
		barrier.dstAccessMask		= EResourceState_ToAccess( dstState );
		auto	src_stage			= EResourceState_ToPipelineStages( srcState );
		auto	dst_stage			= EResourceState_ToPipelineStages( dstState );

		this->_resMngr.AddImageBarrier( src_stage, dst_stage, barrier );
	}
	
	template <typename C, typename I>
	void  _VTransferContextImpl<C,I>::ImageBarrier (ImageID image, EResourceState srcState, EResourceState dstState, const ImageSubresourceRange &subRes)
	{
		auto*	img = this->_resMngr.Get( image );
		CHECK_ERRV( img );

		const ImageDesc&	desc = img->Description();
		ASSERT( subRes.baseMipLevel < desc.maxLevel );
		ASSERT( subRes.levelCount == ~0u or subRes.baseMipLevel.Get() + subRes.levelCount <= desc.maxLevel.Get() );
		ASSERT( subRes.baseArrayLayer < desc.arrayLayers );
		ASSERT( subRes.layerCount == ~0u or subRes.baseArrayLayer.Get() + subRes.layerCount <= desc.arrayLayers.Get() );
		
		VkImageMemoryBarrier	barrier = {};
		barrier.subresourceRange.aspectMask		= VEnumCast( subRes.aspectMask );
		barrier.subresourceRange.baseMipLevel	= subRes.baseMipLevel.Get();	// TODO: validate?
		barrier.subresourceRange.levelCount		= Min( desc.maxLevel.Get() - subRes.baseMipLevel.Get(), subRes.levelCount );
		barrier.subresourceRange.baseArrayLayer	= subRes.baseArrayLayer.Get();
		barrier.subresourceRange.layerCount		= Min( desc.arrayLayers.Get() - subRes.baseArrayLayer.Get(), subRes.layerCount );
		barrier.sType				= VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		barrier.image				= img->Handle();
		barrier.oldLayout			= EResourceState_ToImageLayout( srcState, barrier.subresourceRange.aspectMask );
		barrier.newLayout			= EResourceState_ToImageLayout( dstState, barrier.subresourceRange.aspectMask );
		barrier.srcQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
		barrier.srcAccessMask		= EResourceState_ToAccess( srcState );
		barrier.dstAccessMask		= EResourceState_ToAccess( dstState );
		auto	src_stage			= EResourceState_ToPipelineStages( srcState );
		auto	dst_stage			= EResourceState_ToPipelineStages( dstState );

		this->_resMngr.AddImageBarrier( src_stage, dst_stage, barrier );
	}
	
	template <typename C, typename I>
	void  _VTransferContextImpl<C,I>::ImageBarrier (VkPipelineStageFlagBits srcStage, VkPipelineStageFlagBits dstStage, const VkImageMemoryBarrier &barrier)
	{
		this->_resMngr.AddImageBarrier( srcStage, dstStage, barrier );
	}

/*
=================================================
	BufferBarrier
=================================================
*/
	template <typename C, typename I>
	void  _VTransferContextImpl<C,I>::BufferBarrier (BufferID buffer, EResourceState srcState, EResourceState dstState)
	{
		auto*	buf = this->_resMngr.Get( buffer );
		CHECK_ERRV( buf );
		
		VkBufferMemoryBarrier	barrier = {};
		barrier.sType				= VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		barrier.buffer				= buf->Handle();
		barrier.offset				= 0;
		barrier.size				= VK_WHOLE_SIZE;
		barrier.srcQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
		barrier.srcAccessMask		= EResourceState_ToAccess( srcState );
		barrier.dstAccessMask		= EResourceState_ToAccess( dstState );
		auto	src_stage			= EResourceState_ToPipelineStages( srcState );
		auto	dst_stage			= EResourceState_ToPipelineStages( dstState );
		
		this->_resMngr.AddBufferBarrier( src_stage, dst_stage, barrier );
	}
	
	template <typename C, typename I>
	void  _VTransferContextImpl<C,I>::BufferBarrier (BufferID buffer, EResourceState srcState, EResourceState dstState, Bytes offset, Bytes size)
	{
		auto*	buf = this->_resMngr.Get( buffer );
		CHECK_ERRV( buf );

		ASSERT( offset < buf->Size() );
		ASSERT( size == ~0_b or offset + size <= buf->Size() );
		
		VkBufferMemoryBarrier	barrier = {};
		barrier.sType				= VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		barrier.buffer				= buf->Handle();
		barrier.offset				= VkDeviceSize(offset);
		barrier.size				= VkDeviceSize(Min( buf->Size() - barrier.offset, size ));
		barrier.srcQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
		barrier.srcAccessMask		= EResourceState_ToAccess( srcState );
		barrier.dstAccessMask		= EResourceState_ToAccess( dstState );
		auto	src_stage			= EResourceState_ToPipelineStages( srcState );
		auto	dst_stage			= EResourceState_ToPipelineStages( dstState );
		
		this->_resMngr.AddBufferBarrier( src_stage, dst_stage, barrier );
	}
	
	template <typename C, typename I>
	void  _VTransferContextImpl<C,I>::BufferBarrier (VkPipelineStageFlagBits srcStage, VkPipelineStageFlagBits dstStage, const VkBufferMemoryBarrier &barrier)
	{
		this->_resMngr.AddBufferBarrier( srcStage, dstStage, barrier );
	}

/*
=================================================
	SetInitialState
=================================================
*/
	template <typename C, typename I>
	void  _VTransferContextImpl<C,I>::SetInitialState (BufferID id, EResourceState state)
	{
		if constexpr( HasAutoSync )
		{
			auto*	buf = this->_resMngr.Get( id );
			CHECK_ERRV( buf );

			buf->SetInitialState( state );
		}
		Unused( id, state );
	}
	
/*
=================================================
	SetInitialState
=================================================
*/
	template <typename C, typename I>
	void  _VTransferContextImpl<C,I>::SetInitialState (ImageID id, EResourceState state)
	{
		if constexpr( HasAutoSync )
		{
			auto*	img = this->_resMngr.Get( id );
			CHECK_ERRV( img );

			img->SetInitialState( state );
		}
		Unused( id, state );
	}
	
/*
=================================================
	SetInitialState
=================================================
*/
	template <typename C, typename I>
	void  _VTransferContextImpl<C,I>::SetInitialState (ImageID id, EResourceState state, EImageLayout layout)
	{
		if constexpr( HasAutoSync )
		{
			auto*	img = this->_resMngr.Get( id );
			CHECK_ERRV( img );

			img->SetInitialState( state, layout );
		}
		Unused( id, state, layout );
	}
	
/*
=================================================
	GlobalBarrier
=================================================
*/
	template <typename C, typename I>
	void  _VTransferContextImpl<C,I>::GlobalBarrier ()
	{
		VkMemoryBarrier		barrier = {};
		barrier.srcAccessMask	= VWriteAccessMask;		// TODO
		barrier.dstAccessMask	= VReadAccessMask;

		this->_resMngr.AddMemoryBarrier( VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, barrier );
		CommitBarriers();
	}

/*
=================================================
	ClearColorImage
=================================================
*/
	template <typename C, typename I>
	void  _VTransferContextImpl<C,I>::ClearColorImage (ImageID image, const ClearColor_t &color, ArrayView<ImageSubresourceRange> ranges)
	{
		auto*	img = this->_resMngr.Get( image );
		CHECK_ERRV( img );
		ASSERT( ranges.size() );

		if constexpr( HasAutoSync )
		{
			this->_resMngr.AddImage( img, EResourceState::TransferDst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, ranges );
			this->_resMngr.FlushBarriers();
		}
		CommitBarriers();

		VkClearColorValue										clear_value;
		FixedArray<VkImageSubresourceRange, _LocalArraySize>	vk_ranges;
		const ImageDesc &										desc	= img->Description();

		Visit( color,
			   [&clear_value] (const RGBA32f &src) { MemCopy( clear_value.float32, src ); },
			   [&clear_value] (const RGBA32i &src) { MemCopy( clear_value.int32,   src ); },
			   [&clear_value] (const RGBA32u &src) { MemCopy( clear_value.uint32,  src ); });
		
		for (usize i = 0; i < ranges.size(); ++i)
		{
			auto&	src = ranges[i];
			auto&	dst = vk_ranges.emplace_back();

			_ConvertImageSubresourceRange( OUT dst, src, desc );

			if ( vk_ranges.size() == vk_ranges.capacity() )
			{
				RawCtx::ClearColorImage( img->Handle(), clear_value, vk_ranges );
				vk_ranges.clear();
			}
		}

		if ( vk_ranges.size() )
			RawCtx::ClearColorImage( img->Handle(), clear_value, vk_ranges );
	}

/*
=================================================
	ClearDepthStencilImage
=================================================
*/
	template <typename C, typename I>
	void  _VTransferContextImpl<C,I>::ClearDepthStencilImage (ImageID image, const DepthStencil &depthStencil, ArrayView<ImageSubresourceRange> ranges)
	{
		auto*	img = this->_resMngr.Get( image );
		CHECK_ERRV( img );
		ASSERT( ranges.size() );

		if constexpr( HasAutoSync )
		{
			this->_resMngr.AddImage( img, EResourceState::TransferDst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, ranges );
			this->_resMngr.FlushBarriers();
		}
		CommitBarriers();

		VkClearDepthStencilValue								clear_value;
		FixedArray<VkImageSubresourceRange, _LocalArraySize>	vk_ranges;
		const ImageDesc &										desc	= img->Description();

		clear_value.depth	= depthStencil.depth;
		clear_value.stencil	= depthStencil.stencil;
		
		for (usize i = 0; i < ranges.size(); ++i)
		{
			auto&	src = ranges[i];
			auto&	dst = vk_ranges.emplace_back();

			_ConvertImageSubresourceRange( OUT dst, src, desc );

			if ( vk_ranges.size() == vk_ranges.capacity() )
			{
				RawCtx::ClearDepthStencilImage( img->Handle(), clear_value, vk_ranges );
				vk_ranges.clear();
			}
		}

		if ( vk_ranges.size() )
			RawCtx::ClearDepthStencilImage( img->Handle(), clear_value, vk_ranges );
	}

/*
=================================================
	FillBuffer
=================================================
*/
	template <typename C, typename I>
	void  _VTransferContextImpl<C,I>::FillBuffer (BufferID buffer, Bytes offset, Bytes size, uint data)
	{
		auto*	buf = this->_resMngr.Get( buffer );
		CHECK_ERRV( buf );

		ASSERT( offset < buf->Size() );
		ASSERT( size == ~0_b or (offset + size) <= buf->Size() );

		offset	= Min( offset, buf->Size()-1 );
		size	= Min( size, buf->Size() - offset );

		if constexpr( HasAutoSync )
		{
			this->_resMngr.AddBuffer( buf, EResourceState::TransferDst, offset, size );
			this->_resMngr.FlushBarriers();
		}
		CommitBarriers();

		RawCtx::FillBuffer( buf->Handle(), offset, size, data );
	}

/*
=================================================
	UpdateBuffer
=================================================
*/
	template <typename C, typename I>
	void  _VTransferContextImpl<C,I>::UpdateBuffer (BufferID buffer, Bytes offset, Bytes size, const void* data)
	{
		auto*	buf = this->_resMngr.Get( buffer );
		CHECK_ERRV( buf );
		ASSERT( size > 0_b );
		ASSERT( data != null );
		
		if constexpr( HasAutoSync )
		{
			this->_resMngr.AddBuffer( buf, EResourceState::TransferDst, offset, size );
			this->_resMngr.FlushBarriers();
		}
		CommitBarriers();

		RawCtx::UpdateBuffer( buf->Handle(), offset, size, data );
	}
	
/*
=================================================
	UploadBuffer
=================================================
*/
	template <typename C, typename I>
	bool  _VTransferContextImpl<C,I>::UploadBuffer (BufferID buffer, Bytes offset, Bytes size, const void* data, EStagingHeapType heapType)
	{
		BufferView	ranges;

		if ( UploadBuffer( buffer, offset, size, OUT ranges, heapType ))
		{
			ASSERT( ranges.Parts().size() == 1 );
			ASSERT( ranges.Parts().front().size == size );

			MemCopy( OUT ranges.Parts().front().ptr, data, size );
			return true;
		}
		return false;
	}
	
/*
=================================================
	UploadBuffer
=================================================
*/
	template <typename C, typename I>
	bool  _VTransferContextImpl<C,I>::UploadBuffer (BufferID buffer, Bytes offset, Bytes size, OUT BufferView &ranges, EStagingHeapType heapType)
	{
		auto*	dst_buf = this->_resMngr.Get( buffer );
		CHECK_ERR( dst_buf );
		ASSERT( offset + size <= dst_buf->Size() );

		VStagingBufferManager&	sbm	= this->_resMngr.GetStagingManager();
		ranges.clear();

		VStagingBufferManager::StagingBufferResult	res;
		if ( sbm.GetStagingBuffer( OUT res, size, size, 1_b, _batch->GetFrameIndex(), heapType, _batch->GetQueueType(), true ))
		{
			if constexpr( HasAutoSync )
			{
				this->_resMngr.AddBuffer( dst_buf, EResourceState::TransferDst, offset, size );
				this->_resMngr.FlushBarriers();
			}
			CommitBarriers();

			ranges.try_push_back( res.mapped, size );
			CopyBuffer( res.buffer, dst_buf->Handle(), {VkBufferCopy{ VkDeviceSize(res.bufferOffset), VkDeviceSize(offset), VkDeviceSize(size) }});
			return true;
		}
		return false;
	}
	
/*
=================================================
	UploadBuffer
=================================================
*
	template <typename C, typename I>
	bool  _VTransferContextImpl<C,I>::UploadBuffer (BufferStream &stream, OUT BufferView &ranges, EStagingHeapType heapType)
	{
		auto*	dst_buf = this->_resMngr.Get( buffer );
		CHECK_ERR( dst_buf );
		ASSERT( stream.offset + stream.size <= dst_buf->Size() );

		VStagingBufferManager&	sbm			= this->_resMngr.GetStagingManager();
		const Bytes				block_size	= Min( stream.size / 4, stream.size - stream._pos, GraphicsConfig::MinBufferBlockSize );

		VStagingBufferManager::StagingBufferResult	res;
		if ( sbm.GetStagingBuffer( OUT res, stream.size, block_size, 1_b, _batch->GetFrameIndex(), heapType, _batch->GetQueueType(), true ))
		{
			if constexpr( HasAutoSync )
			{
				this->_resMngr.AddBuffer( dst_buf, EResourceState::TransferDst, stream.offset + stream._pos, res.size );
				this->_resMngr.FlushBarriers();
			}
			CommitBarriers();

			ranges.try_push_back( res.mapped, res.size );
			CopyBuffer( res.buffer, dst_buf->Handle(), {VkBufferCopy{ VkDeviceSize(res.bufferOffset), VkDeviceSize(stream.offset + stream._pos), VkDeviceSize(stream.size) }});
			return true;
		}
		return false;
	}
	
/*
=================================================
	UploadImage
=================================================
*
	template <typename C, typename I>
	bool  _VTransferContextImpl<C,I>::UploadImage (ImageStream &stream, OUT ImageView &ranges, EStagingHeapType heapType)
	{
	}

/*
=================================================
	ReadBuffer
=================================================
*/
	template <typename C, typename I>
	Promise<BufferView>  _VTransferContextImpl<C,I>::ReadBuffer (BufferID buffer, Bytes offset, Bytes size, EStagingHeapType heapType)
	{
		auto*	src_buf = this->_resMngr.Get( buffer );
		CHECK_ERR( src_buf );
		ASSERT( offset + size <= src_buf->Size() );
		
		size = Min( size, src_buf->Size() - offset );
		
		VStagingBufferManager&	sbm	= this->_resMngr.GetStagingManager();

		VStagingBufferManager::StagingBufferResult	res;
		if ( sbm.GetStagingBuffer( OUT res, size, size, 1_b, _batch->GetFrameIndex(), heapType, _batch->GetQueueType(), false ))
		{
			if constexpr( HasAutoSync )
			{
				this->_resMngr.AddBuffer( src_buf, EResourceState::TransferSrc, offset, size );
				this->_resMngr.FlushBarriers();
			}
			CommitBarriers();

			CopyBuffer( src_buf->Handle(), res.buffer, {VkBufferCopy{ VkDeviceSize(offset), VkDeviceSize(res.bufferOffset), VkDeviceSize(size) }});

			// TODO: invalidate memory ranges

			return Threading::MakePromise( [ptr = res.mapped, size] ()
											{
												BufferView	view;
												view.try_push_back( ptr, size );
												return view;
											},
											MakeTuple( _batch )
										);
		}
		return Default;
	}
	
/*
=================================================
	ReadImage
=================================================
*
	template <typename C, typename I>
	Promise<ImageView>   _VTransferContextImpl<C,I>::ReadImage (ImageID image, const uint3 &offset, const uint3 &size, MipmapLevel mipLevel,
																ImageLayer arrayLayer, EImageAspect aspectMask, EStagingHeapType heapType)
	{
	}
	
/*
=================================================
	UpdateHostBuffer
=================================================
*/
	template <typename C, typename I>
	bool  _VTransferContextImpl<C,I>::UpdateHostBuffer (BufferID buffer, Bytes offset, Bytes size, const void* data)
	{
		void *	mapped	= null;
		CHECK_ERR( MapHostBuffer( buffer, offset, INOUT size, OUT mapped ));

		MemCopy( mapped, data, size );
		return true;
	}
	
/*
=================================================
	MapHostBuffer
=================================================
*/
	template <typename C, typename I>
	bool  _VTransferContextImpl<C,I>::MapHostBuffer (BufferID buffer, Bytes offset, INOUT Bytes &size, OUT void* &mapped)
	{
		auto*	buf = this->_resMngr.Get( buffer );
		CHECK_ERR( buf );
		
		// TODO: flush host memory ranges

		VulkanMemoryObjInfo		mem_info;
		CHECK_ERR( buf->GetMemoryInfo( OUT mem_info ));
		CHECK_ERR( mem_info.mappedPtr );
		
		offset	= Min( offset, buf->Size()-1 );
		size	= Min( size, buf->Size() - offset );
		mapped	= mem_info.mappedPtr + offset;

		return true;
	}

/*
=================================================
	CopyBuffer
=================================================
*/
	template <typename C, typename I>
	void  _VTransferContextImpl<C,I>::CopyBuffer (BufferID srcBuffer, BufferID dstBuffer, ArrayView<BufferCopy> ranges)
	{
		auto*	src_buf = this->_resMngr.Get( srcBuffer );
		auto*	dst_buf = this->_resMngr.Get( dstBuffer );
		CHECK_ERRV( src_buf and dst_buf );
		ASSERT( ranges.size() );

		if constexpr( HasAutoSync )
		{
			for (usize i = 0; i < ranges.size(); ++i)
			{
				this->_resMngr.AddBuffer( src_buf, EResourceState::TransferSrc, ranges[i].srcOffset, ranges[i].size );
				this->_resMngr.AddBuffer( dst_buf, EResourceState::TransferDst, ranges[i].dstOffset, ranges[i].size );
			}
			this->_resMngr.FlushBarriers();
		}
		CommitBarriers();

		FixedArray<VkBufferCopy, _LocalArraySize>	vk_ranges;
		const Bytes									src_size	= src_buf->Size();
		const Bytes									dst_size	= dst_buf->Size();

		for (usize i = 0; i < ranges.size(); ++i)
		{
			auto&	src = ranges[i];
			auto&	dst = vk_ranges.emplace_back();

			ASSERT( All( src.srcOffset < src_size ));
			ASSERT( All( src.dstOffset < dst_size ));
			ASSERT( All( (src.srcOffset + src.size) <= src_size ));
			ASSERT( All( (src.dstOffset + src.size) <= dst_size ));

			dst.srcOffset	= VkDeviceSize(Min( src.srcOffset, src_size-1 ));
			dst.dstOffset	= VkDeviceSize(Min( src.dstOffset, dst_size-1 ));
			dst.size		= VkDeviceSize(Min( src.size, src_size - src.srcOffset, dst_size - src.dstOffset ));

			if_unlikely( vk_ranges.size() == vk_ranges.capacity() )
			{
				RawCtx::CopyBuffer( src_buf->Handle(), dst_buf->Handle(), vk_ranges );
				vk_ranges.clear();
			}
		}
		
		if ( vk_ranges.size() )
			RawCtx::CopyBuffer( src_buf->Handle(), dst_buf->Handle(), vk_ranges );
	}

/*
=================================================
	CopyImage
=================================================
*/
	template <typename C, typename I>
	void  _VTransferContextImpl<C,I>::CopyImage (ImageID srcImage, ImageID dstImage, ArrayView<ImageCopy> ranges)
	{
		auto*	src_img = this->_resMngr.Get( srcImage );
		auto*	dst_img = this->_resMngr.Get( dstImage );
		CHECK_ERRV( src_img and dst_img );
		ASSERT( ranges.size() );
		
		if constexpr( HasAutoSync )
		{
			this->_resMngr.AddImage( src_img, EResourceState::TransferSrc, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
									 StructView<ImageSubresourceLayers>( ranges, &ImageCopy::srcSubresource ));

			this->_resMngr.AddImage( dst_img, EResourceState::TransferDst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
									 StructView<ImageSubresourceLayers>( ranges, &ImageCopy::dstSubresource ));
			
			this->_resMngr.FlushBarriers();
		}
		CommitBarriers();

		FixedArray<VkImageCopy, _LocalArraySize>	vk_ranges;
		const ImageDesc &							src_desc	= src_img->Description();
		const ImageDesc &							dst_desc	= dst_img->Description();

		for (usize i = 0; i < ranges.size(); ++i)
		{
			auto&	src = ranges[i];
			auto&	dst = vk_ranges.emplace_back();

			ASSERT( All( src.srcOffset < src_img->Dimension() ));
			ASSERT( All( (src.srcOffset + src.extent) <= src_img->Dimension() ));
			ASSERT( All( src.dstOffset < dst_img->Dimension() ));
			ASSERT( All( (src.dstOffset + src.extent) <= dst_img->Dimension() ));

			dst.srcOffset	= { int(src.srcOffset.x), int(src.srcOffset.y), int(src.srcOffset.z) };
			dst.dstOffset	= { int(src.dstOffset.x), int(src.dstOffset.y), int(src.dstOffset.z) };
			dst.extent		= { src.extent.x,         src.extent.y,         src.extent.z };
			
			_ConvertImageSubresourceLayer( OUT dst.srcSubresource, src.srcSubresource, src_desc );
			_ConvertImageSubresourceLayer( OUT dst.dstSubresource, src.dstSubresource, dst_desc );
			
			if_unlikely( vk_ranges.size() == vk_ranges.capacity() )
			{
				RawCtx::CopyImage( src_img->Handle(), dst_img->Handle(), vk_ranges );
				vk_ranges.clear();
			}
		}
		
		if ( vk_ranges.size() )
			RawCtx::CopyImage( src_img->Handle(), dst_img->Handle(), vk_ranges );
	}

/*
=================================================
	CopyBufferToImage
=================================================
*/
	template <typename C, typename I>
	void  _VTransferContextImpl<C,I>::CopyBufferToImage (BufferID srcBuffer, ImageID dstImage, ArrayView<BufferImageCopy> ranges)
	{
		auto*	src_buf = this->_resMngr.Get( srcBuffer );
		auto*	dst_img = this->_resMngr.Get( dstImage );
		CHECK_ERRV( src_buf and dst_img );
		ASSERT( ranges.size() );

		if constexpr( HasAutoSync )
		{
			this->_resMngr.AddBuffer( src_buf, EResourceState::TransferSrc, ranges, dst_img );
			
			this->_resMngr.AddImage( dst_img, EResourceState::TransferDst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
									 StructView<ImageSubresourceLayers>( ranges, &BufferImageCopy::imageSubresource ));

			this->_resMngr.FlushBarriers();
		}
		CommitBarriers();

		FixedArray<VkBufferImageCopy, _LocalArraySize>	vk_ranges;
		const ImageDesc &								dst_desc	= dst_img->Description();
		
		for (usize i = 0; i < ranges.size(); ++i)
		{
			auto&	src = ranges[i];
			auto&	dst = vk_ranges.emplace_back();
			
			_ConvertBufferImageCopy( OUT dst, src, dst_desc );
			
			if_unlikely( vk_ranges.size() == vk_ranges.capacity() )
			{
				RawCtx::CopyBufferToImage( src_buf->Handle(), dst_img->Handle(), vk_ranges );
				vk_ranges.clear();
			}
		}

		if ( vk_ranges.size() )
			RawCtx::CopyBufferToImage( src_buf->Handle(), dst_img->Handle(), vk_ranges );
	}

/*
=================================================
	CopyImageToBuffer
=================================================
*/
	template <typename C, typename I>
	void  _VTransferContextImpl<C,I>::CopyImageToBuffer (ImageID srcImage, BufferID dstBuffer, ArrayView<BufferImageCopy> ranges)
	{
		auto*	src_img = this->_resMngr.Get( srcImage );
		auto*	dst_buf = this->_resMngr.Get( dstBuffer );
		CHECK_ERRV( src_img and dst_buf );
		ASSERT( ranges.size() );
		
		if constexpr( HasAutoSync )
		{
			this->_resMngr.AddImage( src_img, EResourceState::TransferSrc, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
									 StructView<ImageSubresourceLayers>( ranges, &BufferImageCopy::imageSubresource ));
			
			this->_resMngr.AddBuffer( dst_buf, EResourceState::TransferDst, ranges, src_img );

			this->_resMngr.FlushBarriers();
		}
		CommitBarriers();
		
		FixedArray<VkBufferImageCopy, _LocalArraySize>	vk_ranges;
		const ImageDesc &								src_desc	= src_img->Description();
		
		for (usize i = 0; i < ranges.size(); ++i)
		{
			auto&	src = ranges[i];
			auto&	dst = vk_ranges.emplace_back();
			
			_ConvertBufferImageCopy( OUT dst, src, src_desc );
			
			if_unlikely( vk_ranges.size() == vk_ranges.capacity() )
			{
				RawCtx::CopyImageToBuffer( src_img->Handle(), dst_buf->Handle(), vk_ranges );
				vk_ranges.clear();
			}
		}
		
		if ( vk_ranges.size() )
			RawCtx::CopyImageToBuffer( src_img->Handle(), dst_buf->Handle(), vk_ranges );
	}

/*
=================================================
	_ConvertImageSubresourceRange
=================================================
*/
	template <typename C, typename I>
	void  _VTransferContextImpl<C,I>::_ConvertImageSubresourceRange (OUT VkImageSubresourceRange& dst, const ImageSubresourceRange& src, const ImageDesc &desc)
	{
		ASSERT( src.baseMipLevel < desc.maxLevel );
		ASSERT( src.baseArrayLayer < desc.arrayLayers );

		dst.aspectMask		= VEnumCast( src.aspectMask );
		dst.baseMipLevel	= Min( src.baseMipLevel.Get(), desc.maxLevel.Get()-1 );
		dst.levelCount		= Min( src.levelCount, desc.maxLevel.Get() - src.baseMipLevel.Get() );
		dst.baseArrayLayer	= Min( src.baseArrayLayer.Get(), desc.arrayLayers.Get()-1 );
		dst.layerCount		= Min( src.layerCount, desc.arrayLayers.Get() - src.baseArrayLayer.Get() );
	}
	
/*
=================================================
	_ConvertBufferImageCopy
=================================================
*/
	template <typename C, typename I>
	void  _VTransferContextImpl<C,I>::_ConvertBufferImageCopy (OUT VkBufferImageCopy& dst, const BufferImageCopy& src, const ImageDesc &desc)
	{
		ASSERT( All( src.imageOffset < desc.dimension ));
		ASSERT( All( src.imageOffset + src.imageExtent <= desc.dimension ));

		dst.bufferOffset		= VkDeviceSize(src.bufferOffset);
		dst.bufferRowLength		= src.bufferRowLength;
		dst.bufferImageHeight	= src.bufferImageHeight;
		_ConvertImageSubresourceLayer( OUT dst.imageSubresource, src.imageSubresource, desc );
		dst.imageOffset			= { int(src.imageOffset.x), int(src.imageOffset.y), int(src.imageOffset.z) };
		dst.imageExtent			= { src.imageExtent.x, src.imageExtent.y, src.imageExtent.z };
	}
	
/*
=================================================
	_ConvertImageSubresourceLayer
=================================================
*/
	template <typename C, typename I>
	void  _VTransferContextImpl<C,I>::_ConvertImageSubresourceLayer (OUT VkImageSubresourceLayers &dst, const ImageSubresourceLayers &src, const ImageDesc &desc)
	{
		ASSERT( src.mipLevel < desc.maxLevel );
		ASSERT( src.baseArrayLayer < desc.arrayLayers );

		dst.aspectMask		= VEnumCast( src.aspectMask );
		dst.mipLevel		= Min( src.mipLevel.Get(), desc.maxLevel.Get()-1 );
		dst.baseArrayLayer	= Min( src.baseArrayLayer.Get(), desc.arrayLayers.Get()-1 );
		dst.layerCount		= Min( src.layerCount, desc.arrayLayers.Get() - src.baseArrayLayer.Get() );
	}

/*
=================================================
	DebugMarker
=================================================
*/
	template <typename C, typename I>
	void  _VTransferContextImpl<C,I>::DebugMarker (StringView text, RGBA8u color)
	{
		if ( text.size() and this->_resMngr.GetDevice().GetFeatures().debugUtils )
		{
			RawCtx::_DebugMarker( text, color );
		}
	}
	
/*
=================================================
	PushDebugGroup
=================================================
*/
	template <typename C, typename I>
	void  _VTransferContextImpl<C,I>::PushDebugGroup (StringView text, RGBA8u color)
	{
		if ( this->_resMngr.GetDevice().GetFeatures().debugUtils )
		{
			RawCtx::_PushDebugGroup( text, color );
		}
	}
	
/*
=================================================
	PopDebugGroup
=================================================
*/
	template <typename C, typename I>
	void  _VTransferContextImpl<C,I>::PopDebugGroup ()
	{
		if ( this->_resMngr.GetDevice().GetFeatures().debugUtils )
		{
			RawCtx::_PopDebugGroup();
		}
	}
//-----------------------------------------------------------------------------
	
	
	
/*
=================================================
	constructor
=================================================
*/
	template <typename RM>
	_VDirectTransferCtx<RM>::_VDirectTransferCtx (RC<VCommandBatch> batch) :
		_cmdbuf{ RenderGraph().GetCommandPoolManager().GetCommandBuffer( batch->GetQueueType(), ECommandBufferType::Primary_OneTimeSubmit )},
		_resMngr{ *batch }
	{
		VulkanDeviceFn_Init( _resMngr.GetDevice() );
	}
	
/*
=================================================
	destructor
=================================================
*/
	template <typename RM>
	_VDirectTransferCtx<RM>::~_VDirectTransferCtx ()
	{
		ASSERT( not _cmdbuf.IsValid() and "you forget to call 'ReleaseCommandBuffer()'" );
	}
	
/*
=================================================
	ReleaseCommandBuffer
=================================================
*/
	template <typename RM>
	VkCommandBuffer  _VDirectTransferCtx<RM>::ReleaseCommandBuffer ()
	{
		VkCommandBuffer	cmd = _cmdbuf.Get();

		_resMngr.TransitToDefaultState();
		CommitBarriers();

		VK_CALL( vkEndCommandBuffer( cmd ));

		_cmdbuf.Release();
		return cmd;
	}

/*
=================================================
	CommitBarriers
=================================================
*/
	template <typename RM>
	void  _VDirectTransferCtx<RM>::CommitBarriers ()
	{
		typename RM::PipelineBarrier	bar;

		if ( _resMngr.GetBarriers( OUT bar ))
		{
			vkCmdPipelineBarrier( _cmdbuf.Get(), bar.srcStageMask, bar.dstStageMask, 0,
								  bar.memoryBarrierCount, bar.memoryBarriers,
								  bar.bufferBarrierCount, bar.bufferBarriers,
								  bar.imageBarrierCount, bar.imageBarriers );
			_resMngr.ClearBarriers();
		}
	}

/*
=================================================
	ClearColorImage
=================================================
*/
	template <typename RM>
	void  _VDirectTransferCtx<RM>::ClearColorImage (VkImage image, const VkClearColorValue &color, ArrayView<VkImageSubresourceRange> ranges)
	{
		ASSERT( image != VK_NULL_HANDLE );
		ASSERT( ranges.size() );
		
		vkCmdClearColorImage( _cmdbuf.Get(), image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &color, uint(ranges.size()), ranges.data() );
	}
	
/*
=================================================
	ClearDepthStencilImage
=================================================
*/
	template <typename RM>
	void  _VDirectTransferCtx<RM>::ClearDepthStencilImage (VkImage image, const VkClearDepthStencilValue &depthStencil, ArrayView<VkImageSubresourceRange> ranges)
	{
		ASSERT( image != VK_NULL_HANDLE );
		ASSERT( ranges.size() );
		
		vkCmdClearDepthStencilImage( _cmdbuf.Get(), image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &depthStencil, uint(ranges.size()), ranges.data() );
	}
	
/*
=================================================
	FillBuffer
=================================================
*/
	template <typename RM>
	void  _VDirectTransferCtx<RM>::FillBuffer (VkBuffer buffer, Bytes offset, Bytes size, uint data)
	{
		ASSERT( buffer != VK_NULL_HANDLE );
		
		vkCmdFillBuffer( _cmdbuf.Get(), buffer, VkDeviceSize(offset), VkDeviceSize(size), data );
	}
	
/*
=================================================
	UpdateBuffer
=================================================
*/
	template <typename RM>
	void  _VDirectTransferCtx<RM>::UpdateBuffer (VkBuffer buffer, Bytes offset, Bytes size, const void* data)
	{
		ASSERT( buffer != VK_NULL_HANDLE );
		
		vkCmdUpdateBuffer( _cmdbuf.Get(), buffer, VkDeviceSize(offset), VkDeviceSize(size), data );
	}
	
/*
=================================================
	CopyBuffer
=================================================
*/
	template <typename RM>
	void  _VDirectTransferCtx<RM>::CopyBuffer (VkBuffer srcBuffer, VkBuffer dstBuffer, ArrayView<VkBufferCopy> ranges)
	{
		ASSERT( srcBuffer != VK_NULL_HANDLE );
		ASSERT( dstBuffer != VK_NULL_HANDLE );
		ASSERT( ranges.size() );
		
		vkCmdCopyBuffer( _cmdbuf.Get(), srcBuffer, dstBuffer, uint(ranges.size()), ranges.data() );
	}
	
/*
=================================================
	CopyImage
=================================================
*/
	template <typename RM>
	void  _VDirectTransferCtx<RM>::CopyImage (VkImage srcImage, VkImage dstImage, ArrayView<VkImageCopy> ranges)
	{
		ASSERT( srcImage != VK_NULL_HANDLE );
		ASSERT( dstImage != VK_NULL_HANDLE );
		ASSERT( ranges.size() );
		
		vkCmdCopyImage( _cmdbuf.Get(), srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
						dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
						uint(ranges.size()), ranges.data() );
	}
	
/*
=================================================
	CopyBufferToImage
=================================================
*/
	template <typename RM>
	void  _VDirectTransferCtx<RM>::CopyBufferToImage (VkBuffer srcBuffer, VkImage dstImage, ArrayView<VkBufferImageCopy> ranges)
	{
		ASSERT( srcBuffer != VK_NULL_HANDLE );
		ASSERT( dstImage != VK_NULL_HANDLE );
		ASSERT( ranges.size() );
		
		vkCmdCopyBufferToImage( _cmdbuf.Get(), srcBuffer, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, uint(ranges.size()), ranges.data() );
	}
	
/*
=================================================
	CopyImageToBuffer
=================================================
*/
	template <typename RM>
	void  _VDirectTransferCtx<RM>::CopyImageToBuffer (VkImage srcImage, VkBuffer dstBuffer, ArrayView<VkBufferImageCopy> ranges)
	{
		ASSERT( srcImage != VK_NULL_HANDLE );
		ASSERT( dstBuffer != VK_NULL_HANDLE );
		ASSERT( ranges.size() );

		vkCmdCopyImageToBuffer( _cmdbuf.Get(), srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dstBuffer, uint(ranges.size()), ranges.data() );
	}

/*
=================================================
	_DebugMarker
=================================================
*/
	template <typename RM>
	void  _VDirectTransferCtx<RM>::_DebugMarker (StringView text, RGBA8u color)
	{
		VkDebugUtilsLabelEXT	info = {};
		info.sType		= VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
		info.pLabelName	= text.data();
		MemCopy( info.color, RGBA32f{color} );
			
		vkCmdInsertDebugUtilsLabelEXT( _cmdbuf.Get(), &info );
	}
	
/*
=================================================
	_PushDebugGroup
=================================================
*/
	template <typename RM>
	void  _VDirectTransferCtx<RM>::_PushDebugGroup (StringView text, RGBA8u color)
	{
		VkDebugUtilsLabelEXT	info = {};
		info.sType		= VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
		info.pLabelName	= text.size() ? text.data() : "";
		MemCopy( info.color, RGBA32f{color} );

		vkCmdBeginDebugUtilsLabelEXT( _cmdbuf.Get(), &info );
	}
	
/*
=================================================
	_PopDebugGroup
=================================================
*/
	template <typename RM>
	void  _VDirectTransferCtx<RM>::_PopDebugGroup ()
	{
		vkCmdEndDebugUtilsLabelEXT( _cmdbuf.Get() );
	}
//-----------------------------------------------------------------------------



/*
=================================================
	CommitBarriers
=================================================
*/
	template <typename RM, typename BC>
	void  _VIndirectCtxHelper<RM, BC>::CommitBarriers ()
	{
		typename RM::PipelineBarrier	bar;

		if ( _resMngr.GetBarriers( OUT bar ))
		{
			this->_CommitBarriers( bar );
			_resMngr.ClearBarriers();
		}
	}
	
/*
=================================================
	Prepare
=================================================
*/
	template <typename RM, typename BC>
	VBakedCommands  _VIndirectCtxHelper<RM, BC>::Prepare ()
	{
		_resMngr.TransitToDefaultState();
		CommitBarriers();

		return this->_Prepare( &BC::_Execute );
	}


}	// AE::Graphics::_hidden_

#endif	// AE_ENABLE_VULKAN
