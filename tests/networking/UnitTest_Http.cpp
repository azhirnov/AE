// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "threading/TaskSystem/WorkerThread.h"
#include "UnitTest_Common.h"

namespace
{
	using EMethod		= RequestDesc::EMethod;
	using ECode			= RequestTask::ECode;
	using EStatus		= IAsyncTask::EStatus;
	using MilliSeconds	= RequestDesc::MilliSeconds;


	void Http_Test1 ()
	{
		LocalNetwork	network;

		// check status
		{
			auto	task1 = network->Send( RequestDesc{"https://httpbin.org/status/200"}.Method( EMethod::Get ));
			TEST( Scheduler().Wait( {task1} ));
			TEST( task1->Status() == EStatus::Completed );
			TEST( task1->Response()->code == ECode::OK );

			auto	task2 = network->Send( RequestDesc{"https://httpbin.org/status/204"}.Method( EMethod::Get ));
			TEST( Scheduler().Wait( {task2} ));
			TEST( task2->Status() == EStatus::Completed );
			TEST( task2->Response()->code == ECode::NoContent );
		}

		// test methods
		{
			auto	task1 = network->Send( RequestDesc{"https://httpbin.org/put"}.Method( EMethod::Get ));
			TEST( Scheduler().Wait( {task1} ));
			TEST( task1->Status() == EStatus::Completed );
			TEST( task1->Response()->code == ECode::MethodNotAllowed );

			auto	task2 = network->Send( RequestDesc{"https://httpbin.org/get"}.Method( EMethod::Get ));
			TEST( Scheduler().Wait( {task2} ));
			TEST( task2->Status() == EStatus::Completed );
			TEST( task2->Response()->code == ECode::OK );

			auto	task3 = network->Send( RequestDesc{"https://httpbin.org/put"}.Method( EMethod::Put ));
			TEST( Scheduler().Wait( {task3} ));
			TEST( task3->Status() == EStatus::Completed );
			TEST( task3->Response()->code == ECode::OK );

			auto	task4 = network->Send( RequestDesc{"https://httpbin.org/post"}.Method( EMethod::Post ));
			TEST( Scheduler().Wait( {task4} ));
			TEST( task4->Status() == EStatus::Completed );
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
			TEST( task1->Status() == EStatus::Completed );

			auto	resp1 = task1->Response();
			TEST( resp1->code == ECode::OK );
			TEST( resp1->headers.count( "Content-Type" ));
			TEST( resp1->headers.count( "Content-Length" ));
			TEST( resp1->headers["Content-Type"] == "image/png" );
			TEST( std::stoull( resp1->headers["Content-Length"] ) == 8'090 );
			TEST( resp1->content.empty() );
			
			auto	task2 = network->Send( RequestDesc{"https://httpbin.org/image/png"}.Method( EMethod::Get ));
			TEST( Scheduler().Wait( {task2} ));
			TEST( task2->Status() == EStatus::Completed );

			auto	resp2 = task2->Response();
			TEST( resp2->code == ECode::OK );
			TEST( resp2->headers.count( "Content-Type" ));
			TEST( resp2->headers.count( "Content-Length" ));
			TEST( resp2->headers["Content-Type"] == "image/png" );
			TEST( std::stoull( resp2->headers["Content-Length"] ) == 8'090 );
			TEST( resp2->content.size() == 8'090 );
		}

		// redirections
		{
			auto	task1 = network->Send( RequestDesc{"https://httpbin.org/redirect/2"}.Method( EMethod::Get ));
			TEST( Scheduler().Wait( {task1} ));
			TEST( task1->Status() == EStatus::Completed );
			TEST( task1->Response()->code == ECode::OK );

			auto	task2 = network->Send( RequestDesc{"https://httpbin.org/redirect/2"}.Method( EMethod::Get ).Redirections( 1 ));
			TEST( Scheduler().Wait( {task2} ));
			TEST( task2->Status() == EStatus::Completed );
			TEST( task2->Response()->code == ECode::Found );

			auto	task3 = network->Send( RequestDesc{"https://httpbin.org/absolute-redirect/3"}.Method( EMethod::Get ));
			TEST( Scheduler().Wait( {task3} ));
			TEST( task3->Status() == EStatus::Completed );
			TEST( task3->Response()->code == ECode::OK );

			auto	task4 = network->Send( RequestDesc{"https://httpbin.org/relative-redirect/4"}.Method( EMethod::Get ));
			TEST( Scheduler().Wait( {task4} ));
			TEST( task4->Status() == EStatus::Completed );
			TEST( task4->Response()->code == ECode::OK );
		}
	}


	void Http_Test2 ()
	{
		NetworkManager::Settings	settings;
		settings.transferTimout = MilliSeconds(1000);

		LocalNetwork	network{ settings };
		
		// timeout
		{
			auto	task1 = network->Send( RequestDesc{"https://httpbin.org/delay/10"}.Method( EMethod::Get ));
			TEST( Scheduler().Wait( {task1} ));
			TEST( task1->Status() == EStatus::Completed );
			TEST( task1->Response()->code == ECode::OperationTimeout );
		}
	}
	

	void Http_Test3 ()
	{
		NetworkManager::Settings	settings;
		settings.responseDelay	= MilliSeconds(1000);

		LocalNetwork	network{ settings };

		// delay
		{
			auto	task1 = network->Send( RequestDesc{"https://httpbin.org/delay/10"}.Method( EMethod::Get ));
			TEST( Scheduler().Wait( {task1} ));
			TEST( task1->Status() == EStatus::Completed );
			TEST( task1->Response()->code == ECode::TimeoutAfterLastResponse );
		}
	}


	void Http_Test4 ()
	{
		LocalNetwork	network;
		Scheduler().AddThread( MakeShared<WorkerThread>() );

		{
			Atomic<bool>	p1_ok = false;

			auto	p0 = network->Download( "https://httpbin.org/image/png" );
			auto	p1 = p0.Then( [&p1_ok] (const Array<uint8_t> &content) { p1_ok = (content.size() == 8'090); });

			TEST( Scheduler().Wait( {AsyncTask(p1)} ));
			TEST( AsyncTask(p1)->Status() == EStatus::Completed );
			TEST( p1_ok );
		}
		
		/*{
			Atomic<bool>	p1_ok = false;
			Atomic<bool>	p2_ok = false;
			Atomic<bool>	p3_ok = false;

			auto	p0 = network->Download( "https://httpbin.org/image11111" );
			auto	p1 = p0.Then( [&p1_ok] (const Array<uint8_t> &) { p1_ok = true; });
			auto	p2 = p0.Except( [&p2_ok] () { p2_ok = true; });
			auto	p3 = p2.Then( [&p3_ok] () { p3_ok = true; });

			TEST( Scheduler().Wait({ AsyncTask(p1), AsyncTask(p2), AsyncTask(p3) }));
			TEST( AsyncTask(p1)->Status() == EStatus::Canceled );
			TEST( AsyncTask(p2)->Status() == EStatus::Canceled );
			TEST( AsyncTask(p3)->Status() == EStatus::Completed );
			TEST( not p1_ok );
			TEST( p2_ok );
			TEST( p3_ok );
		}*/
	}
}


extern void UnitTest_Http ()
{
	Http_Test1();
	Http_Test2();
	Http_Test3();
	Http_Test4();

	AE_LOGI( "UnitTest_Http - passed" );
}
