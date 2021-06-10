// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Stream/Stream.h"
#include "stl/Containers/NtStringView.h"
#include "stl/Utils/FileSystem.h"

namespace AE::STL
{

	//
	// Read-only File Stream
	//

	class FileRStream final : public RStream
	{
	// variables
	private:
		FILE*		_file	= null;
		Bytes		_fileSize;
		Bytes		_position;


	// methods
	public:
		FileRStream (NtStringView filename);
		FileRStream (const char *filename);
		FileRStream (const String &filename);
		
	#ifdef PLATFORM_WINDOWS
		FileRStream (NtWStringView filename);
		FileRStream (const wchar_t *filename);
		FileRStream (const WString &filename);
	#endif
		
		FileRStream () {}
		FileRStream (const Path &path);
		~FileRStream ();

		bool	IsOpen ()	const override		{ return _file != null; }
		Bytes	Position ()	const override		{ return _position; }
		Bytes	Size ()		const override		{ return _fileSize; }
		
		bool	SeekSet (Bytes pos) override;
		Bytes	Read2 (OUT void *buffer, Bytes size) override;

	private:
		ND_ Bytes  _GetSize () const;
	};



	//
	// Write-only File Stream
	//

	class FileWStream final : public WStream
	{
	// variables
	private:
		FILE*		_file	= null;


	// methods
	public:
		FileWStream (NtStringView filename);
		FileWStream (const char *filename);
		FileWStream (const String &filename);

	#ifdef PLATFORM_WINDOWS
		FileWStream (NtWStringView filename);
		FileWStream (const wchar_t *filename);
		FileWStream (const WString &filename);
	#endif

		FileWStream () {}
		FileWStream (const Path &path);
		~FileWStream ();
		
		bool	IsOpen ()	const override		{ return _file != null; }
		Bytes	Position ()	const override;
		Bytes	Size ()		const override;
		
		bool	SeekSet (Bytes pos) override;
		Bytes	Write2 (const void *buffer, Bytes size) override;
		void	Flush () override;
	};

}	// AE::STL


// check definitions
#ifdef AE_CPP_DETECT_MISSMATCH

# ifdef _FILE_OFFSET_BITS
#  if _FILE_OFFSET_BITS == 64
#	pragma detect_mismatch( "_FILE_OFFSET_BITS", "64" )
#  else
#	pragma detect_mismatch( "_FILE_OFFSET_BITS", "32" )
#  endif
# endif

#endif	// AE_CPP_DETECT_MISSMATCH
