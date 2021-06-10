// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#if defined(PLATFORM_LINUX) || defined(PLATFORM_ANDROID)

# include "stl/Utils/Noncopyable.h"
# include "stl/Algorithms/ArrayUtils.h"
# include "stl/Containers/NtStringView.h"
# include "stl/Utils/FileSystem.h"

# include <dlfcn.h>
# include <linux/limits.h>

namespace AE::STL
{

	//
	// Dynamic Library
	//

	class PosixLibrary final : public Noncopyable
	{
	// variables
	private:
		void *		_handle	= null;


	// methods
	public:
		PosixLibrary ()		{}
		~PosixLibrary ()	{ Unload(); }

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

	

	inline bool  PosixLibrary::Load (NtStringView libName)
	{
		CHECK_ERR( _handle == null );
		_handle = ::dlopen( libName.c_str(), RTLD_LAZY | RTLD_LOCAL );
		return _handle != null;
	}

	inline bool  PosixLibrary::Load (const Path &libName)
	{
		CHECK_ERR( _handle == null );
		_handle = ::dlopen( libName.c_str(), RTLD_LAZY | RTLD_LOCAL );
		return _handle != null;
	}

	inline void  PosixLibrary::Unload ()
	{
		if ( _handle ) {
			::dlclose( _handle );
			_handle = null;
		}
	}
	
	template <typename T>
	inline bool  PosixLibrary::GetProcAddr (NtStringView name, OUT T &result) const
	{
		result = BitCast<T>( ::dlsym( _handle, name.c_str() ));
		return result != null;
	}
	
	inline Path  PosixLibrary::GetPath () const
	{
#	ifdef PLATFORM_ANDROID
		RETURN_ERR( "not supported" );
#	else

		CHECK_ERR( _handle );

		char	buf [PATH_MAX] = {};
		CHECK_ERR( ::dlinfo( _handle, RTLD_DI_ORIGIN, buf ) == 0 );

		return Path{ buf };
#	endif
	}


}	// AE::STL

#endif	// PLATFORM_LINUX or PLATFORM_ANDROID
