// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#ifdef AE_ENABLE_VULKAN
# include "Test_VulkanRenderGraph.h"

using BufferCopy = ITransferContext::BufferCopy;

#define TEST( ... )		CHECK_ERR( __VA_ARGS__, void())


bool VRGTest::Test_VirtualRes1 ()
{
	const BytesU	buf_size = 128_Kb;

	UniqueID<GfxResourceID>	virt_1;
	UniqueID<GfxResourceID>	virt_2;
	UniqueID<GfxResourceID>	virt_3;
	UniqueID<GfxResourceID>	virt_4;

	{
		VirtualBufferDesc	desc;
		desc.size	= buf_size;

		virt_1 = _resourceMngr->CreateBuffer( desc, "virt_1" );
		CHECK_ERR( virt_1 );

		virt_2 = _resourceMngr->CreateBuffer( desc, "virt_2" );
		CHECK_ERR( virt_2 );

		virt_3 = _resourceMngr->CreateBuffer( desc, "virt_3" );
		CHECK_ERR( virt_3 );

		virt_4 = _resourceMngr->CreateBuffer( desc, "virt_4" );
		CHECK_ERR( virt_4 );
	}

	uint64_t	order = 1;
	
	CHECK_ERR( _renderGraph->Add(
		EQueueType::Graphics,
		{{virt_1, EVirtualResourceUsage::IndexBuffer}},
		{{virt_2, EVirtualResourceUsage::Storage}, {virt_3, EVirtualResourceUsage::Storage}},
		[&] (IGraphicsContext &ctx, ArrayView<GfxResourceID> input, ArrayView<GfxResourceID> output)
		{
			(order <<= 3) |= 1;

			TEST( input.size()  == 1 );
			TEST( output.size() == 2 );

			TEST( input[0].ResourceType()  == GfxResourceID::EType::Buffer );
			TEST( output[0].ResourceType() == GfxResourceID::EType::VirtualBuffer );
			TEST( output[1].ResourceType() == GfxResourceID::EType::VirtualBuffer );
			
			auto&	desc = _resourceMngr->GetBufferDescription( input[0] );
			TEST( AllBits( desc.usage, EBufferUsage::Index ));
			TEST( desc.size == buf_size );

			TEST( ctx.SetOutput( output[0], input[0] ));
			TEST( ctx.SetOutput( output[1], input[0] ));
		},
		"pass 2" ));

	CHECK_ERR( _renderGraph->Add(
		EQueueType::Graphics,
		{},
		{{virt_1, EVirtualResourceUsage::Storage}},
		[&] (IGraphicsContext &ctx, ArrayView<GfxResourceID> input, ArrayView<GfxResourceID> output)
		{
			(order <<= 2) |= 2;

			TEST( input.size()  == 0 );
			TEST( output.size() == 1 );

			GfxResourceID	id = ctx.GetOutput( output[0] );
			TEST( id.ResourceType() == GfxResourceID::EType::Buffer );

			auto&	desc = _resourceMngr->GetBufferDescription( id );
			TEST( AllBits( desc.usage, EBufferUsage::Storage ));
			TEST( desc.size == buf_size );
		},
		"pass 1" ));

	CHECK_ERR( _renderGraph->Add(
		EQueueType::Graphics,
		{{virt_2, EVirtualResourceUsage::IndexBuffer}},
		{},
		[&] (IGraphicsContext &, ArrayView<GfxResourceID> input, ArrayView<GfxResourceID> output)
		{
			(order <<= 3) |= 2;

			TEST( input.size()  == 1 );
			TEST( output.size() == 0 );
			
			TEST( input[0].ResourceType()  == GfxResourceID::EType::Buffer );
		},
		"pass 3" ));

	CHECK_ERR( _renderGraph->Add(
		EQueueType::Graphics,
		{{virt_3, EVirtualResourceUsage::IndexBuffer}},
		{{GfxResourceID_External, EVirtualResourceUsage::Unknown}},
		[&] (IGraphicsContext &, ArrayView<GfxResourceID> input, ArrayView<GfxResourceID> output)
		{
			(order <<= 4) |= 1;

			TEST( input.size()  == 1 );
			TEST( output.size() == 1 );
			
			TEST( input[0].ResourceType()  == GfxResourceID::EType::Buffer );
			TEST( output[0] == GfxResourceID_External );
		},
		"pass 4" ));

	CmdBatchID	batch = _renderGraph->Submit();
	CHECK_ERR( batch );
	
	CHECK_ERR( _renderGraph->Wait({ batch }));

	uint64_t	required = 1ull;
	(required <<= 2) |= 2;
	(required <<= 3) |= 1;
	(required <<= 3) |= 2;
	(required <<= 4) |= 1;
	CHECK_ERR( order == required );

	DeleteResources( virt_1, virt_2, virt_3, virt_4 );
	
	AE_LOGI( TEST_NAME << " - passed" );
	return true;
}

#endif	// AE_ENABLE_VULKAN
