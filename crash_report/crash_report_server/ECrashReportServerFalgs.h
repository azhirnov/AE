// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

enum class ECrashReportServerFalgs : uint32_t
{
	Unknown				= 0,
	ReceiveAndClose		= 1 << 1,		// for tests
};
