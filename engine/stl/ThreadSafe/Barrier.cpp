// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/ThreadSafe/Barrier.h"
#include "stl/Memory/MemUtils.h"

#if (AE_BARRIER_MODE == 0)

# include "stl/Platforms/WindowsHeader.h"

namespace AE::STL
{

/*
=================================================
	ArraySizeOf
=================================================
*/
	Barrier::Barrier (uint numThreads)
	{
		auto*	barrier_ptr = PlacementNew<SYNCHRONIZATION_BARRIER>( _data );

		CHECK( InitializeSynchronizationBarrier( barrier_ptr, numThreads, -1 ));
	}
	
/*
=================================================
	ArraySizeOf
=================================================
*/
	Barrier::~Barrier ()
	{
		CHECK( DeleteSynchronizationBarrier( BitCast<SYNCHRONIZATION_BARRIER *>( &_data[0] ) ));
	}
	
/*
=================================================
	wait
=================================================
*/
	void Barrier::wait ()
	{
		EnterSynchronizationBarrier( BitCast<SYNCHRONIZATION_BARRIER *>( &_data[0] ), SYNCHRONIZATION_BARRIER_FLAGS_NO_DELETE );
	}
	
}	// AE::STL
#endif	// AE_BARRIER_MODE == 0