// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "threading/TaskSystem/TaskScheduler.h"

using namespace AE::Threading;

#define TEST	CHECK_FATAL


struct LocalTaskScheduler
{
	LocalTaskScheduler (size_t maxWorkerThreads)
	{
		Scheduler().Setup( maxWorkerThreads );
	}

	~LocalTaskScheduler ()
	{
		Scheduler().Release();
	}

	TaskScheduler* operator -> ()
	{
		return &Scheduler();
	}
};
