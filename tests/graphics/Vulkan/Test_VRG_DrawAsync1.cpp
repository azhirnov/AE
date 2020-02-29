// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#ifdef AE_ENABLE_VULKAN
# include "Test_VulkanRenderGraph.h"


bool VRGTest::Test_DrawAsync1 ()
{
	const uint3		view_size {800, 600, 1};

	UniqueID<GfxResourceID>	image;

	{
		ImageDesc	desc;
		desc.dimension	= view_size;
		desc.format		= EPixelFormat::RGBA8_UNorm;
		desc.memType	= EMemoryType::DeviceLocal;
		desc.imageType	= EImage::_2D;
		desc.usage		= EImageUsage::ColorAttachment | EImageUsage::TransferSrc;

		image = _resourceMngr->CreateImage( desc, Default, "RenderTarget" );
		CHECK_ERR( image );
	}

	const auto	GetPipeline = [this] (RenderPassID rp)
	{
		GraphicsPipelineDesc	desc;
		desc.renderPassId	= rp;
		desc.subpassIndex	= 0;
		desc.viewportCount	= 1;
		desc.dynamicState	= EPipelineDynamicState::Viewport | EPipelineDynamicState::Scissor;

		desc.renderState.inputAssembly.topology = EPrimitive::TriangleList;

		return _resourceMngr->GetGraphicsPipeline( PipelineName{"draw1"}, desc );
	};

	bool		data_is_correct = false;

	const auto	OnLoaded =	[OUT &data_is_correct] (const ImageView &imageData)
	{
		const auto	TestPixel = [&imageData] (float x, float y, const RGBA32f &color)
		{
			uint	ix	 = uint( (x + 1.0f) * 0.5f * float(imageData.Dimension().x) + 0.5f );
			uint	iy	 = uint( (y + 1.0f) * 0.5f * float(imageData.Dimension().y) + 0.5f );

			RGBA32f	col;
			imageData.Load( uint3(ix, iy, 0), OUT col );

			bool	is_equal = All(Equals( col, color, 0.1f ));
			ASSERT( is_equal );
			return is_equal;
		};

		data_is_correct  = true;
		data_is_correct &= TestPixel( 0.00f, -0.49f, RGBA32f{1.0f, 0.0f, 0.0f, 1.0f} );
		data_is_correct &= TestPixel( 0.49f,  0.49f, RGBA32f{0.0f, 1.0f, 0.0f, 1.0f} );
		data_is_correct &= TestPixel(-0.49f,  0.49f, RGBA32f{0.0f, 0.0f, 1.0f, 1.0f} );
			
		data_is_correct &= TestPixel( 0.00f, -0.51f, RGBA32f{0.0f} );
		data_is_correct &= TestPixel( 0.51f,  0.51f, RGBA32f{0.0f} );
		data_is_correct &= TestPixel(-0.51f,  0.51f, RGBA32f{0.0f} );
		data_is_correct &= TestPixel( 0.00f,  0.51f, RGBA32f{0.0f} );
		data_is_correct &= TestPixel( 0.51f, -0.51f, RGBA32f{0.0f} );
		data_is_correct &= TestPixel(-0.51f, -0.51f, RGBA32f{0.0f} );
	};
	
	_renderGraph->Add(
		EQueueType::Graphics, {}, {},
		[&] (IGraphicsContext &ctx, ArrayView<GfxResourceID>, ArrayView<GfxResourceID>)
		{
			// initial layout transition
			ctx.SetInitialState( image, EResourceState::Unknown );
		});

	_renderGraph->Add(
		EQueueType::Graphics, {}, {},
		RenderPassDesc{ RenderPassName{"ColorPass"}, uint2{view_size} }
			.AddTarget( RenderTargetName{"out_Color"}, image, RGBA32f(0.0f), EAttachmentStoreOp::Store )
			.AddViewport( uint2{view_size} ),
		[&] (IAsyncRenderContext &ctx, ArrayView<GfxResourceID>, ArrayView<GfxResourceID>)
		{
			auto	info = ctx.GetContextInfo();

			ctx.BeginCommandBuffer();

			ctx.BindPipeline( GetPipeline( info.renderPass ));
			ctx.Draw( 3 );
		});
	
	_renderGraph->Add(
		EQueueType::Graphics, {}, {},
		[&] (IGraphicsContext &ctx, ArrayView<GfxResourceID>, ArrayView<GfxResourceID>)
		{
			CHECK( ctx.ReadImage( image, uint3(0), view_size, 0_mipmap, 0_layer, EImageAspect::Color, std::move(OnLoaded) ));
		});

	CmdBatchID	batch = _renderGraph->Submit();
	
	CHECK_ERR( _renderGraph->Wait({ batch }));
	CHECK_ERR( data_is_correct );

	DeleteResources( image );
	
	AE_LOGI( TEST_NAME << " - passed" );
	return true;
}

#endif	// AE_ENABLE_VULKAN
