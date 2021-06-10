// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Platforms/CPUInfo.h"
#include "stl/Math/BitMath.h"

#ifdef COMPILER_MSVC
#	include <intrin.h>
#endif

#ifdef PLATFORM_ANDROID
#	include <sys/auxv.h>

# ifdef __arm__
#	include <asm/hwcap.h>
# endif
#endif

namespace AE::STL
{

#ifdef COMPILER_MSVC
/*
=================================================
	constructor
----
	https://docs.microsoft.com/en-us/cpp/intrinsics/cpuid-cpuidex?view=vs-2019
=================================================
*/
	CPUInfo::CPUInfo ()
	{
		std::memset( this, 0, sizeof(*this) );

		StaticArray<int, 4>	cpui;

		__cpuid( OUT cpui.data(), 0 );
		const int count = cpui[0];

		__cpuid( cpui.data(), 0x80000000 );
		const int ex_count = cpui[0];

		__cpuid( cpui.data(), 0x00000001 );
		
		SSE2	= AllBits( cpui[3], 1u << 26 );
		SSE3	= AllBits( cpui[2], 1u << 0 );
		SSE41	= AllBits( cpui[2], 1u << 19 );
		SSE42	= AllBits( cpui[2], 1u << 20 );
		AVX		= AllBits( cpui[2], 1u << 28 );
		POPCNT	= AllBits( cpui[2], 1u << 23 );

		CmpXchg16 = AllBits( cpui[2], 1u << 13 );
		
		__cpuid( cpui.data(), 0x00000007 );
		
		AVX2	= AllBits( cpui[1], 1u << 5 );
	}
#endif	// COMPILER_MSVC
	

#ifdef PLATFORM_ANDROID
/*
=================================================
	constructor
=================================================
*/
	CPUInfo::CPUInfo ()
	{
		std::memset( this, 0, sizeof(*this) );

		auto	caps  = getauxval(AT_HWCAP);
		auto	caps2 = getauxval(AT_HWCAP2);

	#ifdef __arm__
		NEON = AllBits( caps, HWCAP_NEON );
	#endif

	#ifdef __aarch64__
		NEON = true;
	#endif
	}
#endif	// PLATFORM_ANDROID

/*
=================================================
	SetCurrentThreadName
=================================================
*/
	CPUInfo const&  CPUInfo::Get ()
	{
		static const CPUInfo  info;
		return info;
	}

}	// AE::STL
