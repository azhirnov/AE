// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#ifdef AE_ENABLE_VULKAN
# include "Test_VulkanRenderGraph.h"

namespace
{
	enum class ExeOrderIndex : uint {
		Initial	= 0,
		Unknown = ~0u
	};

	class VBufferMock1
	{
	public:
		BufferDesc	desc;

		VkBuffer			Handle ()		const	{ return VK_NULL_HANDLE; }
		BufferDesc const&	Description ()	const	{ return desc; }
	};

	class VBarrierManagerMock1
	{
	public:
		struct Barrier : VkBufferMemoryBarrier
		{
			VkPipelineStageFlagBits		srcStageMask;
			VkPipelineStageFlagBits		dstStageMask;
			uint						index;
		};

		Array<Barrier>		barriers;
		uint				counter = 0;

	public:
		void AddBufferBarrier (VkPipelineStageFlagBits		srcStageMask,
							   VkPipelineStageFlagBits		dstStageMask,
							   const VkBufferMemoryBarrier	&barrier)
		{
			auto&	dst = barriers.emplace_back();
			dst.sType				= barrier.sType;
			dst.pNext				= barrier.pNext;
			dst.srcAccessMask		= barrier.srcAccessMask;
			dst.dstAccessMask		= barrier.dstAccessMask;
			dst.srcQueueFamilyIndex	= barrier.srcQueueFamilyIndex;
			dst.dstQueueFamilyIndex	= barrier.dstQueueFamilyIndex;
			dst.buffer				= barrier.buffer;
			dst.offset				= barrier.offset;
			dst.size				= barrier.size;
			dst.srcStageMask		= srcStageMask;
			dst.dstStageMask		= dstStageMask;
			dst.index				= counter;
		}
	};

}	// namespace

# define  TEST				CHECK_ERR
# define  VBuffer			VBufferMock1
# define  VBarrierManager	VBarrierManagerMock1

# include "graphics/Private/EnumUtils.h"
# include "graphics/Vulkan/VEnumCast.h"
# include "graphics/Vulkan/RenderGraph/VLocalBuffer.h"


bool VRGTest::Test_Buffer ()
{
	VBarrierManager		barrier_mngr;
	VBuffer				global;
	VLocalBuffer		local;

	TEST( local.Create( &global ));

	local.AddPendingState( EResourceState::TransferDst, ExeOrderIndex(1) );
	local.CommitBarrier( barrier_mngr );
	barrier_mngr.counter++;

	// check barriers
	{
		TEST( barrier_mngr.barriers.size() == 1 );
		
		const auto&		bar = barrier_mngr.barriers.back();
		TEST( bar.index == 0 );
		TEST( bar.srcStageMask == VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT );
		TEST( bar.dstStageMask == VK_PIPELINE_STAGE_TRANSFER_BIT );
		TEST( bar.srcAccessMask == 0 );
		TEST( bar.dstAccessMask == VK_ACCESS_TRANSFER_WRITE_BIT );
		TEST( bar.offset == 0 );
		TEST( bar.size == VK_WHOLE_SIZE );
		TEST( bar.buffer == global.Handle() );
		TEST( bar.srcQueueFamilyIndex == VK_QUEUE_FAMILY_IGNORED );
		TEST( bar.dstQueueFamilyIndex == VK_QUEUE_FAMILY_IGNORED );
	}
	
	local.AddPendingState( EResourceState::UniformRead | EResourceState::_FragmentShader, ExeOrderIndex(2) );
	local.CommitBarrier( barrier_mngr );
	barrier_mngr.counter++;
	
	// check barriers
	{
		TEST( barrier_mngr.barriers.size() == 2 );
		
		const auto&		bar = barrier_mngr.barriers.back();
		TEST( bar.index == 1 );
		TEST( bar.srcStageMask == VK_PIPELINE_STAGE_TRANSFER_BIT );
		TEST( bar.dstStageMask == VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT );
		TEST( bar.srcAccessMask == VK_ACCESS_TRANSFER_WRITE_BIT );
		TEST( bar.dstAccessMask == VK_ACCESS_UNIFORM_READ_BIT );
		TEST( bar.offset == 0 );
		TEST( bar.size == VK_WHOLE_SIZE );
		TEST( bar.buffer == global.Handle() );
		TEST( bar.srcQueueFamilyIndex == VK_QUEUE_FAMILY_IGNORED );
		TEST( bar.dstQueueFamilyIndex == VK_QUEUE_FAMILY_IGNORED );
	}
	
	local.AddPendingState( EResourceState::UniformRead | EResourceState::_VertexShader, ExeOrderIndex(3) );
	local.CommitBarrier( barrier_mngr );
	barrier_mngr.counter++;
	
	// no barriers because uniform cache is already invalidated
	TEST( barrier_mngr.barriers.size() == 2 );

	local.AddPendingState( EResourceState::ShaderRead | EResourceState::_ComputeShader, ExeOrderIndex(4) );
	local.CommitBarrier( barrier_mngr );
	barrier_mngr.counter++;
	
	// check barriers
	{
		TEST( barrier_mngr.barriers.size() == 3 );
		
		const auto&		bar = barrier_mngr.barriers.back();
		TEST( bar.index == 3 );
		TEST( bar.srcStageMask == VK_PIPELINE_STAGE_TRANSFER_BIT );
		TEST( bar.dstStageMask == VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT );
		TEST( bar.srcAccessMask == VK_ACCESS_TRANSFER_WRITE_BIT );
		TEST( bar.dstAccessMask == VK_ACCESS_SHADER_READ_BIT );
		TEST( bar.offset == 0 );
		TEST( bar.size == VK_WHOLE_SIZE );
		TEST( bar.buffer == global.Handle() );
		TEST( bar.srcQueueFamilyIndex == VK_QUEUE_FAMILY_IGNORED );
		TEST( bar.dstQueueFamilyIndex == VK_QUEUE_FAMILY_IGNORED );
	}
	
	local.AddPendingState( EResourceState::ShaderReadWrite | EResourceState::_ComputeShader, ExeOrderIndex(5) );
	local.CommitBarrier( barrier_mngr );
	barrier_mngr.counter++;
	
	// check barriers
	{
		TEST( barrier_mngr.barriers.size() == 4 );
		
		const auto&		bar = barrier_mngr.barriers.back();
		TEST( bar.index == 4 );
		TEST( bar.srcStageMask == (VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT) );
		TEST( bar.dstStageMask == VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT );
		TEST( bar.srcAccessMask == 0 );
		TEST( bar.dstAccessMask == VkAccessFlags(VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT) );
		TEST( bar.offset == 0 );
		TEST( bar.size == VK_WHOLE_SIZE );
		TEST( bar.buffer == global.Handle() );
		TEST( bar.srcQueueFamilyIndex == VK_QUEUE_FAMILY_IGNORED );
		TEST( bar.dstQueueFamilyIndex == VK_QUEUE_FAMILY_IGNORED );
	}

	local.Destroy( barrier_mngr );

	AE_LOGI( TEST_NAME << " - passed" );
	return true;
}

#endif	// AE_ENABLE_VULKAN
