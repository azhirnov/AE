// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "SpirvCompiler.h"
#include "PrivateDefines.h"
#include "stl/Algorithms/StringUtils.h"
#include "stl/Algorithms/StringParser.h"
#include "stl/Stream/FileStream.h"
#include "graphics/Private/EnumUtils.h"

// glslang includes
#ifdef COMPILER_MSVC
#	pragma warning (push)
#	pragma warning (disable: 4005)
#endif
#ifdef COMPILER_CLANG
#	pragma clang diagnostic push
#	pragma clang diagnostic ignored "-Wdouble-promotion"
#endif

#include "glslang/build_info.h"
#include "glslang/Public/ShaderLang.h"
#include "glslang/MachineIndependent/localintermediate.h"
#include "glslang/Include/intermediate.h"
#include "glslang/SPIRV/doc.h"
#include "glslang/SPIRV/disassemble.h"
#include "glslang/SPIRV/GlslangToSpv.h"
#include "glslang/SPIRV/GLSL.std.450.h"

#ifdef COMPILER_MSVC
#	pragma warning (pop)
#endif
#ifdef COMPILER_CLANG
#	pragma clang diagnostic pop
#endif

// SPIRV-Tools includes
#ifdef ENABLE_OPT
#	include "spirv-tools/optimizer.hpp"
#	include "spirv-tools/libspirv.h"
#else
#	pragma message("SPIRV-Tolls library is missing, SPIRV optimization will be disabled")
#endif

#ifndef AE_ENABLE_GLSL_TRACE
#	pragma message("GLSL-Trace library is missing, shader debugging and profiling will be disabled")
#endif

#if GLSLANG_VERSION_MAJOR != 11 || GLSLANG_VERSION_MINOR != 0 || GLSLANG_VERSION_PATCH != 0
#	error invalid glslang version
#endif

#if 0 //def AE_ENABLE_SPIRV_CROSS
#	include "spirv_cross.hpp"
#	include "spirv_msl.hpp"
#	include "spirv_glsl.hpp"
#endif


namespace AE::PipelineCompiler
{
	
	//
	// Shader Includer
	//
	class SpirvCompiler::ShaderIncluder final : public glslang::TShader::Includer
	{
	// types
	private:
		struct IncludeResultImpl final : IncludeResult
		{
			const String	_data;

			IncludeResultImpl (String &&data, const String& headerName, void* userData = null) :
				IncludeResult{headerName, null, 0, userData}, _data{std::move(data)}
			{
				const_cast<const char*&>(headerData) = _data.c_str();
				const_cast<usize&>(headerLength)    = _data.length();
			}

			ND_ StringView	GetSource () const	{ return _data; }
		};

		using IncludeResultPtr_t	= UniquePtr< IncludeResultImpl >;
		using IncludeResults_t		= Array< IncludeResultPtr_t >;
		using IncludedFiles_t		= HashMap< String, Ptr<IncludeResultImpl> >;


	// variables
	private:
		IncludeResults_t	_results;
		IncludedFiles_t		_includedFiles;
		IncludeDirs_t		_directories;


	// methods
	public:
		explicit ShaderIncluder (IncludeDirs_t dirs) : _directories{dirs} {}
		~ShaderIncluder () override {}

		ND_ IncludedFiles_t const&  GetIncludedFiles () const	{ return _includedFiles; }

		// TShader::Includer //
		IncludeResult*  includeSystem (const char* headerName, const char* includerName, usize inclusionDepth) override;
		IncludeResult*  includeLocal (const char* headerName, const char* includerName, usize inclusionDepth) override;

		void  releaseInclude (IncludeResult *) override {}
	};


/*
=================================================
	includeSystem
=================================================
*/
	SpirvCompiler::ShaderIncluder::IncludeResult*
		SpirvCompiler::ShaderIncluder::includeSystem (const char*, const char *, usize)
	{
		return null;
	}
	
/*
=================================================
	includeLocal
=================================================
*/
	SpirvCompiler::ShaderIncluder::IncludeResult*
		SpirvCompiler::ShaderIncluder::includeLocal (const char* headerName, const char *, usize)
	{
		ASSERT( _directories.size() );

		for (auto& folder : _directories)
		{
			Path	fpath = Path( folder ) / headerName;

			if ( not FileSystem::Exists( fpath ) )
				continue;

			const String	filename = fpath.make_preferred().string();

			// prevent recursive include
			if ( _includedFiles.count( filename ))
				return _results.emplace_back(new IncludeResultImpl{ "// skip header\n", headerName }).get();
			
			FileRStream  file{ filename };
			CHECK_ERR( file.IsOpen() );

			String	data;
			CHECK_ERR( file.Read( usize(file.Size()), OUT data ));

			auto*	result = _results.emplace_back(new IncludeResultImpl{ std::move(data), headerName }).get();

			_includedFiles.insert_or_assign( filename, result );
			return result;
		}

		return null;
	}
//-----------------------------------------------------------------------------
	


	//
	// GLSLang Result
	//
	struct SpirvCompiler::GLSLangResult
	{
		glslang::TProgram				prog;
		UniquePtr< glslang::TShader >	shader;
	};


/*
=================================================
	constructor
=================================================
*/
	SpirvCompiler::SpirvCompiler (IncludeDirs_t dirs) : _directories{dirs}
	{
		glslang::InitializeProcess();

		_GenerateResources( OUT _builtinResource );
	}
	
/*
=================================================
	destructor
=================================================
*/
	SpirvCompiler::~SpirvCompiler ()
	{
		glslang::FinalizeProcess();
	}

/*
=================================================
	BuildReflection
=================================================
*/
	bool  SpirvCompiler::BuildReflection (EShader shaderType, uint spirvVersion, NtStringView entry, NtStringView source, OUT ShaderReflection &outReflection, OUT String &log)
	{
		log.clear();

		_currentStage = EShaderStages_FromShader( shaderType );
		
		ShaderIncluder	includer	{_directories};
		GLSLangResult	glslang_data;

		COMP_CHECK_ERR( _ParseGLSL( shaderType, spirvVersion, entry, {source.c_str()}, INOUT includer, OUT glslang_data, INOUT log ));

		Array<uint>		spirv;
		COMP_CHECK_ERR( _CompileSPIRV( glslang_data, OUT spirv, INOUT log ));

		COMP_CHECK_ERR( _BuildReflection( glslang_data, OUT outReflection ));
		
		if ( _parseAnnotations )
		{
			_ParseAnnotations( StringView{source}, INOUT outReflection );

			for (auto& file : includer.GetIncludedFiles()) {
				_ParseAnnotations( file.second->GetSource(), INOUT outReflection );
			}
		}

		return true;
	}
	
/*
=================================================
	Compile
=================================================
*/
	bool  SpirvCompiler::Compile (EShader shaderType, uint spirvVersion, NtStringView entry, NtStringView source, OUT Array<uint> &outSpirv, OUT String &log)
	{
		log.clear();

		_currentStage = EShaderStages_FromShader( shaderType );
		
		ShaderIncluder	includer	{_directories};
		GLSLangResult	glslang_data;

		COMP_CHECK_ERR( _ParseGLSL( shaderType, spirvVersion, entry, {source.c_str()}, INOUT includer, OUT glslang_data, INOUT log ));

		outSpirv.clear();
		COMP_CHECK_ERR( _CompileSPIRV( glslang_data, OUT outSpirv, INOUT log ));

		return true;
	}

/*
=================================================
	ConvertShaderType
=================================================
*/
	ND_ static EShLanguage  ConvertShaderType (EShader shaderType)
	{
		BEGIN_ENUM_CHECKS();
		switch ( shaderType )
		{
			case EShader::Vertex :			return EShLangVertex;
			case EShader::TessControl :		return EShLangTessControl;
			case EShader::TessEvaluation :	return EShLangTessEvaluation;
			case EShader::Geometry :		return EShLangGeometry;
			case EShader::Fragment :		return EShLangFragment;
			case EShader::Compute :			return EShLangCompute;
			case EShader::MeshTask :		return EShLangTaskNV;
			case EShader::Mesh :			return EShLangMeshNV;
			case EShader::RayGen :			return EShLangRayGen;
			case EShader::RayAnyHit :		return EShLangAnyHit;
			case EShader::RayClosestHit :	return EShLangClosestHit;
			case EShader::RayMiss :			return EShLangMiss;
			case EShader::RayIntersection:	return EShLangIntersect;
			case EShader::RayCallable :		return EShLangCallable;
			case EShader::Unknown :
			case EShader::_Count :			break;	// to shutup warnings
		}
		END_ENUM_CHECKS();
		RETURN_ERR( "unknown shader type", EShLangCount );
	}

/*
=================================================
	_ParseGLSL
=================================================
*/
	bool SpirvCompiler::_ParseGLSL (EShader shaderType, uint spirvVersion, NtStringView entry, ArrayView<const char *> source, INOUT ShaderIncluder &includer,
									OUT GLSLangResult &glslangData, OUT String &log)
	{
		using namespace glslang;

		const EShClient					client			= EShClientVulkan;
			  EshTargetClientVersion	client_version	= EShTargetVulkan_1_0;

		const EShTargetLanguage			target			= EShTargetSpv;
			  EShTargetLanguageVersion	target_version	= EShTargetSpv_1_2;
		
			  int						vk_version		= 100;
		const int		 				sh_version		= 460;
		const EProfile					sh_profile		= ECoreProfile;
		const EShSource					sh_source		= EShSourceGlsl;

		switch ( spirvVersion )
		{
			case 100 :
				_spirvTraget	= SPV_ENV_VULKAN_1_0;
				target_version	= EShTargetSpv_1_0;
				vk_version		= 100;
				client_version	= EShTargetVulkan_1_0;
				break;
			case 110 :
				_spirvTraget	= SPV_ENV_VULKAN_1_0;
				target_version	= EShTargetSpv_1_1;
				vk_version		= 100;
				client_version	= EShTargetVulkan_1_0;
				break;
			case 120 :
				_spirvTraget	= SPV_ENV_VULKAN_1_0;
				target_version	= EShTargetSpv_1_2;
				vk_version		= 100;
				client_version	= EShTargetVulkan_1_0;
				break;
			case 130 :
				_spirvTraget	= SPV_ENV_VULKAN_1_1;
				target_version	= EShTargetSpv_1_3;
				vk_version		= 110;
				client_version	= EShTargetVulkan_1_1;
				break;
			case 140 :
				_spirvTraget	= SPV_ENV_VULKAN_1_1_SPIRV_1_4;
				target_version	= EShTargetSpv_1_4;
				vk_version		= 110;
				client_version	= EShTargetVulkan_1_1;
				break;
			case 150 :
				_spirvTraget	= SPV_ENV_VULKAN_1_2;
				target_version	= EShTargetSpv_1_5;
				vk_version		= 120;
				client_version	= EShTargetVulkan_1_2;
			default :
				RETURN_ERR( "unsupported SPIRV version" );
		}

		EShMessages		messages	= EShMsgDefault;
		EShLanguage		stage		= ConvertShaderType( shaderType );
		auto&			shader		= glslangData.shader;

		shader.reset( new TShader( stage ));
		shader->setStrings( source.data(), int(source.size()) );
		shader->setEntryPoint( entry.c_str() );
		shader->setEnvInput( sh_source, stage, client, vk_version );
		shader->setEnvClient( client, client_version );
		shader->setEnvTarget( target, target_version );
		
		shader->setAutoMapLocations( true );
		shader->setAutoMapBindings( true );

		if ( not shader->parse( &_builtinResource, sh_version, sh_profile, false, true, messages, includer ))
		{
			log += shader->getInfoLog();
			_OnCompilationFailed( source, INOUT log );
			return false;
		}
		
		glslangData.prog.addShader( shader.get() );

		if ( not glslangData.prog.link( messages ) )
		{
			log += glslangData.prog.getInfoLog();
			_OnCompilationFailed( source, INOUT log );
			return false;
		}

		if ( not glslangData.prog.mapIO() )		// TODO: use resolver
		{
			log += "mapIO - failed";
			return false;
		}

		return true;
	}

/*
=================================================
	_CompileSPIRV
=================================================
*/
	bool  SpirvCompiler::_CompileSPIRV (const GLSLangResult &glslangData, OUT Array<uint> &spirv, OUT String &log) const
	{
		using namespace glslang;

		const TIntermediate* intermediate = glslangData.prog.getIntermediate( glslangData.shader->getStage() );
		COMP_CHECK_ERR( intermediate );

		SpvOptions				spv_options;
		spv::SpvBuildLogger		logger;

		spv_options.generateDebugInfo	= false;
		spv_options.disableOptimizer	= true;
		spv_options.optimizeSize		= false;	// TODO
		
		GlslangToSpv( *intermediate, OUT spirv, &logger, &spv_options );
		log += logger.getAllMessages();

		#ifdef ENABLE_OPT
			if ( false )	// TODO
				CHECK_ERR( _OptimizeSPIRV( INOUT spirv, OUT log ));
		#endif

		return true;
	}
	
#ifdef ENABLE_OPT
/*
=================================================
	DisassembleSPIRV
=================================================
*/
	inline bool  DisassembleSPIRV (spv_target_env targetEnv, const Array<uint> &spirv, OUT String &result)
	{
		result.clear();

		spv_context	ctx = spvContextCreate( targetEnv );
		CHECK_ERR( ctx != null );

		spv_text		text		= null;
		spv_diagnostic	diagnostic	= null;

		if ( spvBinaryToText( ctx, spirv.data(), spirv.size(), 0, &text, &diagnostic ) == SPV_SUCCESS )
		{
			result = String{ text->str, text->length };
		}
		
		spvTextDestroy( text );
		spvDiagnosticDestroy( diagnostic );
		spvContextDestroy( ctx );

		return true;
	}

/*
=================================================
	_OptimizeSPIRV
=================================================
*/
	bool  SpirvCompiler::_OptimizeSPIRV (INOUT Array<uint> &spirv, INOUT String &log) const
	{
		spv_target_env	target_env = BitCast<spv_target_env>( _spirvTraget );

		spvtools::Optimizer	optimizer{ target_env };
		optimizer.SetMessageConsumer(
			[&log] (spv_message_level_t level, const char *source, const spv_position_t &position, const char *message) {
				switch ( level )
				{
					case SPV_MSG_FATAL:
					case SPV_MSG_INTERNAL_ERROR:
					case SPV_MSG_ERROR:
						log << "error: ";
						break;
					case SPV_MSG_WARNING:
						log << "warning: ";
						break;
					case SPV_MSG_INFO:
					case SPV_MSG_DEBUG:
						log << "info: ";
						break;
				}

				if ( source )
					log << source << ":";
				
				log << ToString(position.line) << ":" << ToString(position.column) << ":" << ToString(position.index) << ":";
				if ( message )
					log << " " << message;
			});

		optimizer.RegisterLegalizationPasses();
		optimizer.RegisterSizePasses();
		optimizer.RegisterPerformancePasses();

		optimizer.RegisterPass(spvtools::CreateCompactIdsPass());
		optimizer.RegisterPass(spvtools::CreateAggressiveDCEPass());;
		optimizer.RegisterPass(spvtools::CreateRemoveDuplicatesPass());
		optimizer.RegisterPass(spvtools::CreateCFGCleanupPass());
		
		spvtools::OptimizerOptions spvOptOptions;
		spvOptOptions.set_run_validator(false);

		optimizer.Run(spirv.data(), spirv.size(), &spirv, spvOptOptions);
		return true;
	}
#endif	// ENABLE_OPT

/*
=================================================
	ReadLine
=================================================
*/
	inline bool  ReadLine (StringView log, INOUT usize &pos, OUT StringView &line)
	{
		usize	begin = pos;

		line = Default;

		// find new line
		for (; pos < log.size(); ++pos)
		{
			const char	c = log[pos];

			if ( c == '\r' or c == '\n' )
				break;
		}

		if ( pos == begin )
			return false;

		line = log.substr( begin, pos - begin );

		// skip empty lines
		for (; pos < log.size(); ++pos)
		{
			const char	c = log[pos];

			if ( c != '\r' and c != '\n' )
				break;
		}
		return true;
	}
	
/*
=================================================
	ParseGLSLError
=================================================
*/
	struct GLSLErrorInfo
	{
		StringView	description;
		StringView	fileName;
		uint		sourceIndex;
		usize		line;
		bool		isError;

		GLSLErrorInfo () : sourceIndex{0}, line{UMax}, isError{false} {}
	};
	
	static bool  ParseGLSLError (StringView line, OUT GLSLErrorInfo &info)
	{
		const StringView	c_error		= "error";
		const StringView	c_warning	= "warning";

		usize				pos = 0;

		const auto			ReadToken	= [&line, &pos] (OUT bool &isNumber)
		{{
							isNumber= true;
			const usize	start	= pos;

			for (; pos < line.length() and line[pos] != ':'; ++pos) {
				isNumber &= (line[pos] >= '0' and line[pos] <= '9');
			}
			return line.substr( start, pos - start );
		}};

		const auto			SkipSeparator = [&line, &pos] ()
		{{
			if ( pos+1 < line.length() and line[pos] == ':' and line[pos+1] == ' ' )
				pos += 2;
			else
			if ( pos < line.length() and line[pos] == ':' )
				pos += 1;
			else
				return false;

			return true;
		}};
		//---------------------------------------------------------------------------

		
		// parse error/warning/...
		if ( StartsWithIC( line, c_error ))
		{
			pos			+= c_error.length();
			info.isError = true;
		}
		else
		if ( StartsWithIC( line, c_warning ))
		{
			pos			+= c_warning.length();
			info.isError = false;
		}
		else
			return false;

		if ( not SkipSeparator() )
			return false;


		// parse source index or header name
		{
			bool				is_number;
			const StringView	src		= ReadToken( OUT is_number );

			if ( not SkipSeparator() )
				return false;

			if ( not is_number )
				info.fileName = src;
			else
				info.sourceIndex = uint(std::stoi( String(src) ));
		}


		// parse line number
		{
			bool				is_number;
			const StringView	src		= ReadToken( OUT is_number );

			if ( not SkipSeparator() or not is_number )
				return false;

			info.line = uint(std::stoi( String(src) ));
		}

		info.description = line.substr( pos );
		return true;
	}

/*
=================================================
	_OnCompilationFailed
=================================================
*/
	bool  SpirvCompiler::_OnCompilationFailed (ArrayView<const char *> source, INOUT String &log) const
	{
		// glslang errors format:
		// pattern: <error/warning>: <number>:<line>: <description>
		// pattern: <error/warning>: <file>:<line>: <description>
		
		StringView		line;
		uint			line_number = 0;
		usize			prev_line	= UMax;
		usize			pos			= 0;
		String			str;		str.reserve( log.length() );
		Array<usize>	num_lines;	num_lines.resize( source.size() );

		for (; ReadLine( log, INOUT pos, OUT line ); ++line_number)
		{
			GLSLErrorInfo	error_info;
			bool			added = false;

			if ( ParseGLSLError( line, OUT error_info ))
			{
				// unite error in same source lines
				if ( prev_line == error_info.line )
				{
					str << line << "\n";
					continue;
				}

				prev_line = error_info.line;

				if ( error_info.fileName.empty() )
				{
					// search in sources
					StringView	cur_source	= error_info.sourceIndex < source.size() ? source[ error_info.sourceIndex ] : "";
					usize		lines_count	= error_info.sourceIndex < num_lines.size() ? num_lines[ error_info.sourceIndex ] : 0;

					if ( lines_count == 0 and error_info.sourceIndex < num_lines.size() )
					{
						lines_count = StringParser::CalculateNumberOfLines( cur_source ) + 1;
						num_lines[ error_info.sourceIndex ] = lines_count;
					}
					
					CHECK( error_info.line <= lines_count );

					usize	line_pos = 0;
					CHECK( StringParser::MoveToLine( cur_source, INOUT line_pos, error_info.line-1 ));

					StringView	line_str;
					StringParser::ReadLineToEnd( cur_source, INOUT line_pos, OUT line_str );

					str << "in source (" << ToString(error_info.sourceIndex) << ": " << ToString(error_info.line) << "):\n\"" << line_str << "\"\n" << line << "\n";
					added = true;
				}
				else
				{
					// search in header
					/*StringView	src;
					if ( includer.GetHeaderSource( error_info.fileName, OUT src ))
					{
						const usize	lines_count = StringParser::CalculateNumberOfLines( src ) + 1;
						const usize	local_line	= error_info.line;
						usize			line_pos	= 0;
						StringView		line_str;

						CHECK( local_line < lines_count );
						CHECK( StringParser::MoveToLine( src, INOUT line_pos, local_line-1 ));
						
						StringParser::ReadLineToEnd( src, INOUT line_pos, OUT line_str );

						str << "in source (" << error_info.fileName << ": " << ToString(local_line) << "):\n\"" << line_str << "\"\n" << line << "\n";
						added = true;
					}*/
				}
			}
			
			if ( not added )
			{
				str << DEBUG_ONLY( "<unknown> " << ) line << "\n";
			}
		}

		std::swap( log, str );
		return true;
	}
	
/*
=================================================
	_BuildReflection 
=================================================
*/
	bool  SpirvCompiler::_BuildReflection (const GLSLangResult &glslangData, OUT ShaderReflection &result)
	{
		_intermediate = glslangData.prog.getIntermediate( glslangData.shader->getStage() );
		COMP_CHECK_ERR( _intermediate );

		// deserialize shader
		TIntermNode*	root	= _intermediate->getTreeRoot();
		
		COMP_CHECK_ERR( _ProcessExternalObjects( null, root, OUT result ));
		COMP_CHECK_ERR( _ProcessShaderInfo( INOUT result ));

		for (auto& ds : result.layout.descrSets)
		{
			ds.layout.SortUniforms();
		}

		_intermediate = null;
		return true;
	}
	
/*
=================================================
	FindUniform 
=================================================
*/
	ND_ static DescriptorSetLayoutDesc::Uniform*  FindUniform (UniformName name, DescriptorSetLayoutDesc &layout)
	{
		struct Key
		{
			UniformName const&	name;

			Key (const UniformName &n) : name{n} {}

			bool operator > (const Pair<UniformName, DescriptorSetLayoutDesc::Uniform> &un) const {
				return name > un.first;
			}

			bool operator == (const Pair<UniformName, DescriptorSetLayoutDesc::Uniform> &un) const {
				return name == un.first;
			}
		};

		usize	pos = LowerBound( layout.uniforms, Key{name} );
		return pos < layout.uniforms.size() ? &layout.uniforms[pos].second : null;
	}

/*
=================================================
	_ParseAnnotations
----
	supported annotations:
		set <int> <string>	-- descriptor set name
		discard				-- changes writeonly access to writediscard
		dynamic-offset		-- uniform or storage buffer uses dynamic offset
	multiple annotations:
		@<annotation-1>, @<annotation-2>, ...
=================================================
*/
	bool SpirvCompiler::_ParseAnnotations (StringView source, INOUT ShaderReflection &reflection) const
	{
		struct Annotation
		{
			bool	writeDiscard  : 1;
			bool	dynamicOffset : 1;

			Annotation () : writeDiscard{false}, dynamicOffset{false} {}
		};

		const auto	ReadWord = [] (StringView src, INOUT usize &pos)
		{
			const usize	start = pos;

			for (bool is_word = true; is_word & (pos < src.length()); pos += is_word)
			{
				const char	c = src[pos];
				
				is_word = ((c >= 'a') & (c <= 'z')) | (c == '-');
			}
			return src.substr( start, pos - start );
		};

		const auto	ParseDescSet = [this, &reflection] (StringView src, INOUT usize &pos) -> bool
		{
			usize	start		= UMax;
			uint	mode		= 0;
			uint	ds_index	= 0;
			bool	is_string	= false;

			++pos;
			for (; mode < 3; ++pos)
			{
				const char	c = pos < src.length() ? src[pos] : '\n';

				if ( (c == '\n') | (c == '\r') | (c == ',') ) {
					++mode;	++pos;
					break;
				}

				switch ( mode )
				{
					// read descriptor set index
					case 0 : {
						if ( (c >= '0') & (c <= '9') )
							ds_index = ds_index * 10 + uint(c - '0');
						else {
							if ( (ds_index >= _maxDescrSets) | ((c != ' ') & (c != '\t')) )
								return false;
							++mode;
						}
						break;
					}
					// skip first white spaces
					case 1 : {
						if ( (c != ' ') & (c != '\t') ) {
							is_string = (c == '"');
							start = pos + uint(is_string);
							++mode;
						}
						break;
					}
					// find end of string
					case 2 : {
						if ( (is_string & (c == '"')) | ((not is_string) & ((c == ' ') | (c == '\t'))) )
							++mode;
						break;
					}
				}
			}
			COMP_CHECK_ERR( mode >= 3 );

			if ( start < pos )
			{
				bool	found = false;
				for (auto& ds : reflection.layout.descrSets)
				{
					if ( ds.bindingIndex == ds_index )
					{
						ds.name = DescriptorSetName{ src.substr( start, pos - start - 1 )};
						found   = true;
						break;
					}
				}
				//COMP_CHECK_ERR( found );
				return true;
			}
			return false;
		};
		
		const auto	ParseUniform = [this, &reflection] (StringView src, INOUT usize &pos, const Annotation &annot) -> bool
		{
			// patterns:
			//	buffer <SSBO> {...
			//	uniform <UBO> { ...
			//	uniform image* <name>...
			
			constexpr char		buffer_key[]	= "buffer";
			constexpr char		uinform_key[]	= "uniform";
			constexpr usize	max_length		= Max( CountOf(buffer_key), CountOf(uinform_key) ) - 1;

			usize	key_start = 0;
			for (; pos < src.length() - max_length; ++pos)
			{
				const char*	s		= src.data() + pos;
				bool		is_buf	= std::memcmp( s, buffer_key, sizeof(buffer_key)-1 ) == 0;
				bool		is_un	= std::memcmp( s, uinform_key, sizeof(uinform_key)-1 ) == 0;

				if ( is_buf | is_un )
				{
					key_start = pos;
					pos += max_length - usize(is_buf);
					break;
				}
			}

			// skip 'image*'
			const char	image_key[] = "image";
			for (usize i = pos; i < src.length()-1; ++i)
			{
				const char	c = src[i];
				const char	n = src[i+1];

				if ( (c == ' ') | (c == '\t') )
					continue;

				if ( (((c == 'i') | (c == 'u')) & (n == 'i')) | ((c == 'i') & (n == 'm')) )
				{
					for (; (i < src.length()) and (src[i] != ' ' and src[i] != '\t'); ++i) {}
					pos = i;
				}
				break;
			}

			// read uniform name
			usize	word_start = UMax, word_end = 0;
			for (; pos < src.length()-1; ++pos)
			{
				const char	c = src[pos];
				const char	n = src[pos+1];

				const bool	is_space1 = (c == ' ') | (c == '\t');
				const bool	is_space2 = (n == ' ') | (n == '\t') | (n == ';') | (n == '{');
				const bool	is_word1  = ((c >= 'a') & (c <= 'z')) | ((c >= 'A') & (c <= 'Z'));
				const bool	is_word2  = ((n >= 'a') & (n <= 'z')) | ((n >= 'A') & (n <= 'Z'));

				if ( is_space1 & is_word2 )
					word_start = pos+1;

				if ( is_space2 & is_word1 )
					word_end = pos+1;

				if ( (c == ';') | (c == '{') )
					break;
			}

			if ( word_start < word_end )
			{
				UniformName	id { src.substr( word_start, word_end - word_start )};
				bool		found = false;

				for (auto& ds : reflection.layout.descrSets)
				{
					auto*	iter = FindUniform( id, ds.layout );
					if ( not iter )
						continue;
					
					BEGIN_ENUM_CHECKS();
					switch ( iter->type )
					{
						case EDescriptorType::UniformBuffer :
							ASSERT( annot.dynamicOffset );
							ASSERT( not annot.writeDiscard );
							found = true;
							if ( annot.dynamicOffset )
								iter->buffer.dynamicOffsetIndex = 0;
							break;
							
						case EDescriptorType::StorageBuffer :
							ASSERT( annot.dynamicOffset or annot.writeDiscard );
							found = true;
							//if ( annot.writeDiscard )	iter->buffer.state |= EResourceState::InvalidateBefore; // TODO
							if ( annot.dynamicOffset )	iter->buffer.dynamicOffsetIndex = 0;
							break;
							
						case EDescriptorType::UniformTexelBuffer :
						case EDescriptorType::StorageTexelBuffer :
							ASSERT( not annot.dynamicOffset );
							found = true;
							//if ( annot.writeDiscard )	iter->texelBuffer.state |= EResourceState::InvalidateBefore;
							break;

						case EDescriptorType::StorageImage :
							ASSERT( not annot.dynamicOffset );
							ASSERT( annot.writeDiscard );
							found = true;
							//if ( annot.writeDiscard )	iter->storageImage.state |= EResourceState::InvalidateBefore;
							break;

						case EDescriptorType::SampledImage :
							ASSERT( not annot.dynamicOffset );
							ASSERT( not annot.writeDiscard );
							found = true;
							break;

						case EDescriptorType::CombinedImage :
							ASSERT( not annot.dynamicOffset );
							ASSERT( not annot.writeDiscard );
							found = true;
							break;

						case EDescriptorType::CombinedImage_ImmutableSampler :
							ASSERT( not annot.dynamicOffset );
							ASSERT( not annot.writeDiscard );
							found = true;
							break;

						case EDescriptorType::SubpassInput :
							ASSERT( not annot.dynamicOffset );
							ASSERT( not annot.writeDiscard );
							break;

						case EDescriptorType::Sampler :
							break;

						case EDescriptorType::ImmutableSampler :
							break;

						case EDescriptorType::RayTracingScene :
							// TODO
							//break;

						case EDescriptorType::Unknown :
						default :
							RETURN_ERR( "unknown descriptor type" );
					}
					END_ENUM_CHECKS();
				}
				COMP_CHECK_ERR( found );
				return true;
			}
			return false;
		};
		//---------------------------------------------------------------------------


		const StringView	desc_set_key {"set"};
		const StringView	discard_key  {"discard"};
		const StringView	dyn_off_key  {"dynamic-offset"};

		Annotation		annot;

		bool	multiline_comment	= false;
		bool	singleline_comment	= false;
		uint	line_number			= 0;

		for (usize pos = 0; pos < source.length();)
		{
			// search for annotation key
			{
				const char	c = source[pos];
				const char	n = (pos+1) >= source.length() ? 0 : source[pos+1];
				++pos;

				const bool	newline1 = (c == '\r') & (n == '\n');	// windows style "\r\n"
				const bool	newline2 = (c == '\n') | (c == '\r');	// linux style "\n" (or mac style "\r")

				if ( newline1 | newline2 )
				{
					pos += usize(newline1);
					singleline_comment = false;
					++line_number;

					if ( (annot.dynamicOffset | annot.writeDiscard) & (not multiline_comment) )
					{
						COMP_CHECK_ERR( ParseUniform( source, INOUT pos, annot ));
						annot = Default;
					}
					continue;
				}

				if ( multiline_comment )
				{
					if ( (c == '*') & (n == '/') )
						multiline_comment = false;
				}
				else
				{
					if ( (c == '/') & (n == '*') )
						multiline_comment = true;

					if ( (c == '/') & (n == '/') )
						singleline_comment = true;
				}

				if ( not ((singleline_comment | multiline_comment) & (c == '@')) )
					continue;
			}

			StringView	key = ReadWord( source, INOUT pos );

			if ( key == desc_set_key )
				COMP_CHECK_ERR( ParseDescSet( source, INOUT pos ))
			else
			if ( key == discard_key )
				annot.writeDiscard = true;
			else
			if ( key == dyn_off_key )
				annot.dynamicOffset = true;
			else
				AE_LOGD( "unknown annotation '"s << key << "'" );	// TODO: write to 'log'
		}

		return true;
	}

/*
=================================================
	_ProcessExternalObjects
=================================================
*/
	bool SpirvCompiler::_ProcessExternalObjects (TIntermNode*, TIntermNode* node, INOUT ShaderReflection &result) const
	{
		using namespace glslang;

		TIntermAggregate* aggr = node->getAsAggregate();
		
		if ( not aggr )
			return true;

		switch ( aggr->getOp() )
		{
			// continue deserializing
			case TOperator::EOpSequence :
			{
				for (auto& seq : aggr->getSequence())
				{
					COMP_CHECK_ERR( _ProcessExternalObjects( aggr, seq, INOUT result ));
				}
				return true;
			}

			// uniforms, buffers, ...
			case TOperator::EOpLinkerObjects :
			{
				for (auto& seq : aggr->getSequence())
				{
					COMP_CHECK_ERR( _DeserializeExternalObjects( seq, INOUT result ));
				}
				return true;
			}
		}
		return true;
	}
	
/*
=================================================
	GetArraySize
=================================================
*/
	ND_ static uint  GetArraySize (const glslang::TType &type)
	{
		auto*	sizes = type.getArraySizes();

		if ( not sizes or sizes->getNumDims() <= 0 )
			return 1;

		CHECK( sizes->getNumDims() == 1 );

		return sizes->getDimSize(0);
	}

/*
=================================================
	GetDesciptorSet
=================================================
*/
	ND_ static SpirvCompiler::ShaderReflection::DescriptorSet&  GetDesciptorSet (uint dsIndex, INOUT SpirvCompiler::ShaderReflection &reflection)
	{
		reflection.layout.descrSets.resize( Max( dsIndex+1, reflection.layout.descrSets.size() ));

		auto&	ds = reflection.layout.descrSets[dsIndex];
		ds.bindingIndex	= dsIndex;
		ds.name			= DescriptorSetName( ToString(dsIndex) );

		return ds;
	}
	
/*
=================================================
	_ExtractImageType
=================================================
*/
	EImageType  SpirvCompiler::_ExtractImageType (const glslang::TType &type) const
	{
		using namespace glslang;

		if ( type.getBasicType() != TBasicType::EbtSampler )
			COMP_RETURN_ERR( "type is not image/sampler type!" );

		TSampler const&	samp	= type.getSampler();
		EImageType		result	= Default;
			
		if ( samp.isShadow() )
		{
			ASSERT( not samp.isMultiSample() );
			result |= EImageType::Shadow;
		}
			
		BEGIN_ENUM_CHECKS();
		switch ( samp.getBasicType() )
		{
			case TBasicType::EbtFloat :
			case TBasicType::EbtDouble :
			case TBasicType::EbtFloat16 :
				result |= EImageType::Float;
				break;

			case TBasicType::EbtInt8 :
			case TBasicType::EbtInt16 :
			case TBasicType::EbtInt :
			case TBasicType::EbtInt64 :
				result |= EImageType::Int;
				break;

			case TBasicType::EbtUint8 :
			case TBasicType::EbtUint16 :
			case TBasicType::EbtUint :
			case TBasicType::EbtUint64 :
				result |= EImageType::UInt;
				break;
					
			case TBasicType::EbtVoid :
			case TBasicType::EbtBool :
			case TBasicType::EbtAtomicUint :
			case TBasicType::EbtSampler :
			case TBasicType::EbtStruct :
			case TBasicType::EbtBlock :
			case TBasicType::EbtAccStruct :
			case TBasicType::EbtRayQuery :
			case TBasicType::EbtReference :
			case TBasicType::EbtString :
			case TBasicType::EbtNumTypes :
			default :
				COMP_RETURN_ERR( "unknown basic type!" );
		}
		END_ENUM_CHECKS();

		BEGIN_ENUM_CHECKS();
		switch ( samp.dim )
		{
			case TSamplerDim::Esd1D :
			{
				if ( samp.isArrayed() )		result |= EImageType::Img1DArray;
				else						result |= EImageType::Img1D;
				break;
			}
			case TSamplerDim::Esd2D :
			{
				if ( samp.isMultiSample() and samp.isArrayed() )	result |= EImageType::Img2DMSArray;
				else if ( samp.isArrayed() )						result |= EImageType::Img2DArray;
				else if ( samp.isMultiSample() )					result |= EImageType::Img2DMS;
				else												result |= EImageType::Img2D;
				break;
			}
			case TSamplerDim::Esd3D :
			{
				ASSERT( not samp.isArrayed() );
				ASSERT( not samp.isMultiSample() );
				ASSERT( not samp.isShadow() );
				result |= EImageType::Img3D;
				break;
			}
			case TSamplerDim::EsdCube :
			{
				if ( samp.isArrayed() )	result |= EImageType::ImgCubeArray;
				else					result |= EImageType::ImgCube;
				break;
			}
			case TSamplerDim::EsdBuffer :
			case TSamplerDim::EsdNone :		// to shutup warnings
			case TSamplerDim::EsdRect :
			case TSamplerDim::EsdSubpass :
			case TSamplerDim::EsdNumDims :
			default :
				COMP_RETURN_ERR( "unknown sampler dimension type!" );
		}
		END_ENUM_CHECKS();

		if ( type.isSubpass() )
		{
			COMP_CHECK_ERR( samp.dim == TSamplerDim::Esd2D );
			COMP_CHECK_ERR( not samp.isArrayed() );
		}

		return result;
	}

/*
=================================================
	_ExtractImageFormat
=================================================
*/
	EPixelFormat  SpirvCompiler::_ExtractImageFormat (uint format) const
	{
		using namespace glslang;

		BEGIN_ENUM_CHECKS();
		switch ( BitCast<TLayoutFormat>(format) )
		{
			case TLayoutFormat::ElfNone :			return EPixelFormat::Unknown;
			case TLayoutFormat::ElfRgba32f :		return EPixelFormat::RGBA32F;
			case TLayoutFormat::ElfRgba16f :		return EPixelFormat::RGBA16F;
			case TLayoutFormat::ElfR32f :			return EPixelFormat::R32F;
			case TLayoutFormat::ElfRgba8 :			return EPixelFormat::RGBA8_UNorm;
			case TLayoutFormat::ElfRgba8Snorm :		return EPixelFormat::RGBA8_SNorm;
			case TLayoutFormat::ElfRg32f :			return EPixelFormat::RG32F;
			case TLayoutFormat::ElfRg16f :			return EPixelFormat::RG16F;
			case TLayoutFormat::ElfR11fG11fB10f :	return EPixelFormat::RGB_11_11_10F;
			case TLayoutFormat::ElfR16f :			return EPixelFormat::R16F;
			case TLayoutFormat::ElfRgba16 :			return EPixelFormat::RGBA16_UNorm;
			case TLayoutFormat::ElfRgb10A2 :		return EPixelFormat::RGB10_A2_UNorm;
			case TLayoutFormat::ElfRg16 :			return EPixelFormat::RG16_UNorm;
			case TLayoutFormat::ElfRg8 :			return EPixelFormat::RG8_UNorm;
			case TLayoutFormat::ElfR16 :			return EPixelFormat::R16_UNorm;
			case TLayoutFormat::ElfR8 :				return EPixelFormat::R8_UNorm;
			case TLayoutFormat::ElfRgba16Snorm :	return EPixelFormat::RGBA16_SNorm;
			case TLayoutFormat::ElfRg16Snorm :		return EPixelFormat::RG16_SNorm;
			case TLayoutFormat::ElfRg8Snorm :		return EPixelFormat::RG8_SNorm;
			case TLayoutFormat::ElfR16Snorm :		return EPixelFormat::R16_SNorm;
			case TLayoutFormat::ElfR8Snorm :		return EPixelFormat::R8_SNorm;
			case TLayoutFormat::ElfRgba32i :		return EPixelFormat::RGBA32I;
			case TLayoutFormat::ElfRgba16i :		return EPixelFormat::RGBA16I;
			case TLayoutFormat::ElfRgba8i :			return EPixelFormat::RGBA8I;
			case TLayoutFormat::ElfR32i :			return EPixelFormat::R32I;
			case TLayoutFormat::ElfRg32i :			return EPixelFormat::RG32I;
			case TLayoutFormat::ElfRg16i :			return EPixelFormat::RG16I;
			case TLayoutFormat::ElfRg8i :			return EPixelFormat::RG8I;
			case TLayoutFormat::ElfR16i :			return EPixelFormat::R16I;
			case TLayoutFormat::ElfR8i :			return EPixelFormat::R8I;
			case TLayoutFormat::ElfRgba32ui :		return EPixelFormat::RGBA32U;
			case TLayoutFormat::ElfRgba16ui :		return EPixelFormat::RGBA16U;
			case TLayoutFormat::ElfRgba8ui :		return EPixelFormat::RGBA8U;
			case TLayoutFormat::ElfR32ui :			return EPixelFormat::R32U;
			case TLayoutFormat::ElfRg32ui :			return EPixelFormat::RG32U;
			case TLayoutFormat::ElfRg16ui :			return EPixelFormat::RG16U;
			case TLayoutFormat::ElfRgb10a2ui :		return EPixelFormat::RGB10_A2U;
			case TLayoutFormat::ElfRg8ui :			return EPixelFormat::RG8U;
			case TLayoutFormat::ElfR16ui :			return EPixelFormat::R16U;
			case TLayoutFormat::ElfR8ui :			return EPixelFormat::R8U;
			case TLayoutFormat::ElfR64i :			return EPixelFormat::R64I;
			case TLayoutFormat::ElfR64ui :			return EPixelFormat::R64U;
			case TLayoutFormat::ElfEsFloatGuard :
			case TLayoutFormat::ElfFloatGuard :
			case TLayoutFormat::ElfEsIntGuard :
			case TLayoutFormat::ElfIntGuard :
			case TLayoutFormat::ElfEsUintGuard :
			case TLayoutFormat::ElfCount :			break;	// to shutup warnings
		}
		END_ENUM_CHECKS();
		COMP_RETURN_ERR( "Unsupported image format!" );
	}
	
/*
=================================================
	ExtractShaderAccessType
=================================================
*/
	ND_ static EResourceState  ExtractShaderAccessType (const glslang::TQualifier &q)
	{
		if ( q.coherent or
			 q.volatil	or
			 q.restrict )
		{
			return EResourceState::ShaderReadWrite;
		}

		if ( q.readonly )
			return EResourceState::ShaderRead;

		if ( q.writeonly )
			return EResourceState::ShaderWrite;

		// defualt:
		return EResourceState::ShaderReadWrite;
	}

/*
=================================================
	ExtractNodeName
=================================================
*/
	ND_ static String  ExtractNodeName (TIntermNode *node)
	{
		CHECK_ERR( node and node->getAsSymbolNode() );
		
		String				name	= node->getAsSymbolNode()->getName().c_str();
		const StringView	prefix	= "anon@";

		if ( not name.compare( 0, prefix.size(), prefix.data() ))
			name.clear();

		return name;
	}
	
/*
=================================================
	Extract***ID
=================================================
*/
	ND_ static UniformName  ExtractUniformID (TIntermNode *node)
	{
		return UniformName( ExtractNodeName( node ));
	}
	
	ND_ static VertexName  ExtractVertexID (TIntermNode *node)
	{
		return VertexName( ExtractNodeName( node ));
	}

	ND_ static RenderTargetName  ExtractRenderTargetID (TIntermNode *node)
	{
		return RenderTargetName( ExtractNodeName( node ));
	}

	ND_ static SpecializationName  ExtractSpecializationID (TIntermNode *node)
	{
		return SpecializationName( ExtractNodeName( node ));
	}

/*
=================================================
	ExtractBufferUniformID
=================================================
*/
	ND_ static UniformName  ExtractBufferUniformID (const glslang::TType &type)
	{
		return UniformName( type.getTypeName().c_str() );
	}

/*
=================================================
	_ExtractVertexType
=================================================
*/
	EVertexType  SpirvCompiler::_ExtractVertexType (const glslang::TType &type) const
	{
		using namespace glslang;

		EVertexType		result = EVertexType(0);

		COMP_CHECK_ERR( not type.isArray() );
		
		BEGIN_ENUM_CHECKS();
		switch ( type.getBasicType() )
		{
			case TBasicType::EbtFloat :		result |= EVertexType::_Float;	break;
			case TBasicType::EbtDouble :	result |= EVertexType::_Double;	break;
			case TBasicType::EbtFloat16 :	result |= EVertexType::_Half;	break;
			case TBasicType::EbtInt8 :		result |= EVertexType::_Byte;	break;
			case TBasicType::EbtUint8 :		result |= EVertexType::_UByte;	break;
			case TBasicType::EbtInt16 :		result |= EVertexType::_Short;	break;
			case TBasicType::EbtUint16 :	result |= EVertexType::_UShort;	break;
			case TBasicType::EbtInt :		result |= EVertexType::_Int;	break;
			case TBasicType::EbtUint :		result |= EVertexType::_UInt;	break;
			case TBasicType::EbtInt64 :		result |= EVertexType::_Long;	break;
			case TBasicType::EbtUint64 :	result |= EVertexType::_ULong;	break;
			//case TBasicType::EbtBool :	result |= EVertexType::_Bool;	break;
			case TBasicType::EbtVoid :		// to shutup warnings
			case TBasicType::EbtBool :
			case TBasicType::EbtAtomicUint :
			case TBasicType::EbtSampler :
			case TBasicType::EbtStruct :
			case TBasicType::EbtBlock :
			case TBasicType::EbtAccStruct :
			case TBasicType::EbtRayQuery :
			case TBasicType::EbtString :
			case TBasicType::EbtReference :
			case TBasicType::EbtNumTypes :
			default :						COMP_RETURN_ERR( "unsupported basic type!" );
		}
		END_ENUM_CHECKS();

		if ( type.isScalarOrVec1() )
			return result | EVertexType::_Vec1;

		if ( type.isVector() )
		{
			switch ( type.getVectorSize() )
			{
				case 1 :	result |= EVertexType::_Vec1;	break;
				case 2 :	result |= EVertexType::_Vec2;	break;
				case 3 :	result |= EVertexType::_Vec3;	break;
				case 4 :	result |= EVertexType::_Vec4;	break;
				default :	COMP_RETURN_ERR( "unsupported vector size!" );
			}
			return result;
		}

		if ( type.isMatrix() )
		{
			COMP_RETURN_ERR( "not supported, yet" );
		}

		COMP_RETURN_ERR( "unknown vertex type" );
	}
	
/*
=================================================
	_ExtractFragmentOutputType
=================================================
*/
	EFragOutput  SpirvCompiler::_ExtractFragmentOutputType (const glslang::TType &type) const
	{
		using namespace glslang;

		COMP_CHECK_ERR( type.getVectorSize() == 4 );

		switch ( type.getBasicType() )
		{
			case TBasicType::EbtFloat :		return EFragOutput::Float4;
			case TBasicType::EbtInt :		return EFragOutput::Int4;
			case TBasicType::EbtUint :		return EFragOutput::UInt4;
		}
		COMP_RETURN_ERR( "unsupported fragment output" );
	}
	
/*
=================================================
	_CalculateStructSize
----
	based on TParseContext::fixBlockUniformOffsets
=================================================
*/
	bool  SpirvCompiler::_CalculateStructSize (const glslang::TType &bufferType, OUT Bytes &staticSize, OUT Bytes &arrayStride, OUT Bytes &minOffset) const
	{
		using namespace glslang;

		staticSize = arrayStride = 0_b;
		minOffset = ~0_b;

		COMP_CHECK_ERR( bufferType.isStruct() );
		COMP_CHECK_ERR( bufferType.getQualifier().isUniformOrBuffer() or bufferType.getQualifier().layoutPushConstant );
		COMP_CHECK_ERR( bufferType.getQualifier().layoutPacking == ElpStd140 or
						bufferType.getQualifier().layoutPacking == ElpStd430 );

		int		member_size		= 0;
		int		offset			= 0;
		auto&	struct_fields	= *bufferType.getStruct();

		for (usize member = 0; member < struct_fields.size(); ++member)
		{
			const TType&		member_type			= *struct_fields[member].type;
			const TQualifier&	member_qualifier	= member_type.getQualifier();
			TLayoutMatrix		sub_matrix_layout	= member_qualifier.layoutMatrix;

			int dummy_stride;
			int member_alignment = _intermediate->getBaseAlignment( member_type, OUT member_size, OUT dummy_stride,
																	bufferType.getQualifier().layoutPacking,
																	sub_matrix_layout != ElmNone ? sub_matrix_layout == ElmRowMajor
																		: bufferType.getQualifier().layoutMatrix == ElmRowMajor );

			if ( member_qualifier.hasOffset() )
			{
				ASSERT( IsMultipleOfPow2( member_qualifier.layoutOffset, member_alignment ));

				if ( _intermediate->getSpv().spv == 0 )
				{
					ASSERT( member_qualifier.layoutOffset >= offset );

					offset = std::max( offset, member_qualifier.layoutOffset );
				}
				else {
					offset = member_qualifier.layoutOffset;
				}
			}

			if ( member_qualifier.hasAlign() )
				member_alignment = std::max( member_alignment, member_qualifier.layoutAlign );

			glslang::RoundToPow2( offset, member_alignment );
			//member_type.getQualifier().layoutOffset = offset;

			minOffset = Min( minOffset, uint(offset) );

			offset += member_size;

			// for last member
			if ( member+1 == struct_fields.size() and member_type.isUnsizedArray() )
			{
				ASSERT( member_size == 0 );

				arrayStride = Bytes(uint( dummy_stride ));
			}
		}
		staticSize = Bytes(uint( offset ));
		return true;
	}

/*
=================================================
	_DeserializeExternalObjects
=================================================
*/
	bool  SpirvCompiler::_DeserializeExternalObjects (TIntermNode* node, INOUT ShaderReflection &result) const
	{
		using namespace glslang;

		TIntermTyped*	tnode	= node->getAsTyped();
		auto const&		type	= tnode->getType();
		auto const&		qual	= type.getQualifier();
		
		// skip builtin
		if ( type.isBuiltIn() )
			return true;

		// shared variables
		if ( qual.storage == EvqShared )
			return true;

		// shader input
		if ( qual.storage == TStorageQualifier::EvqVaryingIn )
		{
			if ( _currentStage != EShaderStages::Vertex )
				return true;	// skip

			GraphicsPipelineDesc::VertexAttrib	attrib;
			attrib.id		= ExtractVertexID( node );
			attrib.index	= (qual.hasLocation() ? uint(qual.layoutLocation) : UMax);
			attrib.type		= _ExtractVertexType( type );

			result.vertex.vertexAttribs.push_back( std::move(attrib) );
			return true;
		}
		
		// shader output
		if ( qual.storage == TStorageQualifier::EvqVaryingOut )
		{
			if ( _currentStage != EShaderStages::Fragment )
				return true;	// skip

			FragmentOutput	frag_out;
			frag_out.index	= (qual.hasLocation() ? uint(qual.layoutLocation) : UMax);
			frag_out.type	= _ExtractFragmentOutputType( type );

			COMP_CHECK_ERR( result.fragment.fragmentOutput.insert_or_assign( ExtractRenderTargetID( node ), std::move(frag_out) ).second );
			return true;
		}

		// specialization constant
		if ( qual.storage == EvqConst and
			 qual.layoutSpecConstantId != TQualifier::layoutSpecConstantIdEnd )
		{
			COMP_CHECK_ERR( result.layout.specConstants.insert({ ExtractSpecializationID( node ), qual.layoutSpecConstantId }).second );
			return true;
		}

		// global variable or global constant
		if ( qual.storage == EvqGlobal or qual.storage == EvqConst )
			return true;


		auto&	descriptor_set = GetDesciptorSet( qual.hasSet() ? uint(qual.layoutSet) : 0, INOUT result );

		if ( type.getBasicType() == TBasicType::EbtSampler )
		{	
			TSampler const&	samp = type.getSampler();

			// image
			if ( samp.isImage() )
			{
				auto&[name, un]	= descriptor_set.layout.uniforms.emplace_back();
				name			= ExtractUniformID( node );
				un.index		= CheckCast<ushort>( qual.hasBinding() ? uint(qual.layoutBinding) : UMax );
				un.arraySize	= CheckCast<ushort>( GetArraySize( type ));
				un.type			= EDescriptorType::StorageImage;
				un.storageImage.type	= _ExtractImageType( type );
				un.storageImage.state	= ExtractShaderAccessType( qual ) | EResourceState_FromShaders( _currentStage );
				//un.storageImage.format= _ExtractImageFormat( qual.layoutFormat );
				return true;
			}
			
			// subpass
			if ( samp.isSubpass() )
			{
				auto&[name, un]	= descriptor_set.layout.uniforms.emplace_back();
				name			= ExtractUniformID( node );
				un.index		= CheckCast<ushort>( qual.hasBinding() ? uint(qual.layoutBinding) : UMax );
				un.arraySize	= CheckCast<ushort>( GetArraySize( type ));
				un.type			= EDescriptorType::SubpassInput;
				un.subpassInput.state	= EResourceState::InputAttachment | EResourceState_FromShaders( _currentStage );
				un.subpassInput.type	= _ExtractImageType( type );
				//un.subpass.attachmentIndex	= qual.hasAttachment() ? uint(qual.layoutAttachment) : UMax;
				//un.subpass.isMultisample	= false;	// TODO
				return true;
			}
			
			// sampler
			if ( samp.isPureSampler() )
			{
				auto&[name, un]	= descriptor_set.layout.uniforms.emplace_back();
				name			= ExtractUniformID( node );
				un.index		= CheckCast<ushort>( qual.hasBinding() ? uint(qual.layoutBinding) : UMax );
				un.arraySize	= CheckCast<ushort>( GetArraySize( type ));
				un.type			= EDescriptorType::Sampler;
				un.sampler.stageFlags	= _currentStage;
				return true;
			}

			// texel buffer
			if ( samp.isBuffer() )
			{
				ASSERT( qual.storage == TStorageQualifier::EvqUniform or qual.storage == TStorageQualifier::EvqBuffer );
				
				auto&[name, un]	= descriptor_set.layout.uniforms.emplace_back();
				name			= ExtractUniformID( node );
				un.index		= CheckCast<ushort>( qual.hasBinding() ? uint(qual.layoutBinding) : UMax );
				un.arraySize	= CheckCast<ushort>( GetArraySize( type ));
				un.type			= qual.storage == TStorageQualifier::EvqUniform ? EDescriptorType::UniformTexelBuffer : EDescriptorType::StorageTexelBuffer;
				un.texelBuffer.state	= EResourceState::ShaderSample | EResourceState_FromShaders( _currentStage );
				return true;
			}

			// texture
			if ( samp.isCombined() )
			{
				auto&[name, un]	= descriptor_set.layout.uniforms.emplace_back();
				name			= ExtractUniformID( node );
				un.index		= CheckCast<ushort>( qual.hasBinding() ? uint(qual.layoutBinding) : UMax );
				un.arraySize	= CheckCast<ushort>( GetArraySize( type ));
				un.type			= EDescriptorType::CombinedImage;
				un.combinedImage.state	= EResourceState::ShaderSample | EResourceState_FromShaders( _currentStage );
				un.combinedImage.type	= _ExtractImageType( type );
				return true;
			}
		}
		
		// push constants
		if ( qual.layoutPushConstant )
		{
			Bytes	size, stride, offset;
			COMP_CHECK_ERR( _CalculateStructSize( type, OUT size, OUT stride, OUT offset ));
			size -= offset;

			COMP_CHECK_ERR( result.layout.pushConstants.items.insert_or_assign( PushConstantName{type.getTypeName().c_str()}, PushConstant{ _currentStage, offset, size }).second );
			return true;
		}
		
		// uniform buffer or storage buffer
		if ( type.getBasicType() == TBasicType::EbtBlock	and
			 (qual.storage == TStorageQualifier::EvqUniform	or qual.storage == TStorageQualifier::EvqBuffer) )
		{
			COMP_CHECK_ERR( type.isStruct() );

			if ( qual.layoutShaderRecord )
				return true;

			// uniform block
			if ( qual.storage == TStorageQualifier::EvqUniform )
			{
				auto&[name, un]	= descriptor_set.layout.uniforms.emplace_back();
				name			= ExtractBufferUniformID( type );
				un.index		= CheckCast<ushort>( qual.hasBinding() ? uint(qual.layoutBinding) : UMax );
				un.arraySize	= CheckCast<ushort>( GetArraySize( type ));
				un.type			= EDescriptorType::UniformBuffer;
				un.buffer.state				= EResourceState::UniformRead | EResourceState_FromShaders( _currentStage );
				un.buffer.arrayStride		= 0_b;
				un.buffer.dynamicOffsetIndex= UMax;
				
				Bytes	stride, offset;
				COMP_CHECK_ERR( _CalculateStructSize( type, OUT un.buffer.staticSize, OUT stride, OUT offset ));
				COMP_CHECK_ERR( offset == 0_b );
				return true;
			}

			// storage block
			if ( qual.storage == TStorageQualifier::EvqBuffer )
			{
				auto&[name, un]	= descriptor_set.layout.uniforms.emplace_back();
				name			= ExtractBufferUniformID( type );
				un.index		= CheckCast<ushort>( qual.hasBinding() ? uint(qual.layoutBinding) : UMax );
				un.arraySize	= CheckCast<ushort>( GetArraySize( type ));
				un.type			= EDescriptorType::StorageBuffer;
				un.buffer.state				= ExtractShaderAccessType( qual ) | EResourceState_FromShaders( _currentStage );
				un.buffer.dynamicOffsetIndex= UMax;

				Bytes	offset;
				COMP_CHECK_ERR( _CalculateStructSize( type, OUT un.buffer.staticSize, OUT un.buffer.arrayStride, OUT offset ));
				COMP_CHECK_ERR( offset == 0_b );
				return true;
			}
		}
		
		// acceleration structure
		if ( type.getBasicType() == TBasicType::EbtAccStruct )
		{
			auto&[name, un]	= descriptor_set.layout.uniforms.emplace_back();
			name			= ExtractUniformID( node );
			un.index		= CheckCast<ushort>( qual.hasBinding() ? uint(qual.layoutBinding) : UMax );
			un.arraySize	= CheckCast<ushort>( GetArraySize( type ));
			un.type			= EDescriptorType::RayTracingScene;
			//rt_scene.state = EResourceState::_RayTracingShader | EResourceState::ShaderRead;
			return true;
		}

		if ( qual.storage == TStorageQualifier::EvqPayload		or
			 qual.storage == TStorageQualifier::EvqPayloadIn	or
			 qual.storage == TStorageQualifier::EvqHitAttr		or
			 qual.storage == TStorageQualifier::EvqCallableData	or
			 qual.storage == TStorageQualifier::EvqCallableDataIn )
		{
			return true;	// TODO
		}

		// uniform
		if ( qual.storage == TStorageQualifier::EvqUniform )
		{
			COMP_RETURN_ERR( "uniform is not supported for Vulkan!" );
		}

		COMP_RETURN_ERR( "unknown external type!" );
	}

/*
=================================================
	_MergeWithGeometryInputPrimitive
=================================================
*/
	void  SpirvCompiler::_MergeWithGeometryInputPrimitive (INOUT GraphicsPipelineDesc::TopologyBits_t &topologyBits, uint type) const
	{
		using namespace glslang;

		BEGIN_ENUM_CHECKS();
		switch ( BitCast<TLayoutGeometry>(type) )
		{
			case TLayoutGeometry::ElgPoints : {
				topologyBits.set( uint(EPrimitive::Point) );
				return;
			}
			case TLayoutGeometry::ElgLines : {
				topologyBits.set( uint(EPrimitive::LineList) );
				topologyBits.set( uint(EPrimitive::LineStrip) );
				return;
			}
			case TLayoutGeometry::ElgLinesAdjacency : {
				topologyBits.set( uint(EPrimitive::LineListAdjacency) );
				topologyBits.set( uint(EPrimitive::LineStripAdjacency) );
				return;
			}
			case TLayoutGeometry::ElgTriangles : {
				topologyBits.set( uint(EPrimitive::TriangleList) );
				topologyBits.set( uint(EPrimitive::TriangleStrip) );
				topologyBits.set( uint(EPrimitive::TriangleFan) );
				return;
			}
			case TLayoutGeometry::ElgTrianglesAdjacency : {
				topologyBits.set( uint(EPrimitive::TriangleListAdjacency) );
				topologyBits.set( uint(EPrimitive::TriangleStripAdjacency) );
				return;
			}
			case TLayoutGeometry::ElgNone :
			case TLayoutGeometry::ElgLineStrip :
			case TLayoutGeometry::ElgTriangleStrip :
			case TLayoutGeometry::ElgQuads :
			case TLayoutGeometry::ElgIsolines :
				break;	// to shutup warnings
		}
		END_ENUM_CHECKS();
		COMP_RETURN_ERR( "invalid geometry input primitive type!", void() );
	}
	
/*
=================================================
	_ProcessShaderInfo
=================================================
*/
	bool  SpirvCompiler::_ProcessShaderInfo (INOUT ShaderReflection &result) const
	{
		using namespace glslang;

		BEGIN_ENUM_CHECKS();
		switch ( _intermediate->getStage() )
		{
			case EShLangVertex :
			{
				break;
			}

			case EShLangTessControl :
			{
				result.tessellation.patchControlPoints	= uint(_intermediate->getVertices());

				result.vertex.supportedTopology.set( uint(EPrimitive::Patch) );
				break;
			}

			case EShLangTessEvaluation :
			{
				break;
			}

			case EShLangGeometry :
			{
				_MergeWithGeometryInputPrimitive( INOUT result.vertex.supportedTopology, _intermediate->getInputPrimitive() );
				break;
			}

			case EShLangFragment :
			{
				result.fragment.earlyFragmentTests = (_intermediate->getEarlyFragmentTests() or not _intermediate->isDepthReplacing());

				break;
			}

			case EShLangCompute :
			{
				result.compute.localGroupSize.x	= _intermediate->getLocalSize(0);
				result.compute.localGroupSize.y = _intermediate->getLocalSize(1);
				result.compute.localGroupSize.z	= _intermediate->getLocalSize(2);

				result.compute.localGroupSpecialization.x = uint(_intermediate->getLocalSizeSpecId(0));
				result.compute.localGroupSpecialization.y = uint(_intermediate->getLocalSizeSpecId(1));
				result.compute.localGroupSpecialization.z = uint(_intermediate->getLocalSizeSpecId(2));
				break;
			}

			case EShLangRayGen :
			{
				break;
			}
			case EShLangIntersect :
			{
				break;
			}
			case EShLangAnyHit :
			{
				break;
			}
			case EShLangClosestHit :
			{
				break;
			}
			case EShLangMiss :
			{
				break;
			}
			case EShLangCallable :
			{
				break;
			}

			case EShLangTaskNV :
			{
				result.mesh.taskGroupSize			= _intermediate->getLocalSize(0);
				result.mesh.taskGroupSpecialization = uint(_intermediate->getLocalSizeSpecId(0));

				CHECK( _intermediate->getLocalSize(1) <= 1 );
				CHECK( _intermediate->getLocalSize(2) <= 1 );
				CHECK( _intermediate->getLocalSizeSpecId(1) == -1 );
				CHECK( _intermediate->getLocalSizeSpecId(2) == -1 );
				break;
			}
			case EShLangMeshNV :
			{
				result.mesh.maxVertices		= uint(_intermediate->getVertices());
				result.mesh.maxPrimitives	= uint(_intermediate->getPrimitives());
				result.mesh.maxIndices		= result.mesh.maxPrimitives;

				END_ENUM_CHECKS();
				switch ( _intermediate->getOutputPrimitive() )
				{
					case TLayoutGeometry::ElgPoints :
						result.mesh.topology    = EPrimitive::Point;
						result.mesh.maxIndices *= 1;
						break;

					case TLayoutGeometry::ElgLines :
						result.mesh.topology    = EPrimitive::LineList;
						result.mesh.maxIndices *= 2;
						break;

					case TLayoutGeometry::ElgTriangles :
						result.mesh.topology    = EPrimitive::TriangleList;
						result.mesh.maxIndices *= 3;
						break;

					default :
						CHECK(false);
						break;
				}
				BEGIN_ENUM_CHECKS();

				result.mesh.meshGroupSize			= _intermediate->getLocalSize(0);
				result.mesh.meshGroupSpecialization	= uint(_intermediate->getLocalSizeSpecId(0));
				
				CHECK( _intermediate->getLocalSize(1) <= 1 );
				CHECK( _intermediate->getLocalSize(2) <= 1 );
				CHECK( _intermediate->getLocalSizeSpecId(1) == -1 );
				CHECK( _intermediate->getLocalSizeSpecId(2) == -1 );
				break;
			}

			case EShLangCount : break;
		}
		END_ENUM_CHECKS();
		return true;
	}

}	// AE::PipelineCompiler
