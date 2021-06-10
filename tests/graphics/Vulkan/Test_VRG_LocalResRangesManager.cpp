// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#ifdef AE_ENABLE_VULKAN
# include "Test_VulkanRenderGraph.h"

namespace
{
	using BufferState	= VLocalResRangesManager::LocalBuffer::BufferState;
	using ImageState	= VLocalResRangesManager::LocalImage::ImageState;
	using ImageRange	= VLocalResRangesManager::LocalImage::ImageRange;
	using CmdIndex		= uint;


	void  AddBarrier (VLocalResRangesManager &localResMngr, const VCommandBuffer &cmdbuf, const VDevice &dev)
	{
		VBarrierManager::PipelineBarrier	bar;

		if ( localResMngr.GetBarriers( OUT bar ))
		{
			dev.vkCmdPipelineBarrier( cmdbuf.Get(), bar.srcStageMask, bar.dstStageMask, 0,
									  bar.memoryBarrierCount, bar.memoryBarriers,
									  bar.bufferBarrierCount, bar.bufferBarriers,
									  bar.imageBarrierCount, bar.imageBarriers );
			localResMngr.ClearBarriers();
		}
	}


	static bool  Test_BufferBarriers (VLocalResRangesManager &localResMngr)
	{
		auto &				res_mngr = RenderGraph().GetResourceManager();
		VDevice const&		dev		= res_mngr.GetDevice();
		UniqueID<BufferID>	id		= res_mngr.CreateBuffer( BufferDesc{ 1024_b, EBufferUsage::Uniform | EBufferUsage::Storage | EBufferUsage::Transfer }, "Buffer", null );
		auto const&			local	= *localResMngr.Get( BufferID{id} );
		VCommandBuffer		cmdbuf	= RenderGraph().GetCommandPoolManager().GetCommandBuffer( EQueueType::Graphics, ECommandBufferType::Primary_OneTimeSubmit );

		const auto	CommitBarrier = [&] ()
		{
			local.CommitBarrier( localResMngr );
			AddBarrier( localResMngr, cmdbuf, dev );
		};

		dev.SetObjectName( BitCast<ulong>(cmdbuf.Get()), "CmdBuf", VK_OBJECT_TYPE_COMMAND_BUFFER );

		local.AddPendingState(BufferState{ EResourceState::TransferDst, 0, 512, CmdIndex{1} });
		CommitBarrier();

		local.AddPendingState(BufferState{ EResourceState::TransferSrc, 0, 64, CmdIndex{2} });
		CommitBarrier();

		local.AddPendingState(BufferState{ EResourceState::UniformRead | EResourceState::_VertexShader, 64, 64+64, CmdIndex{3} });
		CommitBarrier();

		local.AddPendingState(BufferState{ EResourceState::UniformRead | EResourceState::_FragmentShader, 256, 256+64, CmdIndex{4} });
		CommitBarrier();

		local.AddPendingState(BufferState{ EResourceState::ShaderWrite | EResourceState::_ComputeShader, 512, 512+64, CmdIndex{5} });
		CommitBarrier();

		local.AddPendingState(BufferState{ EResourceState::UniformRead | EResourceState::_VertexShader, 256+32, 256+64, CmdIndex{6} });
		CommitBarrier();

		local.AddPendingState(BufferState{ EResourceState::UniformRead | EResourceState::_GeometryShader, 256+16, 256+16+32, CmdIndex{7} });
		CommitBarrier();

		local.AddPendingState(BufferState{ EResourceState::UniformRead | EResourceState::_GeometryShader, 16, 32, CmdIndex{8} });
		CommitBarrier();

		local.AddPendingState(BufferState{ EResourceState::ShaderRead | EResourceState::_ComputeShader, 0, 256+32, CmdIndex{9} });
		CommitBarrier();

		local.AddPendingState(BufferState{ EResourceState::TransferDst, 64, 512, CmdIndex{10} });
		CommitBarrier();
		
		localResMngr.TransitToDefaultState();
		AddBarrier( localResMngr, cmdbuf, dev );

		VK_CHECK( dev.vkEndCommandBuffer( cmdbuf.Get() ));

		auto	queue = dev.GetQueue( EQueueType::Graphics );
		EXLOCK( queue->guard );

		VkCommandBuffer	cmdbuffers[]	= { cmdbuf.Get() };
		VkSubmitInfo	submit			= {};
		submit.sType				= VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit.pCommandBuffers		= cmdbuffers;
		submit.commandBufferCount	= 1;

		VK_CHECK( dev.vkQueueSubmit( queue->handle, 1, &submit, VK_NULL_HANDLE ));
		VK_CHECK( dev.vkQueueWaitIdle( queue->handle ));

		CHECK_ERR( res_mngr.ReleaseResource( id ));
		return true;
	}

	
	static bool  Test_ImageBarriers1 (VLocalResRangesManager &localResMngr)
	{
		auto &				res_mngr = RenderGraph().GetResourceManager();
		VDevice const&		dev		= res_mngr.GetDevice();
		UniqueID<ImageID>	id		= res_mngr.CreateImage( ImageDesc{}.SetDimension({ 64, 64 }).SetFormat( EPixelFormat::RGBA8_UNorm )
																	.SetUsage( EImageUsage::ColorAttachment | EImageUsage::Transfer | EImageUsage::Storage | EImageUsage::Sampled )
																	.SetMaxMipmaps( 11 ), EResourceState::Unknown, "Image", null );
		auto const&			local	= *localResMngr.Get( ImageID{id} );
		VCommandBuffer		cmdbuf	= RenderGraph().GetCommandPoolManager().GetCommandBuffer( EQueueType::Graphics, ECommandBufferType::Primary_OneTimeSubmit );
		
		const auto	CommitBarrier = [&] ()
		{
			local.CommitBarrier( localResMngr );
			AddBarrier( localResMngr, cmdbuf, dev );
		};

		dev.SetObjectName( BitCast<ulong>(cmdbuf.Get()), "CmdBuf", VK_OBJECT_TYPE_COMMAND_BUFFER );

		local.SetInitialState( EResourceState::Unknown );
		
		local.AddPendingState(ImageState{ EResourceState::TransferDst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
										  ImageRange{ 0_layer, 1, 0_mipmap, 1 },
										  VK_IMAGE_ASPECT_COLOR_BIT, CmdIndex{1} });
		CommitBarrier();

		localResMngr.TransitToDefaultState();
		AddBarrier( localResMngr, cmdbuf, dev );

		VK_CHECK( dev.vkEndCommandBuffer( cmdbuf.Get() ));

		auto	queue = dev.GetQueue( EQueueType::Graphics );
		EXLOCK( queue->guard );

		VkCommandBuffer	cmdbuffers[]	= { cmdbuf.Get() };
		VkSubmitInfo	submit			= {};
		submit.sType				= VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit.pCommandBuffers		= cmdbuffers;
		submit.commandBufferCount	= 1;

		VK_CHECK( dev.vkQueueSubmit( queue->handle, 1, &submit, VK_NULL_HANDLE ));
		VK_CHECK( dev.vkQueueWaitIdle( queue->handle ));

		CHECK_ERR( res_mngr.ReleaseResource( id ));
		return true;
	}

	
	static bool  Test_ImageBarriers2 (VLocalResRangesManager &localResMngr)
	{
		auto &				res_mngr = RenderGraph().GetResourceManager();
		VDevice const&		dev		= res_mngr.GetDevice();
		UniqueID<ImageID>	id		= res_mngr.CreateImage( ImageDesc{}.SetDimension({ 64, 64 }).SetFormat( EPixelFormat::RGBA8_UNorm )
																.SetUsage( EImageUsage::ColorAttachment | EImageUsage::Transfer | EImageUsage::Storage | EImageUsage::Sampled )
																.SetMaxMipmaps( 11 ).SetArrayLayers( 8 ), EResourceState::Unknown, "Image", null );
		auto const&			local	= *localResMngr.Get( ImageID{id} );
		VCommandBuffer		cmdbuf	= RenderGraph().GetCommandPoolManager().GetCommandBuffer( EQueueType::Graphics, ECommandBufferType::Primary_OneTimeSubmit );
		
		const auto	CommitBarrier = [&] ()
		{
			local.CommitBarrier( localResMngr );
			AddBarrier( localResMngr, cmdbuf, dev );
		};

		dev.SetObjectName( BitCast<ulong>(cmdbuf.Get()), "CmdBuf", VK_OBJECT_TYPE_COMMAND_BUFFER );

		local.SetInitialState( EResourceState::Unknown );
		
		local.AddPendingState( ImageState{ EResourceState::TransferDst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
										   ImageRange{ 0_layer, 2, 0_mipmap, local.MipmapLevels() },
										   VK_IMAGE_ASPECT_COLOR_BIT, CmdIndex{1} });
		CommitBarrier();
		
		local.AddPendingState( ImageState{ EResourceState::ShaderSample | EResourceState::_FragmentShader, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
										   ImageRange{ 0_layer, local.ArrayLayers(), 0_mipmap, 2 },
										   VK_IMAGE_ASPECT_COLOR_BIT, CmdIndex{2} });
		CommitBarrier();
		
		local.AddPendingState( ImageState{ EResourceState::ShaderWrite | EResourceState::_ComputeShader, VK_IMAGE_LAYOUT_GENERAL,
										   ImageRange{ 0_layer, local.ArrayLayers(), 0_mipmap, local.MipmapLevels() },
										   VK_IMAGE_ASPECT_COLOR_BIT, CmdIndex{3} });
		CommitBarrier();

		localResMngr.TransitToDefaultState();
		AddBarrier( localResMngr, cmdbuf, dev );

		VK_CHECK( dev.vkEndCommandBuffer( cmdbuf.Get() ));

		auto	queue = dev.GetQueue( EQueueType::Graphics );
		EXLOCK( queue->guard );

		VkCommandBuffer	cmdbuffers[]	= { cmdbuf.Get() };
		VkSubmitInfo	submit			= {};
		submit.sType				= VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit.pCommandBuffers		= cmdbuffers;
		submit.commandBufferCount	= 1;

		VK_CHECK( dev.vkQueueSubmit( queue->handle, 1, &submit, VK_NULL_HANDLE ));
		VK_CHECK( dev.vkQueueWaitIdle( queue->handle ));

		CHECK_ERR( res_mngr.ReleaseResource( id ));
		return true;
	}
}


bool VRGTest::Test_LocalResRangesManager ()
{
	VLocalResRangesManager	res_mngr{ *(VCommandBatch *)null };
	
	CHECK_ERR( Test_BufferBarriers( res_mngr ));
	CHECK_ERR( Test_ImageBarriers1( res_mngr ));
	CHECK_ERR( Test_ImageBarriers2( res_mngr ));
	
	CHECK_ERR( _CompareDumps( TEST_NAME ));

	AE_LOGI( TEST_NAME << " - passed" );
	return true;
}

#endif	// AE_ENABLE_VULKAN
