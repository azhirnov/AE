// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#if defined(PLATFORM_LINUX) || defined(PLATFORM_ANDROID)

# include "stl/Types/Noncopyable.h"
# include "stl/Algorithms/ArrayUtils.h"
# include "stl/Containers/NtStringView.h"
# include "stl/Types/FileSystem.h"

# include <dlfcn.h>
# include <linux/limits.h>

namespace AE::STL
{

	//
	// Dynamic Library
	//

	class Library final : public Noncopyable
	{
	// variables
	private:
		void *		_handle	= null;


	// methods
	public:
		Library ()		{}
		~Library ()		{ Unload(); }

		bool  Load (NtStringView libName);
		bool  Load (const Path &libName);
		void  Unload ();

		template <typename T>
		bool  GetProcAddr (NtStringView name, OUT T &result) const;
		
		ND_ Path  GetPath () const;

		ND_ explicit operator bool ()	const	{ return _handle != null; }
	};

	

	inline bool  Library::Load (NtStringView libName)
	{
		CHECK_ERR( _handle == null );
		_handle = ::dlopen( libName.c_str(), RTLD_NOW | RTLD_LOCAL );
		return _handle != null;
	}

	inline bool  Library::Load (const Path &libName)
	{
		CHECK_ERR( _handle == null );
		_handle = ::dlopen( libName.c_str(), RTLD_NOW | RTLD_LOCAL );	// TODO
		return _handle != null;
	}

	inline void  Library::Unload ()
	{
		if ( _handle ) {
			::dlclose( _handle );
			_handle = null;
		}
	}
	
	template <typename T>
	inline bool  Library::GetProcAddr (NtStringView name, OUT T &result) const
	{
		result = BitCast<T>( ::dlsym( _handle, name.c_str() ));
		return result != null;
	}
	
	inline Path  Library::GetPath () const
	{
#	ifndef PLATFORM_ANDROID
		CHECK_ERR( _handle );

		char	buf [PATH_MAX] = {};
		CHECK_ERR( ::dlinfo( _handle, RTLD_DI_ORIGIN, buf ) == 0 );

		return Path{ buf };
#	else

		RETURN_ERR( "not supported" );
#	endif
	}


}	// AE::STL

#endif	// PLATFORM_LINUX or PLATFORM_ANDROID
