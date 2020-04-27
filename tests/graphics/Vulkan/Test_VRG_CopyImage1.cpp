// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#ifdef AE_ENABLE_VULKAN
# include "Test_VulkanRenderGraph.h"

using ImageCopy = ITransferContext::ImageCopy;


bool VRGTest::Test_CopyImage1 ()
{
	const uint3		img_dim {1u<<10, 1u<<10, 1};

	UniqueID<GfxResourceID>	img_1;
	UniqueID<GfxResourceID>	img_2;

	{
		ImageDesc	desc;
		desc.dimension	= img_dim;
		desc.format		= EPixelFormat::RGBA32U;
		desc.memType	= EMemoryType::DeviceLocal;
		desc.imageType	= EImage::_2D;
		desc.usage		= EImageUsage::Transfer | EImageUsage::Sampled;

		img_1 = _resourceMngr->CreateImage( desc, Default, "img_1" );
		CHECK_ERR( img_1 );
	}{
		ImageDesc	desc;
		desc.dimension	= img_dim;
		desc.format		= EPixelFormat::RGBA32U;
		desc.memType	= EMemoryType::DeviceLocal;
		desc.imageType	= EImage::_2D;
		desc.usage		= EImageUsage::Transfer | EImageUsage::Sampled;
		
		img_2 = _resourceMngr->CreateImage( desc, Default, "img_2" );
		CHECK_ERR( img_2 );
	}
	
	const auto	GenContent = [](uint count, uint seed)
	{
		Array<RGBA32u>		result;
		result.resize( count );
		for (size_t i = 0; i < count; ++i) {
			result[i].r = uint(i*4+0) ^ seed;
			result[i].g = uint(i*4+1) ^ seed;
			result[i].b = uint(i*4+2) ^ seed;
			result[i].a = uint(i*4+3) ^ seed;
		}
		return result;
	};

	struct ImgData
	{
		Array<RGBA32u>	content;	// row
		uint2			offset;
	};
	Array<ImgData>	img_data;
	img_data.push_back({ GenContent(128, 0x33445566), uint2(0, 128) });
	img_data.push_back({ GenContent(128, 0x11883399), uint2(0, 256) });
	img_data.push_back({ GenContent(128, 0xEE44AA22), uint2(0, 512) });

	
	bool	cb_was_called	= false;
	bool	data_is_correct	= false;

	auto	OnLoaded = [&] (const ImageView &inData)
	{
		cb_was_called	= true;
		data_is_correct = true;
		
		const BytesU	bpp {inData.BitsPerPixel() / 8};
		for (auto& part : img_data)
		{
			auto	row		 = inData.GetRow( part.offset.y );
			bool	is_equal = (std::memcmp( row.ptr + bpp * part.offset.x, part.content.data(), size_t(ArraySizeOf(part.content)) ) == 0);
			ASSERT( is_equal );
			data_is_correct &= is_equal;
		}
	};

	CHECK_ERR( _renderGraph->Add(
		EQueueType::Graphics, {}, {},
		[&] (IGraphicsContext &ctx, ArrayView<GfxResourceID>, ArrayView<GfxResourceID>)
		{
			// initial layout transition
			ctx.SetInitialState( img_1, EResourceState::Unknown );
			ctx.SetInitialState( img_2, EResourceState::Unknown );

			ImageView	ranges;
			CHECK( ctx.UploadImage( img_1, uint3(0), img_dim, 0_mipmap, 0_layer, EImageAspect::Color, OUT ranges ));

			const BytesU	bpp {ranges.BitsPerPixel() / 8};
			for (auto& part : img_data)
			{
				auto	row = ranges.GetRow( part.offset.y );
				std::memcpy( OUT row.ptr + bpp * part.offset.x, part.content.data(), size_t(ArraySizeOf(part.content)) );
			}
		}));
	
	CHECK_ERR( _renderGraph->Add(
		EQueueType::Graphics, {}, {},
		[&] (IGraphicsContext &ctx, ArrayView<GfxResourceID>, ArrayView<GfxResourceID>)
		{
			ImageCopy	range;
			range.srcOffset	= {0, 0, 0};
			range.dstOffset	= {0, 0, 0};
			range.extent	= img_dim;
			range.srcSubresource.aspectMask = EImageAspect::Color;
			range.dstSubresource.aspectMask = EImageAspect::Color;

			ctx.CopyImage( img_1, img_2, {range} );

			CHECK( ctx.ReadImage( img_2, uint3(0), img_dim, 0_mipmap, 0_layer, EImageAspect::Color, std::move(OnLoaded) ));
		}));

	CmdBatchID	batch = _renderGraph->Submit();
	CHECK_ERR( batch );
	
	CHECK_ERR( _renderGraph->Wait({ batch }));
	CHECK_ERR( cb_was_called );
	CHECK_ERR( data_is_correct );

	DeleteResources( img_1, img_2 );
	
	AE_LOGI( TEST_NAME << " - passed" );
	return true;
}

#endif	// AE_ENABLE_VULKAN
