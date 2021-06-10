// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#ifdef AE_ENABLE_VULKAN
# include "Test_VulkanRenderGraph.h"

namespace
{
	struct TestData
	{
		UniqueID<BufferID>		buf_1;
		UniqueID<BufferID>		buf_2;
		const Bytes				buf_size	= 128_b;
		Array<ubyte>			buffer_data;
		AsyncTask				result;
		RC<VCommandBatch>		batch;
		bool					isOK		= false;
	};


	template <typename Ctx>
	class CopyBufferTask final : public VRenderTask
	{
	public:
		TestData&	t;

		CopyBufferTask (TestData& t, RC<VCommandBatch> batch, StringView dbgName) :
			VRenderTask{ batch, dbgName },
			t{ t }
		{}

		void  Run () override
		{
			Ctx		ctx{ GetBatch() };

			if ( not ctx.IsValid() )
				return OnFailure();
			
			if ( not ctx.UpdateHostBuffer( t.buf_1, 0_b, ArraySizeOf(t.buffer_data), t.buffer_data.data() ))
				return OnFailure();

			ctx.CopyBuffer( t.buf_1, t.buf_2, {BufferCopy{ 0_b, 0_b, t.buf_size }});

			t.result = AsyncTask{ ctx.ReadBuffer( t.buf_2, 0_b, t.buf_size )
						.Then( [p = &t] (const BufferView &view)
								{
									bool	data_is_correct	= (view.DataSize() == p->buf_size);

									for (usize i = 0; data_is_correct and (i < p->buffer_data.size()); ++i)
									{
										bool	is_equal = (p->buffer_data[i] == Cast<uint8_t>(view.Parts().front().ptr)[i]);
										ASSERT( is_equal );

										data_is_correct &= is_equal;
									}
									p->isOK = data_is_correct;
								})};

			Execute( ctx );

			CHECK( t.batch->Submit() );
		}
	};

	
	template <typename Ctx>
	static bool  CopyBufferTest ()
	{
		auto&		rg		 = RenderGraph();
		auto&		res_mngr = rg.GetResourceManager();
		TestData	t;

		t.buf_1 = res_mngr.CreateBuffer( BufferDesc{ t.buf_size, EBufferUsage::TransferSrc, Default, EMemoryType::HostCached  }, "buf_1" );
		t.buf_2 = res_mngr.CreateBuffer( BufferDesc{ t.buf_size, EBufferUsage::Transfer,    Default, EMemoryType::DeviceLocal }, "buf_2" );
		CHECK_ERR( t.buf_1 and t.buf_2 );
	
		t.buffer_data.resize( uint(t.buf_size) );
		for (usize i = 0; i < t.buffer_data.size(); ++i) {
			t.buffer_data[i] = ubyte(i);
		}

		AsyncTask	begin	= rg.BeginFrame();
		CHECK_ERR( begin );

		t.batch	= rg.CreateBatch( EQueueType::Graphics );
		CHECK_ERR( t.batch );

		AsyncTask	task1	= t.batch->Add<CopyBufferTask<Ctx>>( MakeTuple(ArgRef(t)), MakeTuple(begin), "Copy buffer task" );
		CHECK_ERR( task1 );

		AsyncTask	end		= rg.EndFrame( MakeTuple(task1) );
		CHECK_ERR( end );

		CHECK_ERR( Scheduler().Wait({ end }));
		CHECK_ERR( end->Status() == EStatus::Completed );

		CHECK_ERR( rg.WaitAll() );
		
		CHECK_ERR( Scheduler().Wait({ t.result }));
		CHECK_ERR( t.result->Status() == EStatus::Completed );

		CHECK_ERR( t.isOK );

		res_mngr.ReleaseResource( t.buf_1 );
		res_mngr.ReleaseResource( t.buf_2 );
		return true;
	}

}	// namespace


bool VRGTest::Test_CopyBuffer1 ()
{
	CHECK_ERR( CopyBufferTest< VDirectTransferContext< EBarrierPlacement::PerResource >>());
	CHECK_ERR( CopyBufferTest< VDirectTransferContext< EBarrierPlacement::PerRange >>());
	
	CHECK_ERR( CopyBufferTest< VIndirectTransferContext< EBarrierPlacement::PerResource >>());
	CHECK_ERR( CopyBufferTest< VIndirectTransferContext< EBarrierPlacement::PerRange >>());
	
	CHECK_ERR( CopyBufferTest< VDirectComputeContext< EBarrierPlacement::PerResource >>());
	CHECK_ERR( CopyBufferTest< VDirectComputeContext< EBarrierPlacement::PerRange >>());
	
	CHECK_ERR( CopyBufferTest< VIndirectComputeContext< EBarrierPlacement::PerResource >>());
	CHECK_ERR( CopyBufferTest< VIndirectComputeContext< EBarrierPlacement::PerRange >>());
	
	CHECK_ERR( CopyBufferTest< VDirectGraphicsContext< EBarrierPlacement::PerResource >>());
	CHECK_ERR( CopyBufferTest< VDirectGraphicsContext< EBarrierPlacement::PerRange >>());
	
	CHECK_ERR( CopyBufferTest< VIndirectGraphicsContext< EBarrierPlacement::PerResource >>());
	CHECK_ERR( CopyBufferTest< VIndirectGraphicsContext< EBarrierPlacement::PerRange >>());
	
	CHECK_ERR( _CompareDumps( TEST_NAME ));

	AE_LOGI( TEST_NAME << " - passed" );
	return true;
}

#endif	// AE_ENABLE_VULKAN
