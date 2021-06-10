// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Common.h"

namespace AE::STL
{

	//
	// Noncopyable
	//

	class Noncopyable
	{
	public:
		Noncopyable () = default;

		Noncopyable (const Noncopyable &) = delete;
		Noncopyable (Noncopyable &&) = delete;

		Noncopyable& operator = (const Noncopyable &) = delete;
		Noncopyable& operator = (Noncopyable &&) = delete;
	};


	//
	// Movable Only
	//

	class MovableOnly
	{
	public:
		MovableOnly () = default;

		MovableOnly (MovableOnly &&) = default;
		MovableOnly& operator = (MovableOnly &&) = default;

		MovableOnly (const MovableOnly &) = delete;
		MovableOnly& operator = (const MovableOnly &) = delete;
	};


}	// AE::STL
