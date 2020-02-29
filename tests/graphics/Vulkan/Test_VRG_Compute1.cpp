// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#ifdef AE_ENABLE_VULKAN
# include "Test_VulkanRenderGraph.h"


bool VRGTest::Test_Compute1 ()
{
	const uint3		view_size {16, 16, 1};

	UniqueID<GfxResourceID>	image0;
	UniqueID<GfxResourceID>	image1;
	UniqueID<GfxResourceID>	image2;
	{
		ImageDesc	desc;
		desc.dimension	= view_size;
		desc.format		= EPixelFormat::RGBA8_UNorm;
		desc.memType	= EMemoryType::DeviceLocal;
		desc.imageType	= EImage::_2D;
		desc.usage		= EImageUsage::Storage | EImageUsage::TransferSrc;

		image0 = _resourceMngr->CreateImage( desc, Default, "Image_0" );
		CHECK_ERR( image0 );
	}{
		ImageDesc	desc;
		desc.dimension	= view_size;
		desc.format		= EPixelFormat::RGBA8_UNorm;
		desc.memType	= EMemoryType::DeviceLocal;
		desc.imageType	= EImage::_2D;
		desc.usage		= EImageUsage::Storage | EImageUsage::TransferSrc;

		image1 = _resourceMngr->CreateImage( desc, Default, "Image_1" );
		CHECK_ERR( image1 );
	}{
		ImageDesc	desc;
		desc.dimension	= view_size;
		desc.format		= EPixelFormat::RGBA8_UNorm;
		desc.memType	= EMemoryType::DeviceLocal;
		desc.imageType	= EImage::_2D;
		desc.usage		= EImageUsage::Storage | EImageUsage::TransferSrc;

		image2 = _resourceMngr->CreateImage( desc, Default, "Image_2" );
		CHECK_ERR( image2 );
	}

	ComputePipelineID	ppln0;
	ComputePipelineID	ppln1;
	ComputePipelineID	ppln2;
	{
		ComputePipelineDesc	desc;

		ppln0 = _resourceMngr->GetComputePipeline( PipelineName{"compute1"}, desc );
		CHECK_ERR( ppln0 );
		
		desc.localGroupSize = {4, 4, 1};

		ppln1 = _resourceMngr->GetComputePipeline( PipelineName{"compute1"}, desc );
		CHECK_ERR( ppln1 );
		
		desc.localGroupSize = {16, 16, 1};

		ppln2 = _resourceMngr->GetComputePipeline( PipelineName{"compute1"}, desc );
		CHECK_ERR( ppln2 );
	}

	DescriptorSetID	ds0;
	DescriptorSetID	ds1;
	DescriptorSetID	ds2;
	uint			ds_index;
	{
		DescriptorSet	desc;
		CHECK_ERR( _resourceMngr->InitializeDescriptorSet( ppln0, DescriptorSetName{"0"}, OUT desc ));

		ds_index = desc.Index();

		desc.BindImage( UniformName{"un_OutImage"}, image0 );

		ds0 = _resourceMngr->CreateDescriptorSet( desc );
		CHECK_ERR( ds0 );
		
		desc.BindImage( UniformName{"un_OutImage"}, image1 );

		ds1 = _resourceMngr->CreateDescriptorSet( desc );
		CHECK_ERR( ds1 );

		desc.BindImage( UniformName{"un_OutImage"}, image2 );
		
		ds2 = _resourceMngr->CreateDescriptorSet( desc );
		CHECK_ERR( ds2 );
	}
	
	bool	data0_is_correct = false;
	bool	data1_is_correct = false;
	bool	data2_is_correct = false;
	
	const auto	CheckData = [] (const ImageView &imageData, uint blockSize, OUT bool &isCorrect)
	{
		isCorrect = true;

		for (uint y = 0; y < imageData.Dimension().y; ++y)
		{
			for (uint x = 0; x < imageData.Dimension().x; ++x)
			{
				RGBA32u		dst;
				imageData.Load( uint3(x,y,0), OUT dst );

				const uint	r	= uint(float(x % blockSize) * 255.0f / float(blockSize) + 0.5f);
				const uint	g	= uint(float(y % blockSize) * 255.0f / float(blockSize) + 0.5f);

				const bool	is_equal = (Equals( dst[0], r, 1u )	and
										Equals( dst[1], g, 1u )	and
										dst[2] == 255			and
										dst[3] == 0);

				ASSERT( is_equal );
				isCorrect &= is_equal;
			}
		}
	};
	const auto	OnLoaded0 =	[&CheckData, OUT &data0_is_correct] (const ImageView &imageData) {
		CheckData( imageData, 8, OUT data0_is_correct );
	};
	const auto	OnLoaded1 =	[&CheckData, OUT &data1_is_correct] (const ImageView &imageData) {
		CheckData( imageData, 4, OUT data1_is_correct );
	};
	const auto	OnLoaded2 =	[&CheckData, OUT &data2_is_correct] (const ImageView &imageData) {
		CheckData( imageData, 16, OUT data2_is_correct );
	};
	
	_renderGraph->Add(
		EQueueType::Graphics, {}, {},
		[&] (IGraphicsContext &ctx, ArrayView<GfxResourceID>, ArrayView<GfxResourceID>)
		{
			// initial layout transition
			ctx.SetInitialState( image0, EResourceState::Unknown );
			ctx.SetInitialState( image1, EResourceState::Unknown );
			ctx.SetInitialState( image2, EResourceState::Unknown );

			ctx.BindPipeline( ppln0 );
			ctx.BindDescriptorSet( ds_index, ds0 );
			ctx.Dispatch({ 2, 2, 1 });
			
			ctx.BindPipeline( ppln1 );
			ctx.BindDescriptorSet( ds_index, ds1 );
			ctx.Dispatch({ 4, 4, 1 });
			
			ctx.BindPipeline( ppln2 );
			ctx.BindDescriptorSet( ds_index, ds2 );
			ctx.Dispatch({ 1, 1, 1 });
		});
	
	_renderGraph->Add(
		EQueueType::Graphics, {}, {},
		[&] (IGraphicsContext &ctx, ArrayView<GfxResourceID>, ArrayView<GfxResourceID>)
		{
			CHECK( ctx.ReadImage( image0, uint3(0), view_size, 0_mipmap, 0_layer, EImageAspect::Color, std::move(OnLoaded0) ));
			CHECK( ctx.ReadImage( image1, uint3(0), view_size, 0_mipmap, 0_layer, EImageAspect::Color, std::move(OnLoaded1) ));
			CHECK( ctx.ReadImage( image2, uint3(0), view_size, 0_mipmap, 0_layer, EImageAspect::Color, std::move(OnLoaded2) ));
		});

	CmdBatchID	batch = _renderGraph->Submit();
	
	CHECK_ERR( _renderGraph->Wait({ batch }));
	CHECK_ERR( data0_is_correct and data1_is_correct and data2_is_correct );

	DeleteResources( image0, image1, image2 );
	
	AE_LOGI( TEST_NAME << " - passed" );
	return true;
}

#endif	// AE_ENABLE_VULKAN
