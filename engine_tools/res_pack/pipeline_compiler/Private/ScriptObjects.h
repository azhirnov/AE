// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "Public/PipelinePack.h"
#include "Private/SpirvCompiler.h"
#include "scripting/Impl/ScriptTypes.h"

namespace AE::PipelineCompiler
{
	using namespace AE::Scripting;

	using Filename			= WString;
	using ShaderDefines_t	= Array< String >;	// FixedArray< String, 16 >
	

	enum class EShaderVersion : uint
	{
		_SPIRV		= 1 << 28,
		_Mask		= 0xFu << 28,
		
		Spirv_100	= 100 | _SPIRV,		// Vulkan 1.0
		Spirv_110	= 110 | _SPIRV,
		Spirv_120	= 120 | _SPIRV,
		Spirv_130	= 130 | _SPIRV,		// Vulkan 1.1
		Spirv_140	= 140 | _SPIRV,		// Vulkan 1.1 extension

		Unknown		= 0,
	};


	//
	// Shader Info
	//
	struct ShaderInfo
	{
		Filename		filename;
		EShader			type		= Default;
		EShaderVersion	version		= Default;
		ShaderDefines_t	defines;
		//flags

		ND_ bool  operator == (const ShaderInfo &rhs) const;
		ND_ bool  IsDefined () const;
	};

	struct ShaderInfoHash
	{
		ND_ usize  operator () (const ShaderInfo &value) const;
	};



	//
	// Base Pipeline
	//
	struct BasePipeline : AngelScriptHelper::SimpleRefCounter
	{
		using DSLayouts_t		= StaticArray< DescriptorSetLayoutDesc, PipelineLayoutDesc::DescrSetMap_t::capacity() >;
		using DSLayoutNames_t	= StaticArray< DescriptorSetName, PipelineLayoutDesc::DescrSetMap_t::capacity() >;
		using SpecValues_t		= FixedMap< SpecializationName, uint, 8 >;

		PipelineName		_name;
		DSLayouts_t			_dsLayouts;
		DSLayoutNames_t		_dsLayoutNames;
		PushConstants		_pushConstants;
		Filename			_filename;
		SpecValues_t		_specValues;
		ShaderDefines_t		_defines;

		BasePipeline ();
		void SetName (const String &value);
		bool AddLayout (const ShaderInfo &sh, OUT usize &merged);
		bool MergeLayouts (const ShaderInfo &sh, OUT usize &merged) const;
		void Define (const String &value);
		ND_ PipelineLayoutUID BuildLayout ();
	};



	//
	// Graphics Pipeline
	//
	struct GraphicsPipelineScriptBinding : BasePipeline
	{
		String			rpName;
		ShaderInfo		vertex;
		ShaderInfo		tessControl;
		ShaderInfo		tessEval;
		ShaderInfo		geometry;
		ShaderInfo		fragment;

		GraphicsPipelineScriptBinding ();

		void SetVertexShader (const String &shaderFile, EShaderVersion version, const String &defines);
		void SetTessControlShader (const String &shaderFile, EShaderVersion version, const String &defines);
		void SetTessEvalShader (const String &shaderFile, EShaderVersion version, const String &defines);
		void SetGeometryShader (const String &shaderFile, EShaderVersion version, const String &defines);
		void SetFragmentShader (const String &shaderFile, EShaderVersion version, const String &defines);

		void SetRenderPass (const String &value)	{ rpName = value; }
		void SetName (const String &value)			{ BasePipeline::SetName( value ); }
		void Define (const String &value)			{ BasePipeline::Define( value ); }

		bool MergePass1 (INOUT usize &merged);
		bool MergePass2 (INOUT usize &merged) const;
		bool Build ();
	};
	using GraphicsPipelinePtr = AngelScriptHelper::SharedPtr< GraphicsPipelineScriptBinding >;



	//
	// Mesh Pipeline
	//
	struct MeshPipelineScriptBinding : BasePipeline
	{
		String			rpName;
		ShaderInfo		task;
		ShaderInfo		mesh;
		ShaderInfo		fragment;
		
		MeshPipelineScriptBinding ();

		void SetTaskShader (const String &shaderFile, EShaderVersion version, const String &defines);
		void SetMeshShader (const String &shaderFile, EShaderVersion version, const String &defines);
		void SetFragmentShader (const String &shaderFile, EShaderVersion version, const String &defines);
		
		void SetRenderPass (const String &value)	{ rpName = value; }
		void SetName (const String &value)			{ BasePipeline::SetName( value ); }
		void Define (const String &value)			{ BasePipeline::Define( value ); }

		bool MergePass1 (INOUT usize &merged);
		bool MergePass2 (INOUT usize &merged) const;
		bool Build ();
	};
	using MeshPipelinePtr = AngelScriptHelper::SharedPtr< MeshPipelineScriptBinding >;



	//
	// Compute Pipeline
	//
	struct ComputePipelineScriptBinding : BasePipeline
	{
		ShaderInfo		shader;
		
		ComputePipelineScriptBinding ();

		void SetShader (const String &shaderFile, EShaderVersion version, const String &defines);

		void SetName (const String &value)		{ BasePipeline::SetName( value ); }
		void Define (const String &value)		{ BasePipeline::Define( value ); }

		bool MergePass1 (INOUT usize &merged);
		bool MergePass2 (INOUT usize &merged) const;
		bool Build ();
	};
	using ComputePipelinePtr = AngelScriptHelper::SharedPtr< ComputePipelineScriptBinding >;

	

	//
	// Ray Tracing Pipeline
	//
	struct RayTracingPipelineSciptBinding : BasePipeline
	{
		// TODO
	};
	using RayTracingPipelinePtr = AngelScriptHelper::SharedPtr< RayTracingPipelineSciptBinding >;



	//
	// Render Pass
	//
	struct RenderPassScriptBinding : AngelScriptHelper::SimpleRefCounter
	{
		using FragmentOutput = RenderPassInfo::FragmentOutput;
		
		String				name;
		RenderPassInfo		info;
		String				source;

		RenderPassScriptBinding ();
		void SetName (const String &value);
		void SetSource (const String &value)								{ source = value; }
		void SetOutput (const String &id, uint index, EFragOutput value);
	};
	using RenderPassPtr = AngelScriptHelper::SharedPtr< RenderPassScriptBinding >;



	//
	// Shader Storage
	//
	struct ShaderStorage
	{
	// types
		struct ShaderData
		{
			ShaderUID						uid;
			String							source;
			SpirvCompiler::ShaderReflection	reflection;
			bool							compiled	= false;
		};
		using UniqueShaders_t	= std::unordered_map< ShaderInfo, ShaderData, ShaderInfoHash >;
		using GPipelines_t		= Array< GraphicsPipelinePtr >;
		using MPipelines_t		= Array< MeshPipelinePtr >;
		using CPipelines_t		= Array< ComputePipelinePtr >;
		using ShaderFolders_t	= Array< Filename >;
		using RenderPasses_t	= Array< RenderPassPtr >;
		using RenderPassMap_t	= HashMap< RenderPassName, RenderPassPtr >;


	// variables
		UniqueShaders_t		uniqueShaders;
		GPipelines_t		gpipelines;
		MPipelines_t		mpipelines;
		CPipelines_t		cpipelines;
		RenderPasses_t		renderPasses;
		RenderPassMap_t		_renderPassMap;

		// states
		Filename			pipelineFilename;
		
		// other
		ShaderFolders_t		shaderFolders;
		PipelineStorage*	storage		= null;

		NamedID_HashCollisionCheck	hashCollisionCheck;


	// methods
		ShaderStorage ();

		bool CacheShaders ();
		bool MergePipelines ();
		bool BuildPipelines ();

		ND_ static ShaderStorage*  Instance ();
		static void SetInstance (ShaderStorage* inst);
		
	private:
		void _AddShader (INOUT ShaderInfo &info, const ShaderDefines_t &pplnDefines, const String &renderPassSrc = Default);
		ND_ Filename  _FindShader (const Filename &path) const;
	};


}	// AE::PipelineCompiler
