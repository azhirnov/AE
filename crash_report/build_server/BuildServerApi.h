// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Containers/NtStringView.h"
#include "stl/Stream/BrotliStream.h"
#include "stl/Stream/FileStream.h"
#include "stl/Algorithms/ArrayUtils.h"
#include "stl/Algorithms/StringUtils.h"
#include "stl/Algorithms/StringParser.h"
#include "stl/Utils/FileSystem.h"
#include "stl/Platforms/PlatformUtils.h"
#include "stl/Math/Random.h"

#include "threading/Common.h"

#include "scripting/Bindings/CoreBindings.h"
#include "scripting/Impl/ClassBinder.h"
#include "scripting/Impl/EnumBinder.h"
#include "scripting/Impl/ScriptFn.h"

namespace
{
	using namespace AE::STL;
	using namespace AE::Threading;
	using namespace AE::Scripting;
	
	using TimePoint_t = std::chrono::high_resolution_clock::time_point;


	
	//
	// Build Script Api
	//

	class BuildScriptApi
	{
	// types
	public:
		enum class ECompiler : uint
		{
			VisualStudio2019,
			VisualStudio2019_v141,
			VisualStudio2017,
		};

		enum class EArch : uint
		{
			x86,
			x64,
			//arm,
			//arm64
		};


	// variables
	private:
		uint			_errorCounter	= 0;
		String *		_output			= null;
		Mutex *			_outputGuard	= null;
		Atomic<bool> *	_looping		= null;
		Path			_workDir;
		Path			_currentDir;
		Path			_deployDir;


	// methods
	public:
		BuildScriptApi () {}
		BuildScriptApi (String& output, Mutex& guard, Atomic<bool>& looping, Path workDir, Path deployDir);

		// git
		bool  GitClone (const String &url, const String &branch, const String &dstFolder);
		bool  GitClone2 (const String &url, const String &dstFolder);
		String  GitGetBranch (const String &path);
		String  GitGetHash (const String &path, bool isShort);

		// cmake
		bool  CMakeGen (ECompiler comp, EArch arch, const String &sourceDir, const String &buildDir, const ScriptArray<String> &defines);
		bool  CMakeBuild (const String &buildDir, const String &config, const String &target);
		bool  CMakeInstall (const String &buildDir, const String &dstFolder, const String &config, const String &target);
		bool  CTest (const String &exeDir, const String &config);

		// filesystem
		bool  CurDir (const String &dir);
		bool  MakeDir (const String &dir);
		bool  IsFile (const String &path);
		bool  IsDirectory (const String &path);

		// build server
		bool  Deploy (const String &version, const String &distFolder, const String &dbgSymFolder);

		bool  HasErrors () const	{ return _errorCounter > 0; }
	};

}	// namespace

	
AE_DECL_SCRIPT_OBJ( BuildScriptApi,				"BuildScriptApi" );
AE_DECL_SCRIPT_TYPE( BuildScriptApi::ECompiler,	"ECompiler" );
AE_DECL_SCRIPT_TYPE( BuildScriptApi::EArch,		"EArch" );


namespace
{
	
/*
=================================================
	Execute
=================================================
*/
	static bool  Execute (String commandLine, INOUT String *output, Mutex *guard, Atomic<bool> &looping, const Path &currentDir,
						  milliseconds timeout = milliseconds{6000'000})
	{
		if ( guard and output )
		{
			EXLOCK( *guard );
			*output << "\n-------------------------------------------------------------------------------\n\n";
			*output << commandLine << '\n';
		}

		const TimePoint_t	start = TimePoint_t::clock::now();

		WindowsProcess	proc;
		CHECK_ERR( proc.ExecuteAsync( commandLine, currentDir, WindowsProcess::EFlags::ReadOutput ));

		for (;;)
		{
			if ( not proc.IsActive() )
			{
				if ( output )
					return proc.WaitAndClose( OUT *output, guard, timeout );
				else
					return proc.WaitAndClose( timeout );
			}
			
			auto	dt = std::chrono::duration_cast<milliseconds>( TimePoint_t::clock::now() - start );

			if ( not looping.load( EMemoryOrder::Relaxed ) or
				 dt > timeout )
			{
				proc.Terminate();
				return false;
			}

			std::this_thread::sleep_for( milliseconds{20} );
		}
	}

/*
=================================================
	PathToString
=================================================
*/
	static String  PathToString (const Path &p)
	{
		String	s = p.string();
		if ( s.size() and s.back() == '\\' )
			s.pop_back();
		return '"' + s + '"';
	}
//-----------------------------------------------------------------------------


	
/*
=================================================
	constructor
=================================================
*/
	BuildScriptApi::BuildScriptApi (String& output, Mutex& guard, Atomic<bool>& looping, Path workDir, Path deployDir) :
		_output{ &output },
		_outputGuard{ &guard },
		_looping{ &looping },
		_workDir{ workDir },
		_currentDir{ workDir },
		_deployDir{ std::move(deployDir) }
	{}

/*
=================================================
	GitClone
----
	"git clone https://username:password@github.com/username/repository.git"
=================================================
*/
	bool  BuildScriptApi::GitClone (const String &url, const String &branch, const String &dstFolder)
	{
		if ( not _looping->load( EMemoryOrder::Relaxed ))
			return false;
		
		Path	dst_folder	= Path{_currentDir}.append( dstFolder );
		Path	relative	= FileSystem::ToRelative( dst_folder, _workDir );

		if ( relative.string().find( ".." ) != String::npos )
		{
			EXLOCK( *_outputGuard );
			*_output << "GitClone: invalid path\n";
			_errorCounter ++;
			return false;
		}

		String	cmd;
		cmd << '"' << AE_GIT_EXE << "\" clone \"" << url << "\" "
			<< PathToString( dst_folder ) << " --branch \"" << branch << '"';

		bool	res = Execute( cmd, INOUT _output, _outputGuard, *_looping, _currentDir );
		ASSERT( res );
		_errorCounter += uint(res);
		return res;
	}
	
/*
=================================================
	GitClone2
=================================================
*/
	bool  BuildScriptApi::GitClone2 (const String &url, const String &dstFolder)
	{
		if ( not _looping->load( EMemoryOrder::Relaxed ))
			return false;
		
		Path	dst_folder	= Path{_currentDir}.append( dstFolder );
		Path	relative	= FileSystem::ToRelative( dst_folder, _workDir );

		if ( relative.string().find( ".." ) != String::npos )
		{
			EXLOCK( *_outputGuard );
			*_output << "GitClone: invalid path\n";
			_errorCounter ++;
			return false;
		}
		
		String	cmd;
		cmd << '"' << AE_GIT_EXE << "\" clone \"" << url << "\" " << PathToString( dst_folder );

		bool	res = Execute( cmd, INOUT _output, _outputGuard, *_looping, _currentDir );
		ASSERT( res );
		_errorCounter += uint(res);
		return res;
	}
	
/*
=================================================
	GitGetBranch
=================================================
*/
	String  BuildScriptApi::GitGetBranch (const String &path)
	{
		if ( not _looping->load( EMemoryOrder::Relaxed ))
			return "";
		
		Path	folder		= Path{_currentDir}.append( path );
		Path	relative	= FileSystem::ToRelative( folder, _workDir );

		if ( relative.string().find( ".." ) != String::npos )
		{
			EXLOCK( *_outputGuard );
			*_output << "GitGetBranch: invalid path\n";
			_errorCounter ++;
			return "";
		}

		String	result;
		String	cmd;

		cmd << '"' << AE_GIT_EXE << "\" -C " << PathToString( folder ) << " rev-parse --abbrev-ref HEAD";
		
		{
			EXLOCK( *_outputGuard );
			*_output << "\n-------------------------------------------------------------------------------\n\n" << cmd << '\n';
		}

		bool	res = WindowsProcess::Execute( cmd, OUT result, null );
		ASSERT( res );
		_errorCounter += uint(res);

		return result;
	}
		
/*
=================================================
	GitGetHash
=================================================
*/
	String  BuildScriptApi::GitGetHash (const String &path, bool isShort)
	{
		if ( not _looping->load( EMemoryOrder::Relaxed ))
			return "";
		
		Path	folder		= Path{_currentDir}.append( path );
		Path	relative	= FileSystem::ToRelative( folder, _workDir );

		if ( relative.string().find( ".." ) != String::npos )
		{
			EXLOCK( *_outputGuard );
			*_output << "GitGetHash: invalid path\n";
			_errorCounter ++;
			return "";
		}

		String	result;
		String	cmd;

		cmd << '"' << AE_GIT_EXE << "\" -C " << PathToString( folder ) << " rev-parse" << (isShort ? " --short" : " ") << " HEAD";
		
		{
			EXLOCK( *_outputGuard );
			*_output << "\n-------------------------------------------------------------------------------\n\n" << cmd << '\n';
		}

		bool	res = WindowsProcess::Execute( cmd, OUT result, null );
		ASSERT( res );
		_errorCounter += uint(res);

		return result;
	}

/*
=================================================
	CMakeGen
=================================================
*/
	bool  BuildScriptApi::CMakeGen (ECompiler comp, EArch arch, const String &sourceDir, const String &buildDir, const ScriptArray<String> &defines)
	{
		if ( not _looping->load( EMemoryOrder::Relaxed ))
			return false;
		
		Path	source_folder	= Path{_currentDir}.append( sourceDir );
		Path	source_relative = FileSystem::ToRelative( source_folder, _workDir );

		if ( source_relative.string().find( ".." ) != String::npos )
		{
			EXLOCK( *_outputGuard );
			*_output << "CMakeGen: invalid source path\n";
			_errorCounter ++;
			return false;
		}
		
		Path	build_folder	= Path{_currentDir}.append( buildDir );
		Path	build_relative = FileSystem::ToRelative( build_folder, _workDir );

		if ( build_relative.string().find( ".." ) != String::npos )
		{
			EXLOCK( *_outputGuard );
			*_output << "CMakeGen: invalid build path\n";
			_errorCounter ++;
			return false;
		}

		String	cmd;
		cmd << '"' << AE_CMAKE_EXE << '"';
		
		BEGIN_ENUM_CHECKS();
		switch ( comp )
		{
			case ECompiler::VisualStudio2017 :
			{
				cmd << " -G \"Visual Studio 15 2017 ";
				switch ( arch ) {
					case EArch::x64 :	cmd << " Win64";	break;
					case EArch::x86 :	break;		// default
				}
				cmd << "\"";
				break;
			}
			case ECompiler::VisualStudio2019_v141 :
			case ECompiler::VisualStudio2019 :
			{
				cmd << " -G \"Visual Studio 16 2019\"";
				if ( comp == ECompiler::VisualStudio2019_v141 )
					cmd << " -T v141";
				switch ( arch ) {
					case EArch::x64 :	break;	// default
					case EArch::x86 :	cmd << " -A Win32";	break;
				}
				break;
			}
		}
		END_ENUM_CHECKS();

		cmd << " -S " << PathToString( source_folder );
		cmd << " -B " << PathToString( build_folder );

		for (auto& def : defines) {
			cmd << " -D" << def;	// TODO: convert " to \", validate
		}

		bool	res = Execute( cmd, INOUT _output, _outputGuard, *_looping, _currentDir );
		ASSERT( res );
		_errorCounter += uint(res);
		return res;
	}
	
/*
=================================================
	CMakeBuild
=================================================
*/
	bool  BuildScriptApi::CMakeBuild (const String &buildDir, const String &config, const String &target)
	{
		if ( not _looping->load( EMemoryOrder::Relaxed ))
			return false;
		
		Path	build_folder	= Path{_currentDir}.append( buildDir );
		Path	build_relative = FileSystem::ToRelative( build_folder, _workDir );

		if ( build_relative.string().find( ".." ) != String::npos )
		{
			EXLOCK( *_outputGuard );
			*_output << "CMakeBuild: invalid build path\n";
			_errorCounter ++;
			return false;
		}

		String	cmd;
		cmd << '"' << AE_CMAKE_EXE << '"';
		cmd << " --build " << PathToString( build_folder );

		if ( config.size() )
			cmd << " --config \"" << config << '"';
	
		if ( target.size() )
			cmd << " --target \"" << target << '"';

		bool	res = Execute( cmd, INOUT _output, _outputGuard, *_looping, _currentDir );
		ASSERT( res );
		_errorCounter += uint(res);
		return res;
	}
	
/*
=================================================
	CMakeInstall
=================================================
*/
	bool  BuildScriptApi::CMakeInstall (const String &buildDir, const String &dstFolder, const String &config, const String &target)
	{
		if ( not _looping->load( EMemoryOrder::Relaxed ))
			return false;
		
		Path	build_folder	= Path{_currentDir}.append( buildDir );
		Path	build_relative = FileSystem::ToRelative( build_folder, _workDir );

		if ( build_relative.string().find( ".." ) != String::npos )
		{
			EXLOCK( *_outputGuard );
			*_output << "CMakeInstall: invalid build path\n";
			_errorCounter ++;
			return false;
		}

		Path	dst_folder	= Path{_currentDir}.append( dstFolder );
		Path	dst_relative = FileSystem::ToRelative( dst_folder, _workDir );

		if ( dst_relative.string().find( ".." ) != String::npos )
		{
			EXLOCK( *_outputGuard );
			*_output << "CMakeInstall: invalid install path\n";
			_errorCounter ++;
			return false;
		}

		String	cmd;
		cmd << '"' << AE_CMAKE_EXE << '"';
		cmd << " --install " << PathToString( build_folder );

		if ( dstFolder.size() )
			cmd << " --prefix " << PathToString( dst_folder );

		if ( config.size() )
			cmd << " --config \"" << config << '"';
	
		if ( target.size() )
			cmd << " --target \"" << target << '"';

		bool	res = Execute( cmd, INOUT _output, _outputGuard, *_looping, _currentDir );
		ASSERT( res );
		_errorCounter += uint(res);
		return res;
	}

/*
=================================================
	CTest
=================================================
*/
	bool  BuildScriptApi::CTest (const String &exeDir, const String &config)
	{
		if ( not _looping->load( EMemoryOrder::Relaxed ))
			return false;
		
		Path	exe_folder	 = Path{_currentDir}.append( exeDir );
		Path	exe_relative = FileSystem::ToRelative( exe_folder, _workDir );

		if ( exe_relative.string().find( ".." ) != String::npos )
		{
			EXLOCK( *_outputGuard );
			*_output << "CTest: invalid exe path\n";
			_errorCounter ++;
			return false;
		}

		String	cmd;
		cmd << PathToString( Path{AE_CMAKE_EXE}.replace_filename( "ctest.exe" ));

		if ( exeDir.size() )
			cmd << " --build-exe-dir " << PathToString( exe_folder );

		cmd << " -C " << config;
		cmd << " --verbose";

		String	output;
		bool	res = Execute( cmd, INOUT &output, null, *_looping, _currentDir );
		ASSERT( res );

		if ( res )
		{
			size_t	pos = output.find( "tests failed out of" );
			if ( pos != String::npos )
			{
				size_t	start	= pos;
				size_t	end		= pos;
				StringParser::ToBeginOfLine( output, INOUT start );
				StringParser::ToEndOfLine( output, INOUT end );

				StringView	str = StringView{output}.substr( start, end - start );

				if ( str.find( "100% tests passed" ) == StringView::npos )
				{
					res = false;
				}
			}
			else
				res = false;
		}

		{
			EXLOCK( *_outputGuard );
			*_output << output;
		}

		_errorCounter += uint(res);
		return res;
	}

/*
=================================================
	CurDir
=================================================
*/
	bool  BuildScriptApi::CurDir (const String &dir)
	{
		if ( not _looping->load( EMemoryOrder::Relaxed ))
			return false;
		
		Path	folder	 = Path{_currentDir}.append( dir );
		Path	relative = FileSystem::ToRelative( folder, _workDir );

		if ( relative.string().find( ".." ) != String::npos )
		{
			EXLOCK( *_outputGuard );
			*_output << "CurDir: invalid path\n";
			_errorCounter ++;
			return false;
		}
		
		{
			EXLOCK( *_outputGuard );
			*_output << "cd " << PathToString( folder ) << "\n";
		}
		_currentDir = std::move(folder);
		return true;
	}
	
/*
=================================================
	MakeDir
=================================================
*/
	bool  BuildScriptApi::MakeDir (const String &dir)
	{
		if ( not _looping->load( EMemoryOrder::Relaxed ))
			return false;

		Path	folder	 = Path{_currentDir}.append( dir );
		Path	relative = FileSystem::ToRelative( folder, _workDir );

		if ( relative.string().find( ".." ) != String::npos )
		{
			EXLOCK( *_outputGuard );
			*_output << "MakeDir: invalid path\n";
			_errorCounter ++;
			return false;
		}
		
		{
			EXLOCK( *_outputGuard );
			*_output << "mkdir " << PathToString( folder ) << "\n";
		}

		FileSystem::CreateDirectory( folder );
		return FileSystem::Exists( folder ) and FileSystem::IsDirectory( folder );
	}
	
/*
=================================================
	IsFile
=================================================
*/
	bool  BuildScriptApi::IsFile (const String &path)
	{
		if ( not _looping->load( EMemoryOrder::Relaxed ))
			return false;

		Path	path2	 = Path{_currentDir}.append( path );
		Path	relative = FileSystem::ToRelative( path2, _workDir );

		if ( relative.string().find( ".." ) != String::npos )
		{
			EXLOCK( *_outputGuard );
			*_output << "IsFile: invalid path\n";
			_errorCounter ++;
			return false;
		}

		return FileSystem::IsFile( path2 );
	}
	
/*
=================================================
	IsDirectory
=================================================
*/
	bool  BuildScriptApi::IsDirectory (const String &path)
	{
		if ( not _looping->load( EMemoryOrder::Relaxed ))
			return false;

		Path	folder	 = Path{_currentDir}.append( path );
		Path	relative = FileSystem::ToRelative( folder, _workDir );

		if ( relative.string().find( ".." ) != String::npos )
		{
			EXLOCK( *_outputGuard );
			*_output << "IsDirectory: invalid path\n";
			_errorCounter ++;
			return false;
		}

		return FileSystem::IsDirectory( folder );
	}
	
/*
=================================================
	Deploy
=================================================
*/
	bool  BuildScriptApi::Deploy (const String &version, const String &distFolder, const String &dbgSymFolder)
	{
		if ( not _looping->load( EMemoryOrder::Relaxed ))
			return false;

		Path	dist_folder	  = Path{_currentDir}.append( distFolder );
		Path	dist_relative = FileSystem::ToRelative( dist_folder, _workDir );

		if ( dist_relative.string().find( ".." ) != String::npos )
		{
			EXLOCK( *_outputGuard );
			*_output << "Deploy: invalid path for distributive\n";
			_errorCounter ++;
			return false;
		}

		Path	dbgsym_folder	= Path{_currentDir}.append( dbgSymFolder );
		Path	dbgsym_relative = FileSystem::ToRelative( dbgsym_folder, _workDir );

		if ( dbgsym_relative.string().find( ".." ) != String::npos )
		{
			EXLOCK( *_outputGuard );
			*_output << "Deploy: invalid path for debug symbols\n";
			_errorCounter ++;
			return false;
		}

		String	ver = version;
		FileSystem::ValidateFileName( INOUT ver );

		Path	ver_folder			= Path{_deployDir}.append( ver );
		Path	dst_dist_folder		= Path{ver_folder}.append( "dist" );
		Path	dst_dbgsym_folder	= Path{ver_folder}.append( "dbg_sym" );

		if ( FileSystem::Exists( ver_folder ))
		{
			EXLOCK( *_outputGuard );
			*_output << "Deploy: folder '" << ver_folder.string() << " already exists'\n";
			_errorCounter ++;
			return false;
		}

		if ( not FileSystem::CreateDirectory( ver_folder ))
		{
			EXLOCK( *_outputGuard );
			*_output << "Deploy: failed to create folder '" << ver_folder.string() << "'\n";
			_errorCounter ++;
			return false;
		}

		// copy distributive
		{
			if ( not FileSystem::IsDirectory( dist_folder ))
			{
				EXLOCK( *_outputGuard );
				*_output << "Deploy: distributive folder '" << dist_folder.string() << " dosn't exists\n";
				_errorCounter ++;
				return false;
			}

			if ( not FileSystem::CopyDirectory( dist_folder, dst_dist_folder ))
			{
				EXLOCK( *_outputGuard );
				*_output << "Deploy: failed to copy from '" << dist_folder.string() << " to '" << dst_dist_folder.string() << "'\n";
				_errorCounter ++;
				return false;
			}
		}

		// copy debug symbols
		if ( dbgSymFolder.size() )
		{
			if ( not FileSystem::IsDirectory( dbgsym_folder ))
			{
				EXLOCK( *_outputGuard );
				*_output << "Deploy: debug symbols folder '" << dbgsym_folder.string() << " dosn't exists\n";
				_errorCounter ++;
				return false;
			}
			
			if ( not FileSystem::CopyDirectory( dbgsym_folder, dst_dbgsym_folder ))
			{
				EXLOCK( *_outputGuard );
				*_output << "Deploy: failed to copy from '" << dbgsym_folder.string() << " to '" << dst_dbgsym_folder.string() << "'\n";
				_errorCounter ++;
				return false;
			}
		}

		return true;
	}

}	// namespace
