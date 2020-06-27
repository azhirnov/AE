// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "../UnitTest_Common.h"

#ifdef AE_ENABLE_VULKAN

# include "ecs-st/RendererVk/VGraphicsPass.h"
# include "graphics/Vulkan/RenderGraph/VRenderGraph.h"
# include "pipeline_compiler/PipelineCompiler.h"
# include "stl/Utils/FileSystem.h"
# include "stl/Stream/FileStream.h"
# include "stl/Platforms/PlatformUtils.h"
# include "threading/TaskSystem/WorkerThread.h"

namespace
{
	using AE::ECS::Systems::VGraphicsPass;
	using AE::ECS::Renderer::ERenderPass;
	using namespace AE::Graphics;
	using namespace ECS::Components;
	using namespace AE::Threading;

	using OpquePassTag = RenderPassTag<ERenderPass::Opaque_1>;

	static UniqueID<PipelinePackID>		pipelinePack;
	
/*
=================================================
	InitRegistry
=================================================
*/
	static void  InitRegistry (Registry &reg)
	{
		reg.RegisterComponents< VDrawVertices, VDrawIndexedVertices, VDrawMeshes, VPipeline, VPushConstant, VDescriptorSets,
								VGraphicsPipelineRef, VMeshPipelineRef, InvalidatePipelineTag, OpquePassTag >();
	}
	
/*
=================================================
	GraphicsPass_Test1
=================================================
*/
	static void  GraphicsPass_Test1 (VResourceManager &resourceMngr, IRenderGraph &renderGraph)
	{
		Registry	reg;
		InitRegistry( reg );

		VGraphicsPass<ERenderPass::Opaque_1>	pass{ reg, resourceMngr };

		UniqueID<GfxResourceID>		buffer;
		VkDescriptorSet				desc_sets[3];
		{
			BufferDesc	desc;
			desc.size	= SizeOf<int4>;
			desc.usage	= EBufferUsage::Uniform | EBufferUsage::Transfer;

			buffer = resourceMngr.CreateBuffer( desc, "Buffer", null );
			CHECK_FATAL( buffer );

			DescriptorSet	ds_per_pass;
			DescriptorSet	ds_material;
			DescriptorSet	ds_draw_call;
			CHECK_FATAL( resourceMngr.InitializeDescriptorSet( PipelineName{"draw1"}, DescriptorSetName{"PerPass"},  OUT ds_per_pass ));
			CHECK_FATAL( resourceMngr.InitializeDescriptorSet( PipelineName{"draw1"}, DescriptorSetName{"Material"}, OUT ds_material ));
			CHECK_FATAL( resourceMngr.InitializeDescriptorSet( PipelineName{"draw1"}, DescriptorSetName{"DrawCall"}, OUT ds_draw_call ));

			ds_per_pass.BindBuffer(  UniformName{"un_GlobalConst"}, buffer );
			ds_material.BindBuffer(  UniformName{"un_Material"},    buffer );
			ds_draw_call.BindBuffer( UniformName{"un_DrawCall"},    buffer );

			desc_sets[0] = resourceMngr.GetResource( resourceMngr.CreateDescriptorSet( ds_per_pass ))->Handle();
			desc_sets[1] = resourceMngr.GetResource( resourceMngr.CreateDescriptorSet( ds_material ))->Handle();
			desc_sets[2] = resourceMngr.GetResource( resourceMngr.CreateDescriptorSet( ds_draw_call ))->Handle();
		}

		for (uint i = 0; i < 100; ++i)
		{
			EntityID	e = reg.CreateEntity< VDrawVertices, VGraphicsPipelineRef, VDescriptorSets, VPipeline, InvalidatePipelineTag, OpquePassTag >();

			auto&	ppln = *reg.GetComponent<VGraphicsPipelineRef>( e );
			ppln.name				= PipelineName{"draw1"};
			ppln.desc.subpassIndex	= 0;
			ppln.desc.viewportCount	= 1;
			ppln.desc.dynamicState	= EPipelineDynamicState::Viewport | EPipelineDynamicState::Scissor;
			ppln.desc.renderState.inputAssembly.topology = EPrimitive::TriangleList;

			auto&	ds = *reg.GetComponent<VDescriptorSets>( e );
			ds.value[0] = desc_sets[1];
			ds.value[1] = desc_sets[2];

			auto&	draw = *reg.GetComponent<VDrawVertices>( e );
			draw.firstVertex = 0;
			draw.vertexCount = 3;
		}
		
		const uint3		view_size {800, 600, 1};
		
		UniqueID<GfxResourceID>	image;
		{
			ImageDesc	desc;
			desc.dimension	= view_size;
			desc.format		= EPixelFormat::RGBA8_UNorm;
			desc.memType	= EMemoryType::DeviceLocal;
			desc.imageType	= EImage::_2D;
			desc.usage		= EImageUsage::ColorAttachment | EImageUsage::TransferSrc;

			image = resourceMngr.CreateImage( desc, Default, "RenderTarget", null );
			CHECK_FATAL( image );
		}

		renderGraph.Add(
			EQueueType::Graphics, {}, {},
			[&] (IGraphicsContext &ctx, ArrayView<GfxResourceID>, ArrayView<GfxResourceID>)
			{
				// initial layout transition
				ctx.SetInitialState( image, EResourceState::Unknown );
			});

		renderGraph.Add(
			EQueueType::Graphics, {}, {},
			RenderPassDesc{ RenderPassName{"ColorPass"}, uint2{view_size} }
				.AddTarget( RenderTargetName{"out_Color"}, image, RGBA32f(0.0f), EAttachmentStoreOp::Store )
				.AddViewport( uint2{view_size} ),
			[&] (IAsyncRenderContext &ctx, ArrayView<GfxResourceID>, ArrayView<GfxResourceID>)
			{
				auto	info = ctx.GetContextInfo();

				pass._renderPassId	= info.renderPass;
				pass._perPassDS		= desc_sets[0];

				pass.Update();
				pass.Render( ctx );
			});
		
		CmdBatchID	batch = renderGraph.Submit();
		CHECK_FATAL( batch );
		CHECK_FATAL( renderGraph.Wait({ batch }));

		resourceMngr.ReleaseResource( image );
		resourceMngr.ReleaseResource( buffer );

		reg.DestroyAllEntities();
	}

/*
=================================================
	CompilePipelines
=================================================
*/
	bool  CompilePipelines (VResourceManager &resMngr)
	{
	#ifdef PLATFORM_WINDOWS
		using namespace AE::PipelineCompiler;

		decltype(&PipelineCompiler::CompilePipelines)	compile_pipelines = null;

		Path	dll_path;
		CHECK_ERR( FileSystem::Search( "PipelineCompiler.dll", 3, 3, OUT dll_path ));

		Library		lib;
		CHECK_ERR( lib.Load( dll_path ));
		CHECK_ERR( lib.GetProcAddr( "CompilePipelines", OUT compile_pipelines ));
		
		CHECK_ERR( FileSystem::FindAndSetCurrent( "tests/ecs-st/Vulkan", 5 ));
	
		const wchar_t*	pipeline_folder[]	= { L"pipelines" };
		const wchar_t*	shader_folder[]		= { L"shaders" };
		Path			output				= L"temp";
	
		FileSystem::CreateDirectories( output );

		output.append( L"pipelines.bin" );

		PipelinesInfo	info = {};
		info.inPipelines		= null;
		info.inPipelineCount	= 0;
		info.pipelineFolders	= pipeline_folder;
		info.pipelineFolderCount= uint(CountOf( pipeline_folder ));
		info.includeDirs		= shader_folder;
		info.includeDirCount	= uint(CountOf( shader_folder ));
		info.shaderFolders		= shader_folder;
		info.shaderFolderCount	= uint(CountOf( shader_folder ));
		info.outputPackName		= output.c_str();

		CHECK_ERR( compile_pipelines( &info ));

		auto	file = MakeShared<FileRStream>( output );
		CHECK_ERR( file->IsOpen() );

		pipelinePack = resMngr.LoadPipelinePack( file );
		CHECK_ERR( pipelinePack );
	#endif

		return true;
	}
}


extern void UnitTest_RendererVk ()
{
	{
		CHECK_FATAL( Scheduler().Setup( 1 ));
		Scheduler().AddThread( MakeShared<WorkerThread>() );

		VDeviceInitializer	dev;
		CHECK_FATAL( dev.CreateInstance( "TestApp", "AE", dev.GetRecomendedInstanceLayers() ));
		CHECK_FATAL( dev.ChooseHighPerformanceDevice() );
		CHECK_FATAL( dev.CreateLogicalDevice( Default, Default ));

		UniquePtr<VResourceManager>	resourceMngr{ new VResourceManager{ dev }};
		CHECK_FATAL( resourceMngr->Initialize() );
		CHECK_FATAL( CompilePipelines( *resourceMngr ));
		
		UniquePtr<VRenderGraph>		renderGraph{ new VRenderGraph{ *resourceMngr }};
		CHECK_FATAL( renderGraph->Initialize() );
		

		GraphicsPass_Test1( *resourceMngr, *renderGraph );
		Scheduler().ProcessTask( IAsyncTask::EThread::Main, 0 );
		

		renderGraph->Deinitialize();
		renderGraph.reset();

		resourceMngr->ReleaseResource( pipelinePack );
		resourceMngr->Deinitialize();
		resourceMngr.reset();

		CHECK_FATAL( dev.DestroyLogicalDevice() );
		CHECK_FATAL( dev.DestroyInstance() );
		
		Scheduler().Release();
	}
	AE_LOGI( "UnitTest_RendererVk - passed" );
}

#endif	// AE_ENABLE_VULKAN
