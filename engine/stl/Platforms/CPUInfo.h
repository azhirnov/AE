// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Common.h"

namespace AE::STL
{

	//
	// CPU Info
	//

	struct CPUInfo
	{
	// x86-x64 features
		bool	AVX   : 1;
		bool	AVX2  : 1;

		bool	SSE2  : 1;
		bool	SSE3  : 1;
		bool	SSE41 : 1;
		bool	SSE42 : 1;

		bool	POPCNT : 1;

	// android features
		bool	NEON : 1;

	// shared features
		bool	CmpXchg16 : 1;		// 128 bit atomic compare exchange


	// methods
	private:
		CPUInfo ();

	public:
		ND_ static CPUInfo const&  Get ();
	};

}	// AE::STL
