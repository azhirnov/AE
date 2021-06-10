// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#include "threading/Primitives/SyncEvent.h"

#if (AE_SYNC_EVENT_MODE == 0)
# include "stl/Platforms/WindowsHeader.h"

namespace AE::Threading
{
	
/*
=================================================
	constructor
=================================================
*/
	SyncEvent::SyncEvent (EFlags flags)
	{
		STATIC_ASSERT( sizeof(_event) == sizeof(HANDLE) );

		_event = ::CreateEvent( null,
								not AllBits( flags, EFlags::AutoReset ),
								AllBits( flags, EFlags::InitStateSignaled ),
								(const char *)null );
	}
	
/*
=================================================
	destructor
=================================================
*/
	SyncEvent::~SyncEvent ()
	{
		auto	res = ::CloseHandle( _event );
		Unused( res );
		ASSERT( res != FALSE );
	}

/*
=================================================
	Signal
=================================================
*/
	void  SyncEvent::Signal ()
	{
		auto	res = ::SetEvent( static_cast<HANDLE>( _event ));
		Unused( res );
		ASSERT( res != FALSE );
	}
	
/*
=================================================
	Reset
=================================================
*/
	void  SyncEvent::Reset ()
	{
		auto	res = ::ResetEvent( static_cast<HANDLE>( _event ));
		Unused( res );
		ASSERT( res != FALSE );
	}
	
/*
=================================================
	_Wait
=================================================
*/
	bool  SyncEvent::_Wait (uint timeoutMS)
	{
		return ::WaitForSingleObject( static_cast<HANDLE>( _event ), timeoutMS ) == WAIT_OBJECT_0;
	}


}	// AE::Threading

#endif
