// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

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
		const milliseconds		_sleepOnIdle;
		const FixedString<64>	_name;


	// methods
	public:
		WorkerThread ();
		WorkerThread (ThreadMask mask, milliseconds sleepOnIdle, StringView name = "thread");

		bool  Attach (uint uid) override;
		void  Detach () override;

		NtStringView  DbgName () const override		{ return _name; }
	};


}	// AE::Threading
