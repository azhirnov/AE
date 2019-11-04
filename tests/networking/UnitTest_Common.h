// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "networking/NetworkManager.h"

using namespace AE::Threading;
using namespace AE::Networking;

#define TEST	CHECK_FATAL


struct LocalNetwork
{
	LocalNetwork ()
	{
		Scheduler().Setup( 1 );
		NetworkManager::Instance().Setup();
	}

	~LocalNetwork ()
	{
		Scheduler().Release();
		NetworkManager::Instance().Release();
	}

	NetworkManager* operator -> ()
	{
		return &NetworkManager::Instance();
	}
};
