// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

// mem leak check
#if defined(COMPILER_MSVC) && defined(AE_ENABLE_MEMLEAK_CHECKS) && defined(AE_DEBUG)
#	define _CRTDBG_MAP_ALLOC
#	include <stdlib.h>
#	include <crtdbg.h>

	// call at exit
	// returns 'true' if no mem leaks
#	define AE_DUMP_MEMLEAKS()	(::_CrtDumpMemoryLeaks() != 1)
#else

#	undef  AE_ENABLE_MEMLEAK_CHECKS
#	define AE_DUMP_MEMLEAKS()	(true)
#endif

// Config
#ifndef AE_FAST_HASH
#	define AE_FAST_HASH		0
#endif

#ifndef AE_OPTIMIZE_IDS
# if defined(AE_DEBUG) || defined(AE_DEVELOP)
#	define AE_OPTIMIZE_IDS	0
# else
#	define AE_OPTIMIZE_IDS	1
# endif
#endif

