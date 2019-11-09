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
		WorkerThread{ ThreadMask{}.set(uint(EThread::Worker)), Milliseconds{4} }
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
		_thread = std::thread{[this, uid] ()
		{
			uint	seed			= uid;
			uint	idle_counter	= 0;

			PlatformUtils::SetThreadName( _name );
			AE_VTUNE( __itt_thread_set_name( _name.c_str() ));
			
			_looping.store( 1, memory_order_relaxed );
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
					idle_counter = Max( 4u, idle_counter+1 );
					std::this_thread::sleep_for( _sleepOnIdle * idle_counter );
				}
				else
					idle_counter = 0;
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
		if ( _looping.exchange( 0, memory_order_relaxed ))
		{
			_thread.join();
		}
	}

}	// AE::Threading
