// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "platform/Public/Clock.h"

namespace AE::App
{
	
/*
=================================================
	constructor
=================================================
*/
	AppClock::AppClock ()
	{
		_start = TimePoint_t::clock::now();
	}
	
/*
=================================================
	Timestamp
=================================================
*/
	nanoseconds  AppClock::Timestamp () const
	{
		return TimePoint_t::clock::now() - _start;
	}

/*
=================================================
	Instance
=================================================
*/
	AppClock&  AppClock::Instance ()
	{
		static AppClock  inst;
		return inst;
	}

}	// AE::App
