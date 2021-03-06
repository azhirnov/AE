// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Platforms/WindowsUtils.h"
#include "stl/Platforms/WindowsHeader.h"
#include "stl/Algorithms/ArrayUtils.h"

#ifdef PLATFORM_WINDOWS
# include "stl/Platforms/PlatformUtils.h"
# include <processthreadsapi.h>
# include <thread>

namespace AE::STL
{
namespace
{
/*
=================================================
	Kernel32dll
=================================================
*/
	struct Kernel32Lib
	{
	// variables
	private:
		Library		_lib;

	public:
		decltype(&::GetThreadDescription)	getThreadDescription	= null;
		decltype(&::SetThreadDescription)	setThreadDescription	= null;
	

	// methods
	public:
		Kernel32Lib ()
		{
			_lib.Load( "kernel32.dll" );

			if ( _lib )
			{
				_lib.GetProcAddr( "GetThreadDescription", OUT getThreadDescription );
				_lib.GetProcAddr( "SetThreadDescription", OUT setThreadDescription );
			}
		}
	};

	ND_ static Kernel32Lib&  Kernel32dll ()
	{
		static Kernel32Lib	lib;
		return lib;
	}
	
/*
=================================================
	SetCurrentThreadNameXP
=================================================
*/
	#pragma pack(push,8)
	typedef struct tagTHREADNAME_INFO
	{
		DWORD dwType; // Must be 0x1000.  
		LPCSTR szName; // Pointer to name (in user addr space).
		DWORD dwThreadID; // Thread ID (-1=caller thread).
		DWORD dwFlags; // Reserved for future use, must be zero.
	 } THREADNAME_INFO;
	#pragma pack(pop)

	void SetCurrentThreadNameXP (const char* name)
	{
		constexpr DWORD MS_VC_EXCEPTION = 0x406D1388;

		THREADNAME_INFO info;
		info.dwType = 0x1000;
		info.szName = name;
		info.dwThreadID = ::GetCurrentThreadId();
		info.dwFlags = 0;

	#pragma warning(push)
	#pragma warning(disable: 6320 6322)
		__try{
			::RaiseException( MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
		}
	#pragma warning(pop)
	}
	
/*
=================================================
	SetCurrentThreadName10
=================================================
*/
	bool SetCurrentThreadName10 (NtStringView name)
	{
		auto&	kernel = Kernel32dll();

		if ( not kernel.setThreadDescription )
			return false;

		WCHAR	str[256] = {};
		for (size_t i = 0, cnt = Min( name.size(), CountOf(str)-1 ); i < cnt; ++i) {
			str[i] = WCHAR(name.c_str()[i]);
		}

		HRESULT	hr = kernel.setThreadDescription( ::GetCurrentThread(), str );
		ASSERT( SUCCEEDED(hr) );
		Unused( hr );

		return true;
	}
	
/*
=================================================
	GetCurrentThreadName10
=================================================
*/
	bool GetCurrentThreadName10 (OUT String &name)
	{
		auto&	kernel = Kernel32dll();
		
		if ( not kernel.getThreadDescription )
			return false;

		PWSTR	w_name;
		HRESULT	hr = kernel.getThreadDescription( ::GetCurrentThread(), OUT &w_name );

		if ( not SUCCEEDED(hr) )
			return false;

		name.reserve( 128 );

		for (; *w_name; ++w_name) { 
			if ( (*w_name >= 0) & (*w_name < 128) )
				name.push_back( char(*w_name) );
		}

		::LocalFree( w_name );
		return true;
	}

}	// namespace
//-----------------------------------------------------------------------------


/*
=================================================
	SetCurrentThreadName
=================================================
*/
	void WindowsUtils::SetThreadName (NtStringView name)
	{
		if ( SetCurrentThreadName10( name ) )
			return;

		SetCurrentThreadNameXP( name.c_str() );
	}
	
/*
=================================================
	GetThreadName
=================================================
*/
	String  WindowsUtils::GetThreadName ()
	{
		String	name;

		GetCurrentThreadName10( OUT name );
		return name;
	}
	
/*
=================================================
	SetThreadAffinity
=================================================
*/
	bool  WindowsUtils::SetThreadAffinity (const std::thread::native_handle_type &handle, uint cpuCore)
	{
		DWORD_PTR	mask;
		
		//ASSERT( cpuCore < std::thread::hardware_concurrency() );

		#if PLATFORM_BITS == 64
		{
			ASSERT( cpuCore < 64 );
			mask = 1ull << (cpuCore & 63);
		}
		#elif PLATFORM_BITS == 32
		{
			ASSERT( cpuCore < 32 );
			mask = 1u << (cpuCore & 31);
		}
		#endif

		return ::SetThreadAffinityMask( handle, mask ) != 0;
	}
	
/*
=================================================
	CheckErrors
=================================================
*/
	bool  WindowsUtils::CheckErrors (StringView file, int line)
	{
		DWORD	dw = ::GetLastError();

		if ( dw == 0 )
			return true;
		
		char	buf[128] = {};
		DWORD	dw_count = ::FormatMessageA(
									FORMAT_MESSAGE_FROM_SYSTEM |
									FORMAT_MESSAGE_IGNORE_INSERTS,
									null,
									dw,
									MAKELANGID( LANG_ENGLISH, SUBLANG_DEFAULT ),
									LPTSTR(buf),
									uint(CountOf(buf)),
									null );
		
		String	str("WinAPI error: ");
		str += StringView{ buf, dw_count-2 };

		AE_LOGE( str, file, line );
		return false;
	}

}	// AE::STL

#endif	// PLATFORM_WINDOWS
