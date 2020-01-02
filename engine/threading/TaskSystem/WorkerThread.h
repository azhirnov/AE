// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "threading/TaskSystem/TaskScheduler.h"
#include "stl/Containers/StaticString.h"

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
		using Milliseconds	= std::chrono::duration<uint, std::milli>;


	// variables
	private:
		std::thread				_thread;
		Atomic<uint>			_looping;
		const ThreadMask		_threadMask;
		const Milliseconds		_sleepOnIdle;
		const StaticString<64>	_name;


	// methods
	public:
		WorkerThread ();
		WorkerThread (ThreadMask mask, Milliseconds sleepOnIdle, StringView name = "thread");

		bool  Attach (uint uid) override;
		void  Detach () override;

		NtStringView  DbgName () const override		{ return _name; }
	};


}	// AE::Threading
