// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "Private/ScriptObjects.h"

#include "stl/Algorithms/StringUtils.h"
#include "stl/Algorithms/StringParser.h"
#include "stl/Stream/FileStream.h"

#include <locale>
#include <codecvt>

namespace AE::PipelineCompiler
{
namespace {
/*
=================================================
	StringToWString
=================================================
*/
	ND_ WString  StringToWString (const String &str)
	{
	#ifdef COMPILER_MSVC
	#	pragma warning( push )
	#	pragma warning( disable: 4996 )
	#endif

		std::wstring_convert<std::codecvt<wchar_t, char, std::mbstate_t>> converter;
		return converter.from_bytes( str );

	#ifdef COMPILER_MSVC
	#	pragma warning( pop )
	#endif
	}
	
/*
=================================================
	SortDefines
=================================================
*/
	void  SortDefines (INOUT ShaderDefines_t &defines)
	{
		const auto	CmpStrings = [](StringView lhs, StringView rhs) -> bool
		{
			const size_t	len = Min( lhs.size(), rhs.size() );
			for (size_t i = 0; i < len; ++i)
			{
				if ( lhs[i] == rhs[i] )
					continue;

				return ( lhs[i] > rhs[i] );
			}
			return lhs.size() > rhs.size();
		};
		std::sort( defines.begin(), defines.end(), CmpStrings );
	}
	
/*
=================================================
	SetDefines
=================================================
*/
	void  SetDefines (OUT ShaderDefines_t &defines, const String &def)
	{
		Array<StringView>	lines;
		StringParser::DivideLines( def, OUT lines );

		defines.clear();
		for (auto& line : lines) {
			defines.push_back( String{line} );
		}

		SortDefines( INOUT defines );
	}
}
//-----------------------------------------------------------------------------


	
/*
=================================================
	operator ==
=================================================
*/
	bool  ShaderInfo::operator == (const ShaderInfo &rhs) const
	{
		return	filename	== rhs.filename	and
				type		== rhs.type		and
				version		== rhs.version	and
				defines		== rhs.defines;
	}
	
/*
=================================================
	IsDefined
=================================================
*/
	bool  ShaderInfo::IsDefined () const
	{
		return not filename.empty();
	}
	
/*
=================================================
	hash
=================================================
*/
	size_t  ShaderInfoHash::operator () (const ShaderInfo &value) const
	{
		return size_t(HashOf( value.filename ) + HashOf( value.type ) +
					  HashOf( value.version )  + HashOf( value.defines ));
	}
//-----------------------------------------------------------------------------
	

	
/*
=================================================
	constructor
=================================================
*/
	BasePipeline::BasePipeline () :
		_filename{ std::move(ShaderStorage::Instance()->pipelineFilename) }
	{}
	
/*
=================================================
	SetName
=================================================
*/
	void BasePipeline::SetName (const String &value)
	{
		_name = PipelineName{ value };

		ShaderStorage::Instance()->hashCollisionCheck.Add( _name );
	}
	
/*
=================================================
	Define
=================================================
*/
	void BasePipeline::Define (const String &value)
	{
		CHECK( _defines.size() < _defines.capacity() );

		_defines.push_back( value );
		SortDefines( INOUT _defines );
	}
	
/*
=================================================
	AddLayout
=================================================
*/
	bool BasePipeline::AddLayout (const ShaderInfo &sh, OUT size_t &merged)
	{
		merged = 0;

		if ( not sh.IsDefined() )
			return true;

		auto	iter = ShaderStorage::Instance()->uniqueShaders.find( sh );
		CHECK_ERR( iter != ShaderStorage::Instance()->uniqueShaders.end() );

		for (auto& ds : iter->second.reflection.layout.descrSets)
		{
			if ( ds.bindingIndex >= _dsLayouts.size() )
			{
				CHECK( ds.layout.uniforms.empty() );
				continue;
			}

			if ( not _dsLayoutNames[ds.bindingIndex].IsDefined() )
			{
				_dsLayoutNames[ds.bindingIndex] = ds.name;
				
				ShaderStorage::Instance()->hashCollisionCheck.Add( ds.name );
			}
			else
				CHECK_ERR( _dsLayoutNames[ds.bindingIndex] == ds.name );

			size_t	count = 0;
			CHECK_ERR( _dsLayouts[ds.bindingIndex].Merge( ds.layout, OUT count ));
			merged += count;

			// TODO: check hash collision on sampler name
		}

		CHECK_ERR( _pushConstants.Merge( iter->second.reflection.layout.pushConstants ));

		for (auto& pc : _pushConstants.items)
		{
			ShaderStorage::Instance()->hashCollisionCheck.Add( pc.first );
		}
		return true;
	}
	
/*
=================================================
	MergeLayouts
=================================================
*/
	bool BasePipeline::MergeLayouts (const ShaderInfo &sh, OUT size_t &merged) const
	{
		merged = 0;

		if ( not sh.IsDefined() )
			return true;
		
		auto	iter = ShaderStorage::Instance()->uniqueShaders.find( sh );
		CHECK_ERR( iter != ShaderStorage::Instance()->uniqueShaders.end() );
		
		for (auto& ds : iter->second.reflection.layout.descrSets)
		{
			if ( ds.bindingIndex >= _dsLayouts.size() )
				continue;

			size_t	count = 0;
			CHECK_ERR( ds.layout.Merge( _dsLayouts[ds.bindingIndex], OUT count ));
			merged += count;
		}
		return true;
	}
	
/*
=================================================
	BuildLayout
=================================================
*/
	PipelineLayoutUID  BasePipeline::BuildLayout ()
	{
		auto&				ppln_storage = *ShaderStorage::Instance()->storage;
		PipelineLayoutDesc	desc;

		desc.pushConstants = _pushConstants;

		for (size_t i = 0; i < _dsLayouts.size(); ++i)
		{
			if ( _dsLayouts[i].uniforms.empty() )
				continue;

			auto[dst, inserted] = desc.descrSets.insert_or_assign( _dsLayoutNames[i], PipelineLayoutDesc::DescSetLayout{} );
			CHECK( inserted );
			
			dst->second.index = uint(i);
			dst->second.uid   = ppln_storage.AddDescriptorSetLayout( std::move(_dsLayouts[i]) );
		}

		_dsLayouts.fill({});
		_dsLayoutNames.fill({});

		return ppln_storage.AddPipelineLayout( std::move(desc) );
	}
//-----------------------------------------------------------------------------


	
/*
=================================================
	constructor
=================================================
*/
	GraphicsPipelineScriptBinding::GraphicsPipelineScriptBinding ()
	{
		ShaderStorage::Instance()->gpipelines.push_back( GraphicsPipelinePtr{this} );
	}
	
/*
=================================================
	SetVertexShader
=================================================
*/
	void GraphicsPipelineScriptBinding::SetVertexShader (const String &shaderFile, EShaderVersion version, const String &defines)
	{
		CHECK( not vertex.IsDefined() );
		vertex.filename	= StringToWString( shaderFile );
		vertex.type		= EShader::Vertex;
		vertex.version	= version;
		SetDefines( OUT vertex.defines, defines );
	}
	
/*
=================================================
	SetTessControlShader
=================================================
*/
	void GraphicsPipelineScriptBinding::SetTessControlShader (const String &shaderFile, EShaderVersion version, const String &defines)
	{
		CHECK( not tessControl.IsDefined() );
		tessControl.filename	= StringToWString( shaderFile );
		tessControl.type		= EShader::TessControl;
		tessControl.version		= version;
		SetDefines( OUT tessControl.defines, defines );
	}
	
/*
=================================================
	SetTessEvalShader
=================================================
*/
	void GraphicsPipelineScriptBinding::SetTessEvalShader (const String &shaderFile, EShaderVersion version, const String &defines)
	{
		CHECK( not tessEval.IsDefined() );
		tessEval.filename	= StringToWString( shaderFile );
		tessEval.type		= EShader::TessEvaluation;
		tessEval.version	= version;
		SetDefines( OUT tessEval.defines, defines );
	}
	
/*
=================================================
	SetGeometryShader
=================================================
*/
	void GraphicsPipelineScriptBinding::SetGeometryShader (const String &shaderFile, EShaderVersion version, const String &defines)
	{
		CHECK( not geometry.IsDefined() );
		geometry.filename	= StringToWString( shaderFile );
		geometry.type		= EShader::Geometry;
		geometry.version	= version;
		SetDefines( OUT geometry.defines, defines );
	}
	
/*
=================================================
	SetFragmentShader
=================================================
*/
	void GraphicsPipelineScriptBinding::SetFragmentShader (const String &shaderFile, EShaderVersion version, const String &defines)
	{
		CHECK( not fragment.IsDefined() );
		fragment.filename	= StringToWString( shaderFile );
		fragment.type		= EShader::Fragment;
		fragment.version	= version;
		SetDefines( OUT fragment.defines, defines );
	}
	
/*
=================================================
	MergePass1
=================================================
*/
	bool GraphicsPipelineScriptBinding::MergePass1 (INOUT size_t &merged)
	{
		size_t	count;
		CHECK_ERR( AddLayout( vertex,      OUT count ));	merged += count;
		CHECK_ERR( AddLayout( tessControl, OUT count ));	merged += count;
		CHECK_ERR( AddLayout( tessEval,    OUT count ));	merged += count;
		CHECK_ERR( AddLayout( geometry,    OUT count ));	merged += count;
		CHECK_ERR( AddLayout( fragment,    OUT count ));	merged += count;
		return true;
	}
	
/*
=================================================
	MergePass2
=================================================
*/
	bool GraphicsPipelineScriptBinding::MergePass2 (INOUT size_t &merged) const
	{
		size_t	count;
		CHECK_ERR( MergeLayouts( vertex,      OUT count ));	merged += count;
		CHECK_ERR( MergeLayouts( tessControl, OUT count ));	merged += count;
		CHECK_ERR( MergeLayouts( tessEval,    OUT count ));	merged += count;
		CHECK_ERR( MergeLayouts( geometry,    OUT count ));	merged += count;
		CHECK_ERR( MergeLayouts( fragment,    OUT count ));	merged += count;
		return true;
	}
	
/*
=================================================
	Build
=================================================
*/
	bool GraphicsPipelineScriptBinding::Build ()
	{
		auto&					shader_storage	= *ShaderStorage::Instance();
		auto&					ppln_storage	= *shader_storage.storage;
		GraphicsPipelineDesc	desc;

		desc.layout			= BuildLayout();
		desc.specialization	= _specValues;

		CHECK_ERR( vertex.IsDefined() );
		CHECK_ERR( fragment.IsDefined() );

		if ( vertex.IsDefined() )
		{
			auto	iter = shader_storage.uniqueShaders.find( vertex );
			CHECK_ERR( iter != shader_storage.uniqueShaders.end() );

			desc.supportedTopology	= iter->second.reflection.vertex.supportedTopology;
			desc.vertexAttribs		= iter->second.reflection.vertex.vertexAttribs;
			desc.shaders.insert_or_assign( EShader::Vertex, iter->second.uid );
		}

		if ( tessControl.IsDefined() )
		{
			auto	iter = shader_storage.uniqueShaders.find( tessControl );
			CHECK_ERR( iter != shader_storage.uniqueShaders.end() );

			desc.patchControlPoints	= iter->second.reflection.tessellation.patchControlPoints;
			desc.shaders.insert_or_assign( EShader::TessControl, iter->second.uid );
		}

		if ( tessEval.IsDefined() )
		{
			auto	iter = shader_storage.uniqueShaders.find( tessEval );
			CHECK_ERR( iter != shader_storage.uniqueShaders.end() );
			
			desc.shaders.insert_or_assign( EShader::TessEvaluation, iter->second.uid );
		}

		if ( geometry.IsDefined() )
		{
			auto	iter = shader_storage.uniqueShaders.find( geometry );
			CHECK_ERR( iter != shader_storage.uniqueShaders.end() );
			
			desc.shaders.insert_or_assign( EShader::Geometry, iter->second.uid );
		}

		if ( fragment.IsDefined() )
		{
			auto	iter = shader_storage.uniqueShaders.find( fragment );
			CHECK_ERR( iter != shader_storage.uniqueShaders.end() );

			desc.fragmentOutputs	= iter->second.reflection.fragment.fragmentOutput;
			desc.earlyFragmentTests	= iter->second.reflection.fragment.earlyFragmentTests;
			desc.shaders.insert_or_assign( EShader::Fragment, iter->second.uid );
		}

		Unused( ppln_storage.AddPipeline( _name, std::move(desc) ));
		return true;
	}
//-----------------------------------------------------------------------------
	

	
/*
=================================================
	constructor
=================================================
*/
	MeshPipelineScriptBinding::MeshPipelineScriptBinding ()
	{
		ShaderStorage::Instance()->mpipelines.push_back( MeshPipelinePtr{this} );
	}
	
/*
=================================================
	SetTaskShader
=================================================
*/
	void MeshPipelineScriptBinding::SetTaskShader (const String &shaderFile, EShaderVersion version, const String &defines)
	{
		CHECK( not task.IsDefined() );
		task.filename	= StringToWString( shaderFile );
		task.type		= EShader::MeshTask;
		task.version	= version;
		SetDefines( OUT task.defines, defines );
	}
	
/*
=================================================
	SetMeshShader
=================================================
*/
	void MeshPipelineScriptBinding::SetMeshShader (const String &shaderFile, EShaderVersion version, const String &defines)
	{
		CHECK( not mesh.IsDefined() );
		mesh.filename	= StringToWString( shaderFile );
		mesh.type		= EShader::Mesh;
		mesh.version	= version;
		SetDefines( OUT mesh.defines, defines );
	}
	
/*
=================================================
	SetFragmentShader
=================================================
*/
	void MeshPipelineScriptBinding::SetFragmentShader (const String &shaderFile, EShaderVersion version, const String &defines)
	{
		CHECK( not fragment.IsDefined() );
		fragment.filename	= StringToWString( shaderFile );
		fragment.type		= EShader::Fragment;
		fragment.version	= version;
		SetDefines( OUT fragment.defines, defines );
	}
	
/*
=================================================
	MergePass1
=================================================
*/
	bool MeshPipelineScriptBinding::MergePass1 (INOUT size_t &merged)
	{
		size_t	count;
		CHECK_ERR( AddLayout( task,     OUT count ));	merged += count;
		CHECK_ERR( AddLayout( mesh,     OUT count ));	merged += count;
		CHECK_ERR( AddLayout( fragment, OUT count ));	merged += count;
		return true;
	}
	
/*
=================================================
	MergePass2
=================================================
*/
	bool MeshPipelineScriptBinding::MergePass2 (INOUT size_t &merged) const
	{
		size_t	count;
		CHECK_ERR( MergeLayouts( task,     OUT count ));	merged += count;
		CHECK_ERR( MergeLayouts( mesh,     OUT count ));	merged += count;
		CHECK_ERR( MergeLayouts( fragment, OUT count ));	merged += count;
		return true;
	}
	
/*
=================================================
	Build
=================================================
*/
	bool MeshPipelineScriptBinding::Build ()
	{
		auto&				shader_storage	= *ShaderStorage::Instance();
		auto&				ppln_storage	= *shader_storage.storage;
		MeshPipelineDesc	desc;

		desc.layout			= BuildLayout();
		desc.specialization	= _specValues;

		CHECK_ERR( mesh.IsDefined() );
		CHECK_ERR( fragment.IsDefined() );

		if ( task.IsDefined() )
		{
			auto	iter = shader_storage.uniqueShaders.find( task );
			CHECK_ERR( iter != shader_storage.uniqueShaders.end() );

			desc.defaultTaskGroupSize	= iter->second.reflection.mesh.taskGroupSize;
			desc.taskSizeSpec			= iter->second.reflection.mesh.taskGroupSpecialization;
			desc.shaders.insert_or_assign( EShader::MeshTask, iter->second.uid );
		}

		if ( mesh.IsDefined() )
		{
			auto	iter = shader_storage.uniqueShaders.find( mesh );
			CHECK_ERR( iter != shader_storage.uniqueShaders.end() );
			
			desc.topology				= iter->second.reflection.mesh.topology;
			desc.maxVertices			= iter->second.reflection.mesh.maxVertices;
			desc.maxIndices				= iter->second.reflection.mesh.maxIndices;
			desc.defaultMeshGroupSize	= iter->second.reflection.mesh.meshGroupSize;
			desc.meshSizeSpec			= iter->second.reflection.mesh.meshGroupSpecialization;
			desc.shaders.insert_or_assign( EShader::Mesh, iter->second.uid );
		}

		if ( fragment.IsDefined() )
		{
			auto	iter = shader_storage.uniqueShaders.find( fragment );
			CHECK_ERR( iter != shader_storage.uniqueShaders.end() );

			desc.fragmentOutputs	= iter->second.reflection.fragment.fragmentOutput;
			desc.earlyFragmentTests	= iter->second.reflection.fragment.earlyFragmentTests;
			desc.shaders.insert_or_assign( EShader::Fragment, iter->second.uid );
		}

		Unused( ppln_storage.AddPipeline( _name, std::move(desc) ));
		return true;
	}
//-----------------------------------------------------------------------------
	

	
/*
=================================================
	constructor
=================================================
*/
	ComputePipelineScriptBinding::ComputePipelineScriptBinding ()
	{
		ShaderStorage::Instance()->cpipelines.push_back( ComputePipelinePtr{this} );
	}
	
/*
=================================================
	SetShader
=================================================
*/
	void ComputePipelineScriptBinding::SetShader (const String &shaderFile, EShaderVersion version, const String &defines)
	{
		CHECK( not shader.IsDefined() );
		shader.filename	= StringToWString( shaderFile );
		shader.type		= EShader::Compute;
		shader.version	= version;
		SetDefines( OUT shader.defines, defines );
	}
	
/*
=================================================
	MergePass1
=================================================
*/
	bool ComputePipelineScriptBinding::MergePass1 (INOUT size_t &merged)
	{
		size_t	count;
		CHECK_ERR( AddLayout( shader, OUT count ));	merged += count;
		return true;
	}
	
/*
=================================================
	MergePass2
=================================================
*/
	bool ComputePipelineScriptBinding::MergePass2 (INOUT size_t &merged) const
	{
		size_t	count;
		CHECK_ERR( MergeLayouts( shader, OUT count ));	merged += count;
		return true;
	}
	
/*
=================================================
	Build
=================================================
*/
	bool ComputePipelineScriptBinding::Build ()
	{
		auto&				shader_storage	= *ShaderStorage::Instance();
		auto&				ppln_storage	= *shader_storage.storage;
		ComputePipelineDesc	desc;

		desc.layout			= BuildLayout();
		desc.specialization	= _specValues;

		CHECK_ERR( shader.IsDefined() );
		{
			auto	iter = shader_storage.uniqueShaders.find( shader );
			CHECK_ERR( iter != shader_storage.uniqueShaders.end() );
			
			desc.defaultLocalGroupSize	= iter->second.reflection.compute.localGroupSize;
			desc.localSizeSpec			= iter->second.reflection.compute.localGroupSpecialization;
			desc.shader					= iter->second.uid;
		}

		Unused( ppln_storage.AddPipeline( _name, std::move(desc) ));
		return true;
	}
//-----------------------------------------------------------------------------
	

	
/*
=================================================
	constructor
=================================================
*/
	ShaderStorage::ShaderStorage ()
	{
		gpipelines.reserve( 128 );
		mpipelines.reserve( 128 );
		cpipelines.reserve( 128 );
	}
	
/*
=================================================
	_AddShader
=================================================
*/
	void ShaderStorage::_AddShader (INOUT ShaderInfo &info, const ShaderDefines_t &pplnDefines)
	{
		if ( not info.IsDefined() )
			return;

		// validate
		info.filename	= _FindShader( info.filename );
		info.version	= info.version == Default ? EShaderVersion::Spirv_120 : info.version;

		for (auto& def : pplnDefines)
		{
			CHECK( info.defines.size() < info.defines.capacity() );
			info.defines.push_back( def );
		}
		SortDefines( INOUT info.defines );
		// TODO: remove duplicates

		uniqueShaders.insert({ info, {} });
	}
		
/*
=================================================
	_FindShader
=================================================
*/
	Filename  ShaderStorage::_FindShader (const Filename &path) const
	{
		for (auto& folder : shaderFolders)
		{
			Path	sh_path = Path{ folder }.append( path );

			if ( FileSystem::Exists( sh_path ))
				return sh_path.wstring();
		}

		AE_LOGI( "Can't find shader: '"s << Path{path}.string() << "'" );
		return L"";
	}
	
/*
=================================================
	CacheShaders
=================================================
*/
	bool ShaderStorage::CacheShaders ()
	{
		for (auto& ppln : gpipelines)
		{
			_AddShader( INOUT ppln->vertex,      ppln->_defines );
			_AddShader( INOUT ppln->tessControl, ppln->_defines );
			_AddShader( INOUT ppln->tessEval,    ppln->_defines );
			_AddShader( INOUT ppln->geometry,    ppln->_defines );
			_AddShader( INOUT ppln->fragment,    ppln->_defines );
		}

		for (auto& ppln : mpipelines)
		{
			_AddShader( INOUT ppln->task,     ppln->_defines );
			_AddShader( INOUT ppln->mesh,     ppln->_defines );
			_AddShader( INOUT ppln->fragment, ppln->_defines );
		}

		for (auto& ppln : cpipelines)
		{
			_AddShader( INOUT ppln->shader, ppln->_defines );
		}
		return true;
	}
	
/*
=================================================
	MergePipelines
=================================================
*/
	bool ShaderStorage::MergePipelines ()
	{
		// graphics
		for (;;)
		{
			size_t	counter1 = 0;
			size_t	counter2 = 0;

			for (auto& ppln : gpipelines)
			{
				CHECK_ERR( ppln->MergePass1( INOUT counter1 ));
			}
			for (auto& ppln : mpipelines)
			{
				CHECK_ERR( ppln->MergePass1( INOUT counter1 ));
			}
			if ( counter1 == 0 )
				break;

			for (auto& ppln : gpipelines)
			{
				CHECK_ERR( ppln->MergePass2( INOUT counter2 ));
			}
			for (auto& ppln : mpipelines)
			{
				CHECK_ERR( ppln->MergePass2( INOUT counter2 ));
			}
			if ( counter2 == 0 )
				break;
		}
		
		// compute
		for (;;)
		{
			size_t	counter1 = 0;
			size_t	counter2 = 0;

			for (auto& ppln : cpipelines)
			{
				CHECK_ERR( ppln->MergePass1( INOUT counter1 ));
			}
			if ( counter1 == 0 )
				break;
			
			for (auto& ppln : cpipelines)
			{
				CHECK_ERR( ppln->MergePass2( INOUT counter2 ));
			}
			if ( counter2 == 0 )
				break;
		}

		// ray tracing
		{
			// TODO
		}

		return true;
	}
	
/*
=================================================
	BuildPipelines
=================================================
*/
	bool ShaderStorage::BuildPipelines ()
	{
		for (auto& ppln : gpipelines) {
			CHECK_ERR( ppln->Build() );
		}

		for (auto& ppln : mpipelines) {
			CHECK_ERR( ppln->Build() );
		}

		for (auto& ppln : cpipelines) {
			CHECK_ERR( ppln->Build() );
		}
		return true;
	}
	
/*
=================================================
	ShaderStorage_Instance
=================================================
*/
	ShaderStorage*&  ShaderStorage_Instance ()
	{
		static thread_local ShaderStorage*	inst = null;
		return inst;
	}

	ShaderStorage*  ShaderStorage::Instance ()
	{
		return ShaderStorage_Instance();
	}
	
	void  ShaderStorage::SetInstance (ShaderStorage* inst)
	{
		ShaderStorage_Instance() = inst;
	}


}	// AE::PipelineCompiler
