// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "Test_VulkanRenderGraph.h"

using BufferCopy = ITransferContext::BufferCopy;


bool VRGTest::Test_CopyBuffer ()
{
	constexpr BytesU		buf_size = 128_b;

	UniqueID<GfxResourceID>	buf_1;
	UniqueID<GfxResourceID>	buf_2;
	UniqueID<GfxResourceID>	buf_3;

	{
		BufferDesc	desc;
		desc.size		= buf_size;
		desc.usage		= EBufferUsage::TransferSrc;
		desc.memType	= EMemoryType::HostCached;

		buf_1 = _resourceMngr->CreateBuffer( desc, "buf_1" );
		CHECK_ERR( buf_1 );
	}{
		BufferDesc	desc;
		desc.size		= buf_size;
		desc.usage		= EBufferUsage::Transfer;
		desc.memType	= EMemoryType::DeviceLocal;

		buf_2 = _resourceMngr->CreateBuffer( desc, "buf_2" );
		CHECK_ERR( buf_2 );
	}{
		BufferDesc	desc;
		desc.size		= buf_size;
		desc.usage		= EBufferUsage::TransferDst;
		desc.memType	= EMemoryType::HostCocherent;

		buf_3 = _resourceMngr->CreateBuffer( desc, "buf_3" );
		CHECK_ERR( buf_3 );
	}
	
	uint	buffer_data[32] = {};
	STATIC_ASSERT( buf_size == sizeof(buffer_data) );

	for (size_t i = 0; i < CountOf(buffer_data); ++i) {
		buffer_data[i] = uint(i);
	}
	
	void*	mapped_buf_3 = null;

	_renderGraph->Add(
		EQueueType::Graphics, {}, {},
		[&] (IGraphicsContext &ctx, ArrayView<GfxResourceID>, ArrayView<GfxResourceID>)
		{
			CHECK( ctx.UpdateHostBuffer( buf_1, 0_b, buffer_data ));

			ctx.CopyBuffer( buf_1, buf_2, { BufferCopy{ 0_b, 0_b, buf_size } });
		});
	
	_renderGraph->Add(
		EQueueType::Graphics, {}, {},
		[&] (IGraphicsContext &ctx, ArrayView<GfxResourceID>, ArrayView<GfxResourceID>)
		{
			ctx.CopyBuffer( buf_2, buf_3, { BufferCopy{ 0_b, 0_b, buf_size } });

			BytesU	size = buf_size;
			CHECK( ctx.MapHostBuffer( buf_3, 0_b, INOUT size, OUT mapped_buf_3 ));
		});

	CmdBatchID	batch = _renderGraph->Submit();
	
	CHECK_ERR( _renderGraph->Wait({ batch }));
	CHECK_ERR( mapped_buf_3 );

	CHECK_ERR( std::memcmp( buffer_data, mapped_buf_3, sizeof(buffer_data) ) == 0 );

	DeleteResources( buf_1, buf_2, buf_3 );
	
	AE_LOGI( TEST_NAME << " - passed" );
	return true;
}
