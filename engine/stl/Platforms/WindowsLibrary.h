// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef PLATFORM_WINDOWS

# include "stl/Platforms/WindowsHeader.h"
# include "stl/Platforms/WindowsUtils.h"
# include "stl/Types/Noncopyable.h"
# include "stl/Containers/NtStringView.h"
# include "stl/Types/FileSystem.h"
# include "stl/Algorithms/ArrayUtils.h"

namespace AE::STL
{

	//
	// Dynamic Library
	//

	class WindowsLibrary final : public Noncopyable
	{
	// variables
	private:
		HMODULE		_handle	= null;


	// methods
	public:
		WindowsLibrary ()		{}
		~WindowsLibrary ()		{ Unload(); }

		bool  Load (HMODULE lib);
		bool  Load (NtStringView libName);
		bool  Load (const char *libName)		{ return Load( NtStringView{libName} ); }
		bool  Load (const String &libName)		{ return Load( NtStringView{libName} ); }
		bool  Load (const Path &libName);
		void  Unload ();

		template <typename T>
		bool  GetProcAddr (NtStringView name, OUT T &result) const;

		ND_ Path  GetPath () const;

		ND_ explicit operator bool ()	const	{ return _handle != null; }
	};


	
	inline bool  WindowsLibrary::Load (HMODULE lib)
	{
		CHECK_ERR( _handle == null and lib != null );
		_handle = lib;
		return true;
	}

	inline bool  WindowsLibrary::Load (NtStringView libName)
	{
		CHECK_ERR( _handle == null );
		_handle = ::LoadLibraryA( libName.c_str() );
		
		if ( not _handle )
			WindowsUtils::CheckErrors( __FILE__, __LINE__ );

		return _handle != null;
	}

	inline bool  WindowsLibrary::Load (const Path &libName)
	{
		CHECK_ERR( _handle == null );
		_handle = ::LoadLibraryW( libName.c_str() );
		
		if ( not _handle )
			WindowsUtils::CheckErrors( __FILE__, __LINE__ );

		return _handle != null;
	}

	inline void  WindowsLibrary::Unload ()
	{
		if ( _handle ) {
			::FreeLibrary( _handle );
			_handle = null;
		}
	}
	
	template <typename T>
	inline bool  WindowsLibrary::GetProcAddr (NtStringView name, OUT T &result) const
	{
		result = BitCast<T>( ::GetProcAddress( _handle, name.c_str() ));
		return result != null;
	}
	
	inline Path  WindowsLibrary::GetPath () const
	{
		CHECK_ERR( _handle );

		wchar_t	buf[MAX_PATH] = {};
		CHECK_ERR( ::GetModuleFileNameW( _handle, buf, DWORD(CountOf(buf)) ) != FALSE );

		return Path{ buf };
	}


}	// AE::STL

#endif	//	PLATFORM_WINDOWS
