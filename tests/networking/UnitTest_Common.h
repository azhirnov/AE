// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "networking/NetworkManager.h"

using namespace AE::Threading;
using namespace AE::Networking;

#define TEST	CHECK_FATAL


struct LocalNetwork
{
	LocalNetwork (const NetworkManager::Settings &settings = Default)
	{
		Scheduler().Setup( 1 );
		NetworkManager::Instance().Setup( settings );
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
