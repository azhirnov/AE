// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "threading/TaskSystem/WorkerThread.h"
#include "stl/Platforms/PlatformUtils.h"

namespace AE::Threading
{
	
/*
=================================================
	constructor
=================================================
*/
	WorkerThread::WorkerThread () :
		WorkerThread{ ThreadMask{}.set(uint(EThread::Worker)), Milliseconds{10} }
	{}

	WorkerThread::WorkerThread (ThreadMask mask, Milliseconds sleepOnIdle, StringView name) :
		_threadMask{ mask }, _sleepOnIdle{ sleepOnIdle }, _name{ name }
	{}

/*
=================================================
	Attach
=================================================
*/
	bool  WorkerThread::Attach (uint uid)
	{
		_looping.store( 1, memory_order_release );

		_thread = std::thread{[this, uid] ()
		{
			uint	seed = uid;

			PlatformUtils::SetThreadName( _name );
			AE_VTUNE( __itt_thread_set_name( _name.c_str() ));

			for (; _looping.load( memory_order_relaxed );)
			{
				bool	processed = false;

				for (uint t = 0; t < _threadMask.size(); ++t)
				{
					if ( not _threadMask[t] )
						continue;

					processed |= Scheduler().ProcessTask( EThread(t), ++seed );
				}

				if ( not processed and _sleepOnIdle.count() )
				{
					std::this_thread::sleep_for( _sleepOnIdle );
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
