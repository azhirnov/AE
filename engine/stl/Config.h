// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once


#ifdef AE_DEBUG
#	define AE_ENABLE_DATA_RACE_CHECK
#else
//#	define AE_OPTIMAL_MEMORY_ORDER
#endif


#define AE_FAST_HASH	0
