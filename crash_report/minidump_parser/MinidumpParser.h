// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Stream/Stream.h"
#include "stl/Utils/FileSystem.h"

namespace AE::MinidumpParser
{
	using namespace AE::STL;


	struct CrashInfo
	{
		struct {
			String		os;
			String		cpu;
			String		gpu;
		}			system;
		String		reason;		// crash reason
		uint64_t	address;	// crash address
		String		callstack;	// short stack-trace
		WString		userInfo;	// only from crash report
		String		log;		// only from crash report
	};

	bool  ParseMinidump (const Path &minidumpPath, const Path &symbolsPath, uint callStackDepth, OUT CrashInfo &info);

	bool  ParseCrashReport (const SharedPtr<RStream> &crashReport, Path symbolsFolder, uint callStackDepth, OUT CrashInfo &info);


}	// AE::MinidumpParser
