// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'
/*
	Wrapper for std::filesystem that disable all exceptions except std::bad_alloc
*/

#pragma once

#include "stl/Common.h"

#ifdef AE_ENABLE_GFS
#  include "filesystem.hpp"
	namespace _ae_fs_ = ghc::filesystem;
#else
#  include <filesystem>
	namespace _ae_fs_ = std::filesystem;
#endif

namespace AE::STL
{
	using Path = _ae_fs_::path;


	//
	// File System
	//

	class FileSystem final
	{
	private:
		FileSystem () = delete;
		~FileSystem () = delete;


	// filesystem
	public:
		// remove file or empty directory
		static bool  Remove (const Path &p);

		// remove directory and all subdirectories
		static std::uintmax_t  RemoveAll (const Path &p);

		// create directory, parent directory must be exists
		static bool  CreateDirectory (const Path &p);

		// create all directories that is not exists
		static bool  CreateDirectories (const Path &p);

		// set working directory
		static bool  SetCurrentPath (const Path &p);

		// returns 'true' if file or directory is exists
		ND_ static bool  Exists (const Path &p);

		// returns 'true' if path refers to a directory
		ND_ static bool  IsDirectory (const Path &p);

		// returns current path
		ND_ static Path  CurrentPath ();

		// returns absolute path
		ND_ static Path  ToAbsolute (const Path &p);

		// returns relative path
		ND_ static Path  ToRelative (const Path &p, const Path &base);

		// enumerate all files in directory
		ND_ static _ae_fs_::directory_iterator  Enum (const Path &p);

		
	// utils
	public:
		// searches for a directory for which 'Exists( ref )' returns 'true'
		static bool  FindAndSetCurrent (const Path &ref, uint depth);
		static bool  FindAndSetCurrent (const Path &base, const Path &ref, uint depth);

		static bool  SearchBackward (const Path &ref, uint depth, OUT Path &result);
		static bool  SearchBackward (const Path &base, const Path &ref, uint depth, OUT Path &result);

		static bool  SearchForward (const Path &ref, uint depth, OUT Path &result);
		static bool  SearchForward (const Path &base, const Path &ref, uint depth, OUT Path &result);

		static bool  Search (const Path &ref, uint backwardDepth, uint forwardDepth, OUT Path &result);
		static bool  Search (const Path &base, const Path &ref, uint backwardDepth, uint forwardDepth, OUT Path &result);
	};

	

	inline bool  FileSystem::Remove (const Path &p)
	{
		std::error_code	ec;
		return _ae_fs_::remove( p, OUT ec );
	}
	
	inline std::uintmax_t  FileSystem::RemoveAll (const Path &p)
	{
		std::error_code	ec;
		return _ae_fs_::remove_all( p, OUT ec );
	}
	
	inline bool  FileSystem::CreateDirectory (const Path &p)
	{
		std::error_code	ec;
		return _ae_fs_::create_directory( p, OUT ec );
	}
	
	inline bool  FileSystem::CreateDirectories (const Path &p)
	{
		std::error_code	ec;
		return _ae_fs_::create_directories( p, OUT ec );
	}
	
	inline bool  FileSystem::SetCurrentPath (const Path &p)
	{
		std::error_code	ec;
		_ae_fs_::current_path( p, OUT ec );
		return ec == Default;
	}
		
	inline bool  FileSystem::Exists (const Path &p)
	{
		std::error_code	ec;
		return _ae_fs_::exists( p, OUT ec );
	}
	
	inline bool  FileSystem::IsDirectory (const Path &p)
	{
		std::error_code	ec;
		return _ae_fs_::is_directory( p, OUT ec );
	}

	inline Path  FileSystem::CurrentPath ()
	{
		std::error_code	ec;
		return _ae_fs_::current_path( OUT ec );
	}
	
	inline Path  FileSystem::ToAbsolute (const Path &p)
	{
		std::error_code	ec;
		return _ae_fs_::absolute( p, OUT ec );
	}
	
	inline Path  FileSystem::ToRelative (const Path &p, const Path &base)
	{
		std::error_code	ec;
		return _ae_fs_::relative( p, base, OUT ec );
	}
	
	inline _ae_fs_::directory_iterator  FileSystem::Enum (const Path &p)
	{
		std::error_code	ec;
		return _ae_fs_::directory_iterator{ p, OUT ec };
	}


}	// AE::STL
