// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#ifdef AE_ENABLE_VULKAN
# include "Test_VulkanRenderGraph.h"

namespace
{
	enum class ExeOrderIndex : uint {
		Initial	= 0,
		Unknown = ~0u
	};

	class VImageMock2
	{
	public:
		ImageDesc	desc;

		VkImage				Handle ()			const	{ return VK_NULL_HANDLE; }
		ImageDesc const&	Description ()		const	{ return desc; }
		VkImageLayout		DefaultLayout ()	const	{ return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; }
		VkImageAspectFlags	AspectMask ()		const	{ return VK_IMAGE_ASPECT_COLOR_BIT; }
	};

	class VBarrierManagerMock2
	{
	public:
		struct Barrier : VkImageMemoryBarrier
		{
			VkPipelineStageFlagBits		srcStageMask;
			VkPipelineStageFlagBits		dstStageMask;
			uint						index;
		};

		Array<Barrier>		barriers;
		uint				counter = 0;

	public:
		void AddImageBarrier (VkPipelineStageFlagBits		srcStageMask,
							  VkPipelineStageFlagBits		dstStageMask,
							  VkDependencyFlagBits			dependencyFlags,
							  const VkImageMemoryBarrier	&barrier)
		{
			CHECK_FATAL( dependencyFlags == Zero );

			auto&	dst = barriers.emplace_back();
			dst.sType				= barrier.sType;
			dst.pNext				= barrier.pNext;
			dst.srcAccessMask		= barrier.srcAccessMask;
			dst.dstAccessMask		= barrier.dstAccessMask;
			dst.srcQueueFamilyIndex	= barrier.srcQueueFamilyIndex;
			dst.dstQueueFamilyIndex	= barrier.dstQueueFamilyIndex;
			dst.oldLayout			= barrier.oldLayout;
			dst.newLayout			= barrier.newLayout;
			dst.image				= barrier.image;
			dst.subresourceRange	= barrier.subresourceRange;
			dst.srcStageMask		= srcStageMask;
			dst.dstStageMask		= dstStageMask;
			dst.index				= counter;
		}
	};

}	// namespace

# define  TEST				CHECK_ERR
# define  VImage			VImageMock2
# define  VBarrierManager	VBarrierManagerMock2

# include "graphics/Private/EnumUtils.h"
# include "graphics/Vulkan/VEnumCast.h"
# include "graphics/Vulkan/VCommon.h"
# include "graphics/Vulkan/RenderGraph/VLocalImage.h"


bool VRGTest::Test_Image ()
{
	VBarrierManager		barrier_mngr;
	VImage				global;
	VLocalImage			local;

	TEST( local.Create( &global ));

	local.AddPendingState( EResourceState::TransferDst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, ExeOrderIndex(1) );
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
		TEST( bar.image == global.Handle() );
		TEST( bar.oldLayout == global.DefaultLayout() );
		TEST( bar.newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL );
		TEST( bar.srcQueueFamilyIndex == VK_QUEUE_FAMILY_IGNORED );
		TEST( bar.dstQueueFamilyIndex == VK_QUEUE_FAMILY_IGNORED );
	}
	
	local.AddPendingState( EResourceState::ShaderSample | EResourceState::_FragmentShader, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, ExeOrderIndex(2) );
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
		TEST( bar.dstAccessMask == VK_ACCESS_SHADER_READ_BIT );
		TEST( bar.image == global.Handle() );
		TEST( bar.oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL );
		TEST( bar.newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );
		TEST( bar.srcQueueFamilyIndex == VK_QUEUE_FAMILY_IGNORED );
		TEST( bar.dstQueueFamilyIndex == VK_QUEUE_FAMILY_IGNORED );
	}
	
	local.AddPendingState( EResourceState::ShaderSample | EResourceState::_VertexShader, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, ExeOrderIndex(3) );
	local.CommitBarrier( barrier_mngr );
	barrier_mngr.counter++;
	
	// no barriers because texture cache is already invalidated
	TEST( barrier_mngr.barriers.size() == 2 );

	local.AddPendingState( EResourceState::ShaderRead | EResourceState::_ComputeShader, VK_IMAGE_LAYOUT_GENERAL, ExeOrderIndex(4) );
	local.CommitBarrier( barrier_mngr );
	barrier_mngr.counter++;
	
	// check barriers
	{
		TEST( barrier_mngr.barriers.size() == 3 );
		
		const auto&		bar = barrier_mngr.barriers.back();
		TEST( bar.index == 3 );
		TEST( bar.srcStageMask == (VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT) );
		TEST( bar.dstStageMask == VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT );
		TEST( bar.srcAccessMask == Zero );
		TEST( bar.dstAccessMask == VK_ACCESS_SHADER_READ_BIT );
		TEST( bar.image == global.Handle() );
		TEST( bar.oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );
		TEST( bar.newLayout == VK_IMAGE_LAYOUT_GENERAL );
		TEST( bar.srcQueueFamilyIndex == VK_QUEUE_FAMILY_IGNORED );
		TEST( bar.dstQueueFamilyIndex == VK_QUEUE_FAMILY_IGNORED );
	}
	
	local.AddPendingState( EResourceState::ShaderReadWrite | EResourceState::_ComputeShader, VK_IMAGE_LAYOUT_GENERAL, ExeOrderIndex(5) );
	local.CommitBarrier( barrier_mngr );
	barrier_mngr.counter++;
	
	// check barriers
	{
		TEST( barrier_mngr.barriers.size() == 4 );
		
		const auto&		bar = barrier_mngr.barriers.back();
		TEST( bar.index == 4 );
		TEST( bar.srcStageMask == VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT );
		TEST( bar.dstStageMask == VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT );
		TEST( bar.srcAccessMask == Zero );
		TEST( bar.dstAccessMask == VkAccessFlags(VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT) );
		TEST( bar.image == global.Handle() );
		TEST( bar.oldLayout == VK_IMAGE_LAYOUT_GENERAL );
		TEST( bar.newLayout == VK_IMAGE_LAYOUT_GENERAL );
		TEST( bar.srcQueueFamilyIndex == VK_QUEUE_FAMILY_IGNORED );
		TEST( bar.dstQueueFamilyIndex == VK_QUEUE_FAMILY_IGNORED );
	}

	local.Destroy( barrier_mngr );

	AE_LOGI( TEST_NAME << " - passed" );
	return true;
}

#endif	// AE_ENABLE_VULKAN
