// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Algorithms/StringUtils.h"
#include "stl/Containers/NtStringView.h"
#include <iostream>

using namespace AE::STL;


/*
=================================================
	ConsoleOutput
=================================================
*/
namespace {
	void ConsoleOutput (StringView message, StringView file, int line, bool isError)
	{
		const String str = String{file} << '(' << ToString( line ) << "): " << message;

		if ( isError )
			std::cerr << str << std::endl;
		else
			std::cout << str << std::endl;
	}
}
/*
=================================================
	
=================================================
*/
	Logger::EResult  AE::STL::Logger::Info (const char *msg, const char *func, const char *file, int line)
	{
		return Info( StringView{msg}, StringView{func}, StringView{file}, line );
	}

	Logger::EResult  AE::STL::Logger::Error (const char *msg, const char *func, const char *file, int line)
	{
		return Error( StringView{msg}, StringView{func}, StringView{file}, line );
	}
//-----------------------------------------------------------------------------



// Android
#if	defined(PLATFORM_ANDROID)

#	include <android/log.h>

namespace {
	constexpr char	AE_ANDROID_TAG[] = "AE";
}

/*
=================================================
	Info
=================================================
*/
	Logger::EResult  AE::STL::Logger::Info (StringView msg, StringView func, StringView file, int line)
	{
		(void)__android_log_write( ANDROID_LOG_WARN, AE_ANDROID_TAG, (String{} << file << " (" << ToString( line ) << "): " << msg).c_str() );
		return EResult::Continue;
	}
	
/*
=================================================
	Error
=================================================
*/
	Logger::EResult  AE::STL::Logger::Error (StringView msg, StringView func, StringView file, int line)
	{
		(void)__android_log_write( ANDROID_LOG_ERROR, AE_ANDROID_TAG, (String{} << file << " (" << ToString( line ) << "): " << msg).c_str() );
		return EResult::Continue;
	}
//-----------------------------------------------------------------------------



// Windows
#elif defined(PLATFORM_WINDOWS)
	
#	include "stl/Platforms/WindowsHeader.h"

/*
=================================================
	IDEConsoleMessage
=================================================
*/
	static void IDEConsoleMessage (StringView message, StringView file, int line, bool isError)
	{
	#ifdef COMPILER_MSVC
		const String	str = String(file) << '(' << ToString( line ) << "): " << (isError ? "Error: " : "") << message << '\n';

		::OutputDebugStringA( str.c_str() );
	#endif
	}
	
/*
=================================================
	Info
=================================================
*/
	Logger::EResult  AE::STL::Logger::Info (StringView msg, StringView, StringView file, int line)
	{
		IDEConsoleMessage( msg, file, line, false );
		ConsoleOutput( msg, file, line, false );

		return EResult::Continue;
	}
	
/*
=================================================
	Error
=================================================
*/
	Logger::EResult  AE::STL::Logger::Error (StringView msg, StringView func, StringView file, int line)
	{
		IDEConsoleMessage( msg, file, line, true );
		ConsoleOutput( msg, file, line, true );

		#ifndef AE_CI_BUILD
		{
			const String	caption	= "Error message";

			const String	str		= "File: "s << file <<
									  "\nLine: " << ToString( line ) <<
									  "\nFunction: " << func <<
									  "\n\nMessage:\n" << msg;

			int	result = ::MessageBoxExA( null, NtStringView{str}.c_str(), NtStringView{caption}.c_str(),
										  MB_ABORTRETRYIGNORE | MB_ICONERROR | MB_SETFOREGROUND | MB_TOPMOST | MB_DEFBUTTON3,
										  MAKELANGID( LANG_ENGLISH, SUBLANG_ENGLISH_US ) );
			switch ( result )
			{
				case IDABORT :	return Logger::EResult::Abort;
				case IDRETRY :	return Logger::EResult::Break;
				case IDIGNORE :	return Logger::EResult::Continue;
				default :		return Logger::EResult(~0u);
			};
		}
		#else
			return Logger::EResult::Continue;
		#endif
	}
//-----------------------------------------------------------------------------



// OSX
#elif defined(PLATFORM_MACOS) or defined(PLATFORM_IOS)

/*
=================================================
	Info
=================================================
*/
	Logger::EResult  AE::STL::Logger::Info (StringView msg, StringView func, StringView file, int line)
	{
		ConsoleOutput( msg, file, line, false );
		return EResult::Continue;
	}

/*
=================================================
	Error
=================================================
*/
	Logger::EResult  AE::STL::Logger::Error (StringView msg, StringView func, StringView file, int line)
	{
		ConsoleOutput( msg, file, line, true );
		return EResult::Abort;
	}
//-----------------------------------------------------------------------------



// Linux
#elif defined(PLATFORM_LINUX)

// sudo apt-get install lesstif2-dev
//#include <Xm/Xm.h>
//#include <Xm/PushB.h>

/*
=================================================
	Info
=================================================
*/
	Logger::EResult  AE::STL::Logger::Info (StringView msg, StringView func, StringView file, int line)
	{
		ConsoleOutput( msg, file, line, false );
		return EResult::Continue;
	}
	
/*
=================================================
	Error
=================================================
*/
	Logger::EResult  AE::STL::Logger::Error (StringView msg, StringView func, StringView file, int line)
	{
		/*Widget top_wid, button;
		XtAppContext  app;

		top_wid = XtVaAppInitialize(&app, "Push", NULL, 0,
			&argc, argv, NULL, NULL);

		button = XmCreatePushButton(top_wid, "Push_me", NULL, 0);

		/* tell Xt to manage button * /
					XtManageChild(button);

					/* attach fn to widget * /
		XtAddCallback(button, XmNactivateCallback, pushed_fn, NULL);

		XtRealizeWidget(top_wid); /* display widget hierarchy * /
		XtAppMainLoop(app); /* enter processing loop * /
	*/
		ConsoleOutput( msg, file, line, true );
		return EResult::Break;
	}
//-----------------------------------------------------------------------------



// FreeBSD
#elif 0	// TODO

/*
=================================================
	Info
=================================================
*/
	Logger::EResult  AE::STL::Logger::Info (StringView msg, StringView func, StringView file, int line)
	{
		ConsoleOutput( msg, file, line, false );
		return EResult::Continue;
	}
	
/*
=================================================
	Error
=================================================
*/
	Logger::EResult  AE::STL::Logger::Error (StringView msg, StringView func, StringView file, int line)
	{
		ConsoleOutput( msg, file, line, true );
		return EResult::Abort;
	}
//-----------------------------------------------------------------------------


#else

#	error unsupported platform

#endif
