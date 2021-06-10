// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

enum class ECrashReportSenderFlags : uint32_t
{
	Unknown				= 0,
	RemoveDumpFile		= 1 << 1,
	RemoveLogFile		= 1 << 2,
};
AE_BIT_OPERATORS( ECrashReportSenderFlags );
