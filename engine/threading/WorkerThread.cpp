// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "threading/WorkerThread.h"
#include "stl/Platforms/PlatformUtils.h"

namespace AE::Threading
{
	
/*
=================================================
	constructor
=================================================
*/
	WorkerThread::WorkerThread () :
		WorkerThread{ ThreadMask{}.set(uint(EThread::Worker)), true }
	{}

	WorkerThread::WorkerThread (ThreadMask mask, bool canSleep) :
		_threadMask{mask}, _canSleep{canSleep}
	{}

/*
=================================================
	Attach
=================================================
*/
	bool  WorkerThread::Attach (Ptr<TaskScheduler> scheduler, uint uid)
	{
		_thread = std::thread{[this, scheduler, uid] ()
		{
			PlatformUtils::SetThreadName( _name );
			AE_VTUNE( __itt_thread_set_name( _name.c_str() ));

			for (; _looping.load( memory_order_relaxed );)
			{
				bool	processed = false;

				for (uint t = 0; t < _threadMask.size(); ++t)
				{
					if ( not _threadMask[t] )
						continue;

					processed |= scheduler->ProcessTask( EThread(t), uid );	// TODO: dynamic seed
				}

				if ( not processed and _canSleep )
				{
					std::this_thread::sleep_for( _sleepTime );
				}
			}
		}};
		return true;
	}
	
/*
=================================================
	Detach
=================================================
*/
	void  WorkerThread::Detach ()
	{
		_looping.store( 0, memory_order_relaxed );
		_thread.join();
	}

}	// AE::Threading
