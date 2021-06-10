// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Stream/FileStream.h"
#include "stl/Algorithms/StringUtils.h"
#include <stdio.h>

#ifdef PLATFORM_WINDOWS
#	define fread	_fread_nolock
#	define fwrite	_fwrite_nolock
#	define fflush	_fflush_nolock
#	define fclose	_fclose_nolock
#	define ftell	_ftelli64_nolock
#	define fseek	_fseeki64_nolock
#endif

#if defined(PLATFORM_LINUX)
#	define fseek	fseeko
#	define ftell	ftello
#endif
#if defined(PLATFORM_LINUX) or defined(PLATFORM_ANDROID)
#   define fopen_s( _outFile_, _name_, _mode_ ) (*_outFile_=fopen(_name_, _mode_))
#endif


namespace AE::STL
{

/*
=================================================
	constructor
=================================================
*/
	FileRStream::FileRStream (NtStringView filename)
	{
		fopen_s( OUT &_file, filename.c_str(), "rb" );
		
		if ( _file )
			_fileSize = _GetSize();
		else
			AE_LOGI( "Can't open file: \""s << StringView{filename} << '"' );
	}
	
	FileRStream::FileRStream (const char *filename) : FileRStream{ NtStringView{filename} }
	{}

	FileRStream::FileRStream (const String &filename) : FileRStream{ NtStringView{filename} }
	{}
	
/*
=================================================
	constructor
=================================================
*/
#ifdef PLATFORM_WINDOWS
	FileRStream::FileRStream (NtWStringView filename)
	{
		_wfopen_s( OUT &_file, filename.c_str(), L"rb" );
		
		if ( _file )
			_fileSize = _GetSize();
	}
	
	FileRStream::FileRStream (const wchar_t *filename) : FileRStream{ NtWStringView{filename} }
	{}

	FileRStream::FileRStream (const WString &filename) : FileRStream{ NtWStringView{filename} }
	{}
#endif

/*
=================================================
	constructor
=================================================
*/
	FileRStream::FileRStream (const Path &path)
	{
#	ifdef PLATFORM_WINDOWS
		_wfopen_s( OUT &_file, path.c_str(), L"rb" );
#	else
		fopen_s( OUT &_file, path.c_str(), "rb" );
#	endif

		if ( _file )
			_fileSize = _GetSize();
		else
			AE_LOGI( "Can't open file: \""s << path.string() << '"' );
	}
	
/*
=================================================
	destructor
=================================================
*/
	FileRStream::~FileRStream ()
	{
		if ( _file )
			fclose( _file );
	}
	
/*
=================================================
	_GetSize
=================================================
*/
	Bytes  FileRStream::_GetSize () const
	{
		ASSERT( IsOpen() );

		const slong	curr = ftell( _file );
		CHECK( fseek( _file, 0, SEEK_END ) == 0 );

		const slong	size = ftell( _file );
		CHECK( fseek( _file, curr, SEEK_SET ) == 0 );

		return Bytes(ulong(size));
	}
	
/*
=================================================
	SeekSet
=================================================
*/
	bool  FileRStream::SeekSet (Bytes pos)
	{
		ASSERT( IsOpen() );

		if ( pos == _position )
			return true;

		_position = Min( pos, _fileSize );

		return (fseek( _file, slong(pos), SEEK_SET ) == 0);
	}
	
/*
=================================================
	Read2
=================================================
*/
	Bytes  FileRStream::Read2 (OUT void *buffer, Bytes size)
	{
		ASSERT( IsOpen() );

		Bytes	readn{ fread( buffer, 1, usize(size), _file )};

		_position += readn;

		return readn;
	}
//-----------------------------------------------------------------------------


	
/*
=================================================
	constructor
=================================================
*/
	FileWStream::FileWStream (NtStringView filename)
	{
		fopen_s( OUT &_file, filename.c_str(), "wb" );

		if ( not _file )
			AE_LOGI( "Can't open file: \""s << StringView{filename} << '"' );
	}
	
	FileWStream::FileWStream (const char *filename) : FileWStream{ NtStringView{filename} }
	{}

	FileWStream::FileWStream (const String &filename) : FileWStream{ NtStringView{filename} }
	{}
	
/*
=================================================
	constructor
=================================================
*/
#ifdef PLATFORM_WINDOWS
	FileWStream::FileWStream (NtWStringView filename)
	{
		_wfopen_s( OUT &_file, filename.c_str(), L"wb" );
	}
	
	FileWStream::FileWStream (const wchar_t *filename) : FileWStream{ NtWStringView{filename} }
	{}

	FileWStream::FileWStream (const WString &filename) : FileWStream{ NtWStringView{filename} }
	{}
#endif

/*
=================================================
	constructor
=================================================
*/
	FileWStream::FileWStream (const Path &path)
	{
#	ifdef PLATFORM_WINDOWS
		_wfopen_s( OUT &_file, path.c_str(), L"wb" );
#	else
		fopen_s( OUT &_file, path.c_str(), "wb" );
#	endif
		
		if ( not _file )
			AE_LOGI( "Can't open file: \""s << path.string() << '"' );
	}
	
/*
=================================================
	destructor
=================================================
*/
	FileWStream::~FileWStream ()
	{
		if ( _file )
			fclose( _file );
	}
	
/*
=================================================
	Position
=================================================
*/
	Bytes  FileWStream::Position () const
	{
		ASSERT( IsOpen() );

		return Bytes(ulong(ftell( _file )));
	}
	
/*
=================================================
	Size
=================================================
*/
	Bytes  FileWStream::Size () const
	{
		ASSERT( IsOpen() );

		const slong	curr = ftell( _file );
		CHECK( fseek( _file, 0, SEEK_END ) == 0 );

		const slong	size = ftell( _file );
		CHECK( fseek( _file, curr, SEEK_SET ) == 0 );

		return Bytes(ulong(size));
	}
	
/*
=================================================
	SeekSet
=================================================
*/
	bool  FileWStream::SeekSet (Bytes pos)
	{
		ASSERT( IsOpen() );

		return (fseek( _file, slong(pos), SEEK_SET ) == 0);
	}

/*
=================================================
	Write2
=================================================
*/
	Bytes  FileWStream::Write2 (const void *buffer, Bytes size)
	{
		ASSERT( IsOpen() );

		return Bytes(fwrite( buffer, 1, usize(size), _file ));
	}
	
/*
=================================================
	Flush
=================================================
*/
	void  FileWStream::Flush ()
	{
		ASSERT( IsOpen() );

		CHECK( fflush( _file ) == 0 );
	}


}	// AE::STL
