// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

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
		WorkerThread{ ThreadMask{}.set(uint(EThread::Worker)), milliseconds{4} }
	{}

	WorkerThread::WorkerThread (ThreadMask mask, milliseconds sleepOnIdle, StringView name) :
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
			//CHECK( PlatformUtils::SetThreadAffinity( _thread.native_handle(), uid ));
			
			_looping.store( 1, EMemoryOrder::Relaxed );
			for (; _looping.load( EMemoryOrder::Relaxed );)
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
					idle_counter = Min( 4u, idle_counter );
					std::this_thread::sleep_for( _sleepOnIdle * idle_counter );
					++idle_counter;
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
		if ( _looping.exchange( 0, EMemoryOrder::Relaxed ))
		{
			_thread.join();
		}
	}

}	// AE::Threading
