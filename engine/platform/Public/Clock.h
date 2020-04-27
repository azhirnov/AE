// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "platform/Public/Common.h"

namespace AE::App
{
	using Nanoseconds	= std::chrono::nanoseconds;



	//
	// Clock
	//

	class AppClock
	{
	// types
	private:
		using TimePoint_t	= std::chrono::high_resolution_clock::time_point;


	// variables
	private:
		TimePoint_t		_start;


	// methods
	public:
		AppClock ();

		ND_ Nanoseconds  Timestamp () const;

		ND_ static AppClock&  Instance ();
	};


	ND_ forceinline AppClock&  Clock ()
	{
		return AppClock::Instance();
	}

}	// AE::App
