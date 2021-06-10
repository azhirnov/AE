// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "threading/TaskSystem/TaskScheduler.h"
#include "stl/Containers/FixedString.h"

namespace AE::Threading
{

	//
	// Worker Thread
	//

	class WorkerThread : public IThread
	{
	// types
	public:
		using ThreadMask	= BitSet< uint(EThread::_Count) >;


	// variables
	private:
		std::thread				_thread;
		Atomic<uint>			_looping;
		const ThreadMask		_threadMask;
		const nanoseconds		_sleepStep;
		const nanoseconds		_maxSleepOnIdle;
		const FixedString<64>	_name;


	// methods
	public:
		WorkerThread ();
		WorkerThread (ThreadMask mask, nanoseconds sleepStep, nanoseconds maxSleepOnIdle, StringView name = "thread");

		bool  Attach (uint uid) override;
		void  Detach () override;

		NtStringView  DbgName () const override		{ return _name; }
	};


}	// AE::Threading
