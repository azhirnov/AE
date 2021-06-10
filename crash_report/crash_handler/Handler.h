// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'
/*
	How to use crash reporting system:
	* Add 'CrashHandler' library to your application.
	* Build application with debug information ('Debug' or 'Profile' config).
	* Save .pdb or generate .sym file for your application.
	* Add 'CrashReportSender.exe' to application distribution kit.
	* Build and run server 'CrashReportServer'.
	* At application start call 'InitCrashHandler'.
*/

#pragma once

#include "stl/Utils/FileSystem.h"

namespace AE
{
	bool  InitCrashHandler (const STL::Path &minidumpPath, STL::Path logPath, STL::Path crashReporterPath,
							STL::WString userInfo, STL::WString symbolFileID, STL::WString url);

}	// AE
