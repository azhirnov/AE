// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "threading/TaskScheduler.h"
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


	// variables
	private:
		std::thread				_thread;
		std::atomic<uint>		_looping	{1};
		const ThreadMask		_threadMask;
		const bool				_canSleep;
		const StaticString<64>	_name;

		static constexpr auto	_sleepTime	= std::chrono::milliseconds{ 10 };


	// methods
	public:
		WorkerThread ();
		WorkerThread (ThreadMask mask, bool canSleep);

		bool  Attach (Ptr<TaskScheduler>, uint uid) override;
		void  Detach () override;

		NtStringView  DbgName () const override		{ return _name; }
	};


}	// AE::Threading
