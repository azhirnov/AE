// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "threading/TaskSystem/TaskScheduler.h"

namespace AE::Threading
{

	//
	// Function Async Task
	//

	class FunctionTask final : public IAsyncTask
	{
	// types
	public:
		using Func_t	= Function< void () >;

	// variables
	private:
		Func_t	_func;

	// methods
	public:
		FunctionTask (Func_t &&fn, EThread type = EThread::Worker) :
			IAsyncTask{ type }, _func{ std::move(fn) }
		{}

		template <typename Fn>
		FunctionTask (Fn &&fn, EThread type = EThread::Worker) :
			IAsyncTask{ type }, _func{ std::forward<Fn>(fn) }
		{}

	private:
		void Run () override
		{
			return _func();
		}
	};


}	// AE::Threading
