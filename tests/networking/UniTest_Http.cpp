// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "UnitTest_Common.h"

namespace
{
	using EMethod	= RequestDesc::EMethod;
	using ECode		= RequestTask::ECode;
	using EStatus	= IAsyncTask::EStatus;
	using Seconds	= RequestDesc::Seconds;


	void Http_Test1 ()
	{
		LocalNetwork	network;

		// check status
		{
			auto	task1 = network->Send( RequestDesc{"https://httpbin.org/status/200"}.Method( EMethod::Get ));
			TEST( Scheduler().Wait( {task1} ));
			TEST( task1->Response()->code == ECode::OK );

			auto	task2 = network->Send( RequestDesc{"https://httpbin.org/status/204"}.Method( EMethod::Get ));
			TEST( Scheduler().Wait( {task2} ));
			TEST( task2->Response()->code == ECode::NoContent );
		}

		// test methods
		{
			auto	task1 = network->Send( RequestDesc{"https://httpbin.org/put"}.Method( EMethod::Get ));
			TEST( Scheduler().Wait( {task1} ));
			TEST( task1->Response()->code == ECode::MethodNotAllowed );

			auto	task2 = network->Send( RequestDesc{"https://httpbin.org/get"}.Method( EMethod::Get ));
			TEST( Scheduler().Wait( {task2} ));
			TEST( task2->Response()->code == ECode::OK );

			auto	task3 = network->Send( RequestDesc{"https://httpbin.org/put"}.Method( EMethod::Put ));
			TEST( Scheduler().Wait( {task3} ));
			TEST( task3->Response()->code == ECode::OK );

			auto	task4 = network->Send( RequestDesc{"https://httpbin.org/post"}.Method( EMethod::Post ));
			TEST( Scheduler().Wait( {task4} ));
			TEST( task4->Response()->code == ECode::OK );
		}

		// cancel
		{
			auto	task1 = network->Send( RequestDesc{"https://httpbin.org/delay/5"}.Method( EMethod::Get ));
			TEST( Scheduler().Cancel( task1 ));
			TEST( Scheduler().Wait( {task1} ));
			TEST( task1->Status() == EStatus::Canceled );
			TEST( task1->Response()->code == ECode::Unknown );
		}

		// downloading
		{
			auto	task1 = network->Send( RequestDesc{"https://httpbin.org/image/png"}.Method( EMethod::Head ));
			TEST( Scheduler().Wait( {task1} ));

			auto	resp1 = task1->Response();
			TEST( resp1->code == ECode::OK );
			TEST( resp1->headers.count( "Content-Type" ));
			TEST( resp1->headers.count( "Content-Length" ));
			TEST( resp1->headers["Content-Type"] == "image/png" );
			TEST( std::stoull( resp1->headers["Content-Length"] ) == 8090 );
			TEST( resp1->content.empty() );
			
			auto	task2 = network->Send( RequestDesc{"https://httpbin.org/image/png"}.Method( EMethod::Get ));
			TEST( Scheduler().Wait( {task2} ));

			auto	resp2 = task2->Response();
			TEST( resp2->code == ECode::OK );
			TEST( resp2->headers.count( "Content-Type" ));
			TEST( resp2->headers.count( "Content-Length" ));
			TEST( resp2->headers["Content-Type"] == "image/png" );
			TEST( std::stoull( resp2->headers["Content-Length"] ) == 8090 );
			TEST( resp2->content.size() == 8090 );
		}

		// redirections
		{
			auto	task1 = network->Send( RequestDesc{"https://httpbin.org/redirect/2"}.Method( EMethod::Get ));
			TEST( Scheduler().Wait( {task1} ));
			TEST( task1->Response()->code == ECode::OK );

			auto	task2 = network->Send( RequestDesc{"https://httpbin.org/redirect/2"}.Method( EMethod::Get ).Redirections( 1 ));
			TEST( Scheduler().Wait( {task2} ));
			TEST( task2->Response()->code == ECode::Found );

			auto	task3 = network->Send( RequestDesc{"https://httpbin.org/absolute-redirect/3"}.Method( EMethod::Get ));
			TEST( Scheduler().Wait( {task3} ));
			TEST( task3->Response()->code == ECode::OK );

			auto	task4 = network->Send( RequestDesc{"https://httpbin.org/relative-redirect/4"}.Method( EMethod::Get ));
			TEST( Scheduler().Wait( {task4} ));
			TEST( task4->Response()->code == ECode::OK );
		}
	}
}


extern void UnitTest_Http ()
{
	Http_Test1();

	AE_LOGI( "UnitTest_Http - passed" );
}
