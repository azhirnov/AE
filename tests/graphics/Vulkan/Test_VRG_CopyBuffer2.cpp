// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#ifdef AE_ENABLE_VULKAN
# include "Test_VulkanRenderGraph.h"

using BufferCopy = ITransferContext::BufferCopy;


bool VRGTest::Test_CopyBuffer2 ()
{
	constexpr BytesU		buf_size = 256_Mb;

	UniqueID<GfxResourceID>	buf_1;
	UniqueID<GfxResourceID>	buf_2;

	{
		BufferDesc	desc;
		desc.size		= buf_size;
		desc.usage		= EBufferUsage::Transfer;
		desc.memType	= EMemoryType::DeviceLocal;

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

	const auto	GenContent = [](uint count, uint seed)
	{
		Array<uint>		result;
		result.resize( count );
		for (size_t i = 0; i < result.size(); ++i) {
			result[i] = uint(i) ^ seed;
		}
		return result;
	};

	struct BufData
	{
		Array<uint>		content;
		BytesU			offset;
	};
	Array<BufData>	buf_data;
	buf_data.push_back({ GenContent(48, 0x33445566), 64_b });
	buf_data.push_back({ GenContent(72, 0x11883399), 512_Kb });
	buf_data.push_back({ GenContent(84, 0xEE44AA22), 132_Mb });

	
	bool	cb_was_called	= false;
	bool	data_is_correct	= false;

	auto	OnLoaded = [&] (const BufferView &inData)
	{
		cb_was_called	= true;
		data_is_correct	= (inData.DataSize() == buf_size);

		BytesU	offset;
		uint	j = 0;
		for (auto& part : inData.Parts())
		{
			if ( offset >= buf_data[j].offset and (offset + part.size) < buf_data[j].offset )
			{
				bool  is_equal = (std::memcmp( part.ptr + buf_data[j].offset - offset, buf_data[j].content.data(), size_t(ArraySizeOf(buf_data[j].content)) ) == 0);
				ASSERT( is_equal );
				data_is_correct &= is_equal;
				++j;
			}

			if ( j >= buf_data.size() )
				break;

			offset += part.size;
		}
	};

	_renderGraph->Add(
		EQueueType::Graphics, {}, {},
		[&] (IGraphicsContext &ctx, ArrayView<GfxResourceID>, ArrayView<GfxResourceID>)
		{
			BufferView	ranges;
			CHECK( ctx.UploadBuffer( buf_1, 0_b, 256_Mb, OUT ranges ));

			BytesU	offset;
			size_t	j = 0;
			for (auto& range : ranges)
			{
				std::memset( range.ptr, 0, size_t(range.size) );

				if ( offset >= buf_data[j].offset and offset + range.size < buf_data[j].offset )
				{
					std::memcpy( range.ptr + (buf_data[j].offset - offset), buf_data[j].content.data(), size_t(ArraySizeOf(buf_data[j].content)) );
					++j;
				}
				if ( j >= buf_data.size() )
					break;

				offset += range.size;
			}
		});

	_renderGraph->Add(
		EQueueType::Graphics, {}, {},
		[&] (IGraphicsContext &ctx, ArrayView<GfxResourceID>, ArrayView<GfxResourceID>)
		{
			ctx.CopyBuffer( buf_1, buf_2, { BufferCopy{ 0_b, 0_b, buf_size }});
			CHECK( ctx.ReadBuffer( buf_2, 0_b, buf_size, std::move(OnLoaded) ));
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
