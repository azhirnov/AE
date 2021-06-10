// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

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
		String			_androidDevice;


	// methods
	public:
		BuildScriptApi () {}
		BuildScriptApi (String& output, Mutex& guard, Atomic<bool>& looping, Path workDir, Path deployDir);

		static bool  Bind (const ScriptEnginePtr &se);

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

		// android
		bool  AndroidBuild (const String &sourceDir, const ScriptArray<String> &defines);	// TODO: sign
		bool  AndroidDevices (OUT ScriptArray<String> &devices);	// for USB
		bool  AndroidSetDevice (const String &device);				// for USB
		bool  AndroidConnectDevice (const String &ip);				// for WiFi
		bool  AndroidRun (const String &sourceDir);
		bool  AndroidCopyTo (const String &src, const String &dst);
		//bool  AndroidCopyFrom (const String &src, const String &dst);

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
						  milliseconds timeout = milliseconds{6'000'000})
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
	Bind
=================================================
*/
	bool  BuildScriptApi::Bind (const ScriptEnginePtr &se)
	{
		CoreBindings::BindString( se );
		CoreBindings::BindArray( se );
		CoreBindings::BindLog( se );

		bool	binded = true;

		// bind ECompiler
		{
			EnumBinder<ECompiler>	binder{ se };
			binded &= binder.Create();
			binded &= binder.AddValue( "VisualStudio2017",		ECompiler::VisualStudio2017 );
			binded &= binder.AddValue( "VisualStudio2019",		ECompiler::VisualStudio2019 );
			binded &= binder.AddValue( "VisualStudio2019_v141",	ECompiler::VisualStudio2019_v141 );
		}

		// bind EArch
		{
			EnumBinder<EArch>	binder{ se };
			binded &= binder.Create();
			binded &= binder.AddValue( "x86",	EArch::x86 );
			binded &= binder.AddValue( "x64",	EArch::x64 );
		}

		// bind BuildScriptApi
		{
			ClassBinder<BuildScriptApi>	binder{ se };
			binded &= binder.CreateRef( &AngelScriptHelper::FactoryCreate<BuildScriptApi>, null, null, 0 );

			binded &= binder.AddMethod( &BuildScriptApi::GitClone,			"GitClone" );
			binded &= binder.AddMethod( &BuildScriptApi::GitClone2,			"GitClone" );
			binded &= binder.AddMethod( &BuildScriptApi::GitGetBranch,		"GitGetBranch" );
			binded &= binder.AddMethod( &BuildScriptApi::GitGetHash,		"GitGetHash" );

			binded &= binder.AddMethod( &BuildScriptApi::CMakeGen,			"CMakeGen" );
			binded &= binder.AddMethod( &BuildScriptApi::CMakeBuild,		"CMakeBuild" );
			binded &= binder.AddMethod( &BuildScriptApi::CMakeInstall,		"CMakeInstall" );
			binded &= binder.AddMethod( &BuildScriptApi::CTest,				"CTest" );

			binded &= binder.AddMethod( &BuildScriptApi::AndroidBuild,		"AndroidBuild" );
			binded &= binder.AddMethod( &BuildScriptApi::AndroidDevices,	"AndroidDevices" );
			binded &= binder.AddMethod( &BuildScriptApi::AndroidSetDevice,	"AndroidSetDevice" );
			binded &= binder.AddMethod( &BuildScriptApi::AndroidConnectDevice,	"AndroidConnectDevice" );
			binded &= binder.AddMethod( &BuildScriptApi::AndroidRun,		"AndroidRun" );
			binded &= binder.AddMethod( &BuildScriptApi::AndroidCopyTo,		"AndroidCopyTo" );

			binded &= binder.AddMethod( &BuildScriptApi::CurDir,			"CurDir" );
			binded &= binder.AddMethod( &BuildScriptApi::MakeDir,			"MakeDir" );
			binded &= binder.AddMethod( &BuildScriptApi::IsFile,			"IsFile" );
			binded &= binder.AddMethod( &BuildScriptApi::IsDirectory,		"IsDirectory" );

			binded &= binder.AddMethod( &BuildScriptApi::Deploy,			"Deploy" );
		}

		return binded;
	}

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
			usize	pos = output.find( "tests failed out of" );
			if ( pos != String::npos )
			{
				usize	start	= pos;
				usize	end		= pos;
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
	AndroidBuild
=================================================
*/
	bool  BuildScriptApi::AndroidBuild (const String &sourceDir, const ScriptArray<String> &defines)
	{
		if ( not _looping->load( EMemoryOrder::Relaxed ))
			return false;
		
		Path	src_folder	 = Path{_currentDir}.append( sourceDir );
		Path	src_relative = FileSystem::ToRelative( src_folder, _workDir );

		if ( src_relative.string().find( ".." ) != String::npos )
		{
			EXLOCK( *_outputGuard );
			*_output << "AndroidBuild: invalid source path\n";
			_errorCounter ++;
			return false;
		}

		// append cmake defines to 'build.gradle'
		if ( defines.size() )
		{
			Array<Path>		subdirs;
			Array<Path>		files;

			for (auto& entry : FileSystem::Enum( src_folder ))
			{
				if ( entry.is_directory() )
					subdirs.push_back( entry );
				else
				if ( Path{entry}.filename().string() == "build.gradle" )
					files.push_back( entry );
			}

			for (auto& dir : subdirs)
			{
				for (auto& entry : FileSystem::Enum( dir ))
				{
					if ( Path{entry}.filename().string() == "build.gradle" )
						files.push_back( entry );
				}
			}
			
			String	cmake_args;
			for (auto& def : defines) {
				cmake_args << "\n\t\t\t\t\t\t  '-D" << def << "',";		// TODO: convert " to \", validate
			}
			cmake_args << "\n\t\t\t\t\t\t ";

			const String	externalNativeBuild_str = "externalNativeBuild";
			const String	arguments_str			= "arguments";

			// TODO: backup
			for (auto& fname : files)
			{
				FileRStream		file{ fname };
				
				if ( not file.IsOpen() )
				{
					EXLOCK( *_outputGuard );
					*_output << "AndroidBuild: failed to open file: '" << fname.string() << "'\n";
					_errorCounter ++;
					continue;
				}

				String	src;
				if ( not file.Read( usize(file.RemainingSize()), OUT src ))
					continue;

				usize	pos = src.find( externalNativeBuild_str );
				if ( pos == String::npos )
					continue;

				const usize		begin = pos + externalNativeBuild_str.length();

				pos = src.find( '{', begin );
				if ( pos == String::npos )
					continue;

				++pos;
				for (int cnt = 1; (cnt > 0) and (pos < src.length()); ++pos)
				{
					const char	c = src[pos];
					if ( c == '{' )
						++cnt;
					if ( c == '}' )
						--cnt;
				}
				const usize		end = pos;

				pos = src.find( arguments_str, begin );
				if ( pos == String::npos or pos > end )
				{
					// TODO: add field
					continue;
				}

				// append args
				pos += arguments_str.length() + 1;
				src.insert( pos, cmake_args );
			}
		}
		
		String	cmd;
		cmd << "gradlew build";

		String	output;
		bool	res = Execute( cmd, INOUT &output, null, *_looping, src_folder );
		ASSERT( res );

		if ( res )
		{
			const String	success_str	= "BUILD SUCCESSFUL";
			const String	fail_str	= "BUILD FAILED";

			uint	success_count	= 0;
			uint	fail_count		= 0;

			for (usize pos = 0;;)
			{
				pos = output.find( success_str, pos );
				if ( pos == String::npos )
					break;

				++success_count;
				pos += success_str.length();
			}
			
			for (usize pos = 0;;)
			{
				pos = output.find( fail_str, pos );
				if ( pos == String::npos )
					break;

				++fail_count;
				pos += fail_str.length();
			}

			ASSERT( success_count > 0 or fail_count > 0 );

			if ( not (success_count > 0 and fail_count == 0) )
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
	AndroidDevices
----
	https://developer.android.com/studio/command-line
=================================================
*/
	bool  BuildScriptApi::AndroidDevices (OUT ScriptArray<String> &devices)
	{
		devices.clear();

		String	cmdline = "adb devices -l";
		String	list;
		bool	res = WindowsProcess::Execute( cmdline, OUT list );

		if ( res )
		{
			usize	pos = list.find( "List of devices attached" );

			for (; pos < list.length(); ++pos)
			{

			}
		}
		
		_errorCounter += uint(res);
		return res;
	}
	
/*
=================================================
	AndroidSetDevice
=================================================
*/
	bool  BuildScriptApi::AndroidSetDevice (const String &device)
	{
		_androidDevice = device;
		return true;
	}
	
/*
=================================================
	AndroidSetDevice
=================================================
*/
	bool  BuildScriptApi::AndroidConnectDevice (const String &ip)
	{
		_androidDevice = ip;
		
		String	cmdline	= "adb connect "s << _androidDevice;
		bool	res		= WindowsProcess::Execute( cmdline );
		Unused( res );
		return true;
	}

/*
=================================================
	AndroidRun
=================================================
*/
	bool  BuildScriptApi::AndroidRun (const String &sourceDir)
	{
		Unused( sourceDir );
		return true;
	}
	
/*
=================================================
	AndroidCopyTo
=================================================
*/
	bool  BuildScriptApi::AndroidCopyTo (const String &src, const String &dst)
	{
		Unused( src, dst );
		return true;
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
