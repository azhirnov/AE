// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#ifdef AE_ENABLE_VULKAN
# include "Test_VulkanRenderGraph.h"

namespace
{
	using CmdIndex = uint;
	

	void  AddBarrier (VLocalResManager &localResMngr, const VCommandBuffer &cmdbuf, const VDevice &dev)
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


	static bool  Test_BufferBarriers (VLocalResManager &localResMngr)
	{
		auto &				res_mngr = RenderGraph().GetResourceManager();
		VDevice const&		dev		= res_mngr.GetDevice();
		UniqueID<BufferID>	id		= res_mngr.CreateBuffer( BufferDesc{ 256_b, EBufferUsage::Uniform | EBufferUsage::Storage | EBufferUsage::Transfer }, "Buffer", null );
		auto const&			local	= *localResMngr.Get( BufferID{id} );
		VCommandBuffer		cmdbuf	= RenderGraph().GetCommandPoolManager().GetCommandBuffer( EQueueType::Graphics, ECommandBufferType::Primary_OneTimeSubmit );
		
		const auto	CommitBarrier = [&] ()
		{
			local.CommitBarrier( localResMngr );
			AddBarrier( localResMngr, cmdbuf, dev );
		};

		dev.SetObjectName( BitCast<ulong>(cmdbuf.Get()), "CmdBuf", VK_OBJECT_TYPE_COMMAND_BUFFER );
	
		local.AddPendingState( EResourceState::TransferDst, CmdIndex{1} );
		CommitBarrier();
	
		local.AddPendingState( EResourceState::UniformRead | EResourceState::_FragmentShader, CmdIndex{2} );
		CommitBarrier();
	
		local.AddPendingState( EResourceState::UniformRead | EResourceState::_VertexShader, CmdIndex{3} );
		CommitBarrier();
	
		local.AddPendingState( EResourceState::ShaderRead | EResourceState::_ComputeShader, CmdIndex{4} );
		CommitBarrier();
	
		local.AddPendingState( EResourceState::ShaderReadWrite | EResourceState::_ComputeShader, CmdIndex{5} );
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


	static bool  Test_ImageBarriers (VLocalResManager &localResMngr)
	{
		auto &				res_mngr = RenderGraph().GetResourceManager();
		VDevice const&		dev		= res_mngr.GetDevice();
		UniqueID<ImageID>	id		= res_mngr.CreateImage( ImageDesc{}.SetDimension( uint2{256} ).SetUsage( EImageUsage::Storage | EImageUsage::Sampled | EImageUsage::Transfer )
																		.SetFormat( EPixelFormat::RGBA8_UNorm ), EResourceState::Unknown, "Image", null );
		auto const&			local	= *localResMngr.Get( ImageID{id} );
		VCommandBuffer		cmdbuf	= RenderGraph().GetCommandPoolManager().GetCommandBuffer( EQueueType::Graphics, ECommandBufferType::Primary_OneTimeSubmit );
		
		const auto	CommitBarrier = [&] ()
		{
			local.CommitBarrier( localResMngr );
			AddBarrier( localResMngr, cmdbuf, dev );
		};

		dev.SetObjectName( BitCast<ulong>(cmdbuf.Get()), "CmdBuf", VK_OBJECT_TYPE_COMMAND_BUFFER );

		local.SetInitialState( EResourceState::Unknown );

		local.AddPendingState( EResourceState::TransferDst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, CmdIndex{1} );
		CommitBarrier();
		
		local.AddPendingState( EResourceState::ShaderSample | EResourceState::_FragmentShader, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, CmdIndex{2} );
		CommitBarrier();
		
		local.AddPendingState( EResourceState::ShaderSample | EResourceState::_VertexShader, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, CmdIndex{3} );
		CommitBarrier();
		
		local.AddPendingState( EResourceState::ShaderRead | EResourceState::_ComputeShader, VK_IMAGE_LAYOUT_GENERAL, CmdIndex{4} );
		CommitBarrier();
		
		local.AddPendingState( EResourceState::ShaderReadWrite | EResourceState::_ComputeShader, VK_IMAGE_LAYOUT_GENERAL, CmdIndex{5} );
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


bool VRGTest::Test_LocalResManager ()
{
	VLocalResManager	res_mngr{ *(VCommandBatch *)null };
	
	CHECK_ERR( Test_BufferBarriers( res_mngr ));
	CHECK_ERR( Test_ImageBarriers( res_mngr ));
	
	CHECK_ERR( _CompareDumps( TEST_NAME ));

	AE_LOGI( TEST_NAME << " - passed" );
	return true;
}

#endif	// AE_ENABLE_VULKAN
