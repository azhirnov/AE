// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Platforms/WindowsProcess.h"
#include "stl/Platforms/WindowsUtils.h"

#include "stl/Algorithms/StringUtils.h"
#include "stl/Utils/Helpers.h"

#ifdef PLATFORM_WINDOWS
# include "stl/Platforms/WindowsHeader.h"

namespace AE::STL
{
	
/*
=================================================
	destructor
=================================================
*/
	WindowsProcess::~WindowsProcess ()
	{
		ASSERT( not _thread );
		ASSERT( not _process );
	}

/*
=================================================
	_ExecuteAsync
=================================================
*/
	template <typename T>
	bool  WindowsProcess::_ExecuteAsync (BasicString<T> &commandLine, const Path *currentDir, EFlags flags)
	{
		using STARTUPINFO_t = Conditional< IsSameTypes< T, char >, STARTUPINFOA, STARTUPINFOW >;

		HANDLE	stdout_read	 = null;
		HANDLE	stdout_write = null;
		
		OnDestroy( [&]() {
				if ( stdout_read )	{ ::CloseHandle( stdout_read );	 stdout_read = null; }
				if ( stdout_write )	{ ::CloseHandle( stdout_write ); stdout_write = null; }
			});
		
		STARTUPINFO_t			startup_info = {};
		PROCESS_INFORMATION		proc_info	 = {};

		if ( AllBits( flags, EFlags::ReadOutput ))
		{
			SECURITY_ATTRIBUTES			s_attr = {};
			s_attr.nLength				= sizeof(s_attr);
			s_attr.lpSecurityDescriptor	= null;
			s_attr.bInheritHandle		= TRUE;
		
			CHECK_ERR( ::CreatePipe( OUT &stdout_read, OUT &stdout_write, &s_attr, 0 ));
			CHECK_ERR( ::SetHandleInformation( stdout_read, HANDLE_FLAG_INHERIT, 0 ));
			
			startup_info.hStdError	= stdout_write;
			startup_info.hStdOutput	= stdout_write;
			startup_info.hStdInput	= null;
			startup_info.dwFlags	|= STARTF_USESTDHANDLES;
		}

		startup_info.cb = sizeof(startup_info);

		bool	result = false;
		
		if constexpr( IsSameTypes< T, char >)
		{
			if ( AllBits( flags, EFlags::UseCommandPromt ))
			{
				char	buf [MAX_PATH] = {};
				::GetSystemDirectoryA( buf, UINT(CountOf(buf)) );
				commandLine = "\""s << buf << "\\cmd.exe\" /C " << commandLine;
			}

			result = ::CreateProcessA(
				null,
				commandLine.data(),
				null,
				null,
				AllBits( flags, EFlags::ReadOutput ) ? TRUE : FALSE,
				AllBits( flags, EFlags::NoWindow ) ? CREATE_NO_WINDOW : CREATE_NEW_CONSOLE,
				null,
				(currentDir ? currentDir->string().c_str() : null),
				OUT &startup_info,
				OUT &proc_info
			);
		}
		else
		{
			if ( AllBits( flags, EFlags::UseCommandPromt ))
			{
				wchar_t	buf [MAX_PATH] = {};
				::GetSystemDirectoryW( buf, UINT(CountOf(buf)) );
				commandLine = L"\""s << buf << L"\\cmd.exe\" /C " << commandLine;
			}

			result = ::CreateProcessW(
				null,
				commandLine.data(),
				null,
				null,
				AllBits( flags, EFlags::ReadOutput ) ? TRUE : FALSE,
				AllBits( flags, EFlags::NoWindow ) ? CREATE_NO_WINDOW : CREATE_NEW_CONSOLE,
				null,
				(currentDir ? currentDir->wstring().c_str() : null),
				OUT &startup_info,
				OUT &proc_info
			);
		}
		
		if ( not result )
		{
			WindowsUtils::CheckErrors( __FILE__, __LINE__ );
			return false;
		}

		_thread			= proc_info.hThread;
		_process		= proc_info.hProcess;
		_streamWrite	= stdout_write;
		_streamRead		= stdout_read;
		_flags			= flags;

		stdout_write = null;
		stdout_read  = null;

		return true;
	}
	
/*
=================================================
	Execute
=================================================
*/
	bool  WindowsProcess::Execute (String &commandLine, EFlags flags, milliseconds timeout)
	{
		WindowsProcess	proc;
		bool			result = true;

		result &= proc._ExecuteAsync( commandLine, null, flags );
		result &= proc.WaitAndClose( timeout );
		return result;
	}

	bool  WindowsProcess::Execute (WString &commandLine, EFlags flags, milliseconds timeout)
	{
		WindowsProcess	proc;
		bool			result = true;

		result &= proc._ExecuteAsync( commandLine, null, flags );
		result &= proc.WaitAndClose( timeout );
		return result;
	}

	bool  WindowsProcess::Execute (String &commandLine, INOUT String &output, std::mutex *outputGuard, milliseconds timeout)
	{
		WindowsProcess	proc;
		bool			result = true;

		result &= proc._ExecuteAsync( commandLine, null, EFlags::ReadOutput );
		result &= proc.WaitAndClose( INOUT output, outputGuard, timeout );
		return result;
	}

	bool  WindowsProcess::Execute (WString &commandLine, INOUT String &output, std::mutex *outputGuard, milliseconds timeout)
	{
		WindowsProcess	proc;
		bool			result = true;

		result &= proc._ExecuteAsync( commandLine, null, EFlags::ReadOutput );
		result &= proc.WaitAndClose( INOUT output, outputGuard, timeout );
		return result;
	}
	
/*
=================================================
	ExecuteAsync
=================================================
*/
	bool  WindowsProcess::ExecuteAsync (String &commandLine, EFlags flags)
	{
		return _ExecuteAsync( commandLine, null, flags );
	}

	bool  WindowsProcess::ExecuteAsync (WString &commandLine, EFlags flags)
	{
		return _ExecuteAsync( commandLine, null, flags );
	}
	
	bool  WindowsProcess::ExecuteAsync (String &commandLine, const Path &currentDir, EFlags flags)
	{
		return _ExecuteAsync( commandLine, &currentDir, flags );
	}

	bool  WindowsProcess::ExecuteAsync (WString &commandLine, const Path &currentDir, EFlags flags)
	{
		return _ExecuteAsync( commandLine, &currentDir, flags );
	}

/*
=================================================
	Terminate
=================================================
*/
	bool  WindowsProcess::Terminate (milliseconds timeout)
	{
		if ( _thread == null or _process == null )
			return true;
		
		bool	result = true;

		result &= (TerminateProcess( _process, 1 ) == TRUE);
		result &= WaitAndClose( timeout );

		return result;
	}
	
/*
=================================================
	WaitAndClose
=================================================
*/
	bool  WindowsProcess::WaitAndClose (milliseconds timeout)
	{
		if ( _thread == null or _process == null )
			return true;
		
		bool	result = false;

		if ( ::WaitForSingleObject( _thread, DWORD(timeout.count()) ) == WAIT_OBJECT_0 )
		{
			DWORD process_exit;
			::GetExitCodeProcess( _process, OUT &process_exit );

			ASSERT( process_exit != STILL_ACTIVE );
			result = true;
		}
		
		::CloseHandle( _thread );
		::CloseHandle( _process );
		
		if ( _streamWrite and _streamRead )
		{
			::CloseHandle( _streamWrite );
			::CloseHandle( _streamRead );
		}

		_thread = _process = _streamWrite = _streamRead = null;
		_flags = Default;
		return result;
	}
	
/*
=================================================
	WaitAndClose
=================================================
*/
	bool  WindowsProcess::WaitAndClose (INOUT String &output, std::mutex *outputGuard, milliseconds timeout)
	{
		if ( _thread == null or _process == null )
			return true;

		bool	result = false;

		if ( ::WaitForSingleObject( _thread, DWORD(timeout.count()) ) == WAIT_OBJECT_0 )
		{
			DWORD process_exit;
			::GetExitCodeProcess( _process, OUT &process_exit );

			ASSERT( process_exit != STILL_ACTIVE );
			result = true;
		}
		
		::CloseHandle( _thread );
		::CloseHandle( _process );

		ASSERT( AllBits( _flags, EFlags::ReadOutput ));

		if ( _streamWrite and _streamRead )
		{
			::CloseHandle( _streamWrite );
		
			CHAR	buf [1 << 12];
			for (;;)
			{
				DWORD	readn;
				BOOL	success = ::ReadFile( _streamRead, buf, DWORD(CountOf(buf)), OUT &readn, null );

				if( not success or readn == 0 )
					break;

				if ( outputGuard )
					outputGuard->lock();

				output << StringView{ buf, readn };

				if ( outputGuard )
					outputGuard->unlock();
			}

			::CloseHandle( _streamRead );
		}
		
		_thread = _process = _streamWrite = _streamRead = null;
		_flags = Default;
		return result;
	}
	
/*
=================================================
	IsActive
=================================================
*/
	bool  WindowsProcess::IsActive () const
	{
		if ( _thread == null or _process == null )
			return false;
		
		if ( ::WaitForSingleObject( _thread, 1 ) == WAIT_OBJECT_0 )
			return false;

		return true;
	}


}	// AE::STL

#endif	// PLATFORM_WINDOWS
