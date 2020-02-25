// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#ifdef AE_ENABLE_VULKAN
# include "Test_VulkanRenderGraph.h"

using BufferCopy = ITransferContext::BufferCopy;


bool VRGTest::Test_CopyBuffer1 ()
{
	constexpr BytesU		buf_size = 128_b;

	UniqueID<GfxResourceID>	buf_1;
	UniqueID<GfxResourceID>	buf_2;

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
	}
	
	uint8_t		buffer_data [uint(buf_size)] = {};
	STATIC_ASSERT( buf_size == sizeof(buffer_data) );

	for (size_t i = 0; i < CountOf(buffer_data); ++i) {
		buffer_data[i] = uint8_t(i);
	}
	
	bool	cb_was_called	= false;
	bool	data_is_correct	= false;

	auto	OnLoaded = [&] (const BufferView &inData)
	{
		cb_was_called	= true;
		data_is_correct	= (inData.DataSize() == buf_size);

		for (size_t i = 0; data_is_correct and (i < CountOf(buffer_data)); ++i)
		{
			bool	is_equal = (buffer_data[i] == Cast<uint8_t>(inData.Parts().front().ptr)[i]);
			ASSERT( is_equal );

			data_is_correct &= is_equal;
		}
	};

	_renderGraph->Add(
		EQueueType::Graphics, {}, {},
		[&] (IGraphicsContext &ctx, ArrayView<GfxResourceID>, ArrayView<GfxResourceID>)
		{
			CHECK( ctx.UpdateHostBuffer( buf_1, 0_b, buffer_data ));

			ctx.CopyBuffer( buf_1, buf_2, { BufferCopy{ 0_b, 0_b, buf_size }});
			ctx.ReadBuffer( buf_2, 0_b, buf_size, std::move(OnLoaded) );
		});

	CmdBatchID	batch = _renderGraph->Submit();
	
	CHECK_ERR( _renderGraph->Wait({ batch }));
	CHECK_ERR( cb_was_called );
	CHECK_ERR( data_is_correct );

	DeleteResources( buf_1, buf_2 );
	
	AE_LOGI( TEST_NAME << " - passed" );
	return true;
}

#endif	// AE_ENABLE_VULKAN
