// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "threading/TaskSystem/TaskScheduler.h"

using namespace AE::Threading;

#define TEST	CHECK_FATAL


struct LocalTaskScheduler
{
	LocalTaskScheduler (usize maxWorkerThreads)
	{
		TaskScheduler::CreateInstance();
		Scheduler().Setup( maxWorkerThreads );
	}

	~LocalTaskScheduler ()
	{
		Scheduler().Release();
		TaskScheduler::DestroyInstance();
	}

	TaskScheduler* operator -> ()
	{
		return &Scheduler();
	}
};
