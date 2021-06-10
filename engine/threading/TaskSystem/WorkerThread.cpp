// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

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
		WorkerThread{ ThreadMask{}.set(uint(EThread::Worker)), nanoseconds{4}, microseconds{10} }
	{}

	WorkerThread::WorkerThread (ThreadMask mask, nanoseconds sleepStep, nanoseconds maxSleepOnIdle, StringView name) :
		_threadMask{ mask }, _sleepStep{ sleepStep }, _maxSleepOnIdle{ maxSleepOnIdle }, _name{ name }
	{
		ASSERT( (_maxSleepOnIdle.count() == 0) == (_sleepStep.count() == 0) );
		ASSERT( _threadMask.to_ullong() != 0 );
	}

/*
=================================================
	Attach
=================================================
*/
	bool  WorkerThread::Attach (uint uid)
	{
		_looping.store( 1, EMemoryOrder::Relaxed );

		_thread = std::thread{[this, uid] ()
		{
			uint		seed			= uid;
			nanoseconds	sleep_time		= _sleepStep;
			uint		thread_id		= uid % std::thread::hardware_concurrency();

			PlatformUtils::SetThreadName( _name );
			AE_VTUNE( __itt_thread_set_name( _name.c_str() ));
			CHECK( PlatformUtils::SetThreadAffinity( _thread.native_handle(), thread_id ));
			
			for (; _looping.load( EMemoryOrder::Relaxed );)
			{
				uint	processed = 0;

				for (uint t = 0; t < _threadMask.size(); ++t)
				{
					if ( not _threadMask[t] )	// TODO: optimize
						continue;

					processed |= uint(Scheduler().ProcessTask( EThread(t), seed ));
				}
				
				if ( not processed )
					++seed;
					 
				// suspend thread
				if ( _maxSleepOnIdle.count() )
				{
					if ( not processed )
					{
						sleep_time = Min( sleep_time, _maxSleepOnIdle );

						std::this_thread::sleep_for( sleep_time );
						sleep_time += _sleepStep;
					}
					else
						sleep_time = _sleepStep;
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
		if ( _looping.exchange( 0, EMemoryOrder::Relaxed ))
		{
			_thread.join();
		}
	}

}	// AE::Threading
