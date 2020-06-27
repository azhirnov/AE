// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "networking/HttpClient.h"

using namespace AE::Threading;
using namespace AE::Networking;

#define TEST	CHECK_FATAL


struct LocalHttpClient
{
	LocalHttpClient (const HttpClient::Settings &settings = Default)
	{
		Scheduler().Setup( 1 );
		HttpClient::Instance().Setup( settings );
	}

	~LocalHttpClient ()
	{
		Scheduler().Release();
		HttpClient::Instance().Release();
	}

	HttpClient* operator -> ()
	{
		return &HttpClient::Instance();
	}
};
