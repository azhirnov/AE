// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "Public/PipelinePack.h"
#include "stl/Containers/NtStringView.h"
#include "glslang/Include/ResourceLimits.h"

class TIntermNode;

namespace glslang
{
	class TType;
	class TIntermediate;
}

namespace AE::PipelineCompiler
{

	//
	// SPIRV Compiler
	//

	class SpirvCompiler final
	{
	// types
	public:
		struct ShaderReflection
		{
		// types
			using VertexAttrib		= VertexInputState::VertexAttrib;
			using VertexAttribs_t	= GraphicsPipelineDesc::VertexAttribs_t;
			using FragmentOutputs_t	= RenderPassInfo::FragmentOutputs_t;
			using TopologyBits_t	= GraphicsPipelineDesc::TopologyBits_t;
			using SpecConstants_t	= SpirvShaderCode::SpecConstants_t;

			struct DescriptorSet {
				uint						bindingIndex	= UMax;
				DescriptorSetName			name;
				DescriptorSetLayoutDesc		layout;
			};
			using DescSets_t		= FixedArray< DescriptorSet, PipelineLayoutDesc::DescrSetMap_t::capacity() >;


		// variables
			struct {
				DescSets_t			descrSets;
				PushConstants		pushConstants;
				SpecConstants_t		specConstants;
			}					layout;

			struct {
				TopologyBits_t		supportedTopology;
				VertexAttribs_t		vertexAttribs;
			}					vertex;

			struct {
				uint				patchControlPoints;
			}					tessellation;

			struct {
				FragmentOutputs_t	fragmentOutput;
				bool				earlyFragmentTests	= true;
			}					fragment;

			struct {
				uint3				localGroupSize;
				uint3				localGroupSpecialization;
			}					compute;

			struct {
				uint3				taskGroupSize;
				uint3				taskGroupSpecialization;

				uint3				meshGroupSize;
				uint3				meshGroupSpecialization;
				EPrimitive			topology		= Default;
				uint				maxPrimitives	= 0;
				uint				maxIndices		= 0;
				uint				maxVertices		= 0;
			}					mesh;
		};

	private:
		struct GLSLangResult;
		class  ShaderIncluder;
		
		using EDescriptorType	= DescriptorSetLayoutDesc::EDescriptorType;
		using PushConstant		= PushConstants::PushConst;
		using FragmentOutput	= RenderPassInfo::FragmentOutput;

		using IncludeDirs_t		= Array< std::wstring > const &;


	// variables
	private:
		IncludeDirs_t				_directories;

		glslang::TIntermediate *	_intermediate	= null;
		EShaderStages				_currentStage	= Default;
		uint						_spirvTraget	= 0;		// spv_target_env

		TBuiltInResource			_builtinResource;

		static constexpr bool		_quietWarnings		= true;
		static constexpr bool		_parseAnnotations	= true;
		static constexpr size_t		_maxDescrSets		= PipelineLayoutDesc::DescrSetMap_t::capacity();


	// methods
	public:
		explicit SpirvCompiler (IncludeDirs_t includeDirs);
		~SpirvCompiler ();
		
		bool SetDefaultResourceLimits ();
		
		bool BuildReflection (EShader shaderType, uint spirvVersion, NtStringView entry, NtStringView source, OUT ShaderReflection &outReflection, OUT String &log);
		bool Compile (EShader shaderType, uint spirvVersion, NtStringView entry, NtStringView source, OUT Array<uint> &outSpirv, OUT String &log);
		

	private:
		bool _ParseGLSL (EShader shaderType, uint spirvVersion, NtStringView entry, ArrayView<const char *> source,
						 INOUT ShaderIncluder &includer, OUT GLSLangResult &glslangData, INOUT String &log);

		bool _CompileSPIRV (const GLSLangResult &glslangData, OUT Array<uint> &spirv, INOUT String &log) const;
		bool _OptimizeSPIRV (INOUT Array<uint> &spirv, INOUT String &log) const;

		bool _BuildReflection (const GLSLangResult &glslangData, OUT ShaderReflection &reflection);

		bool _OnCompilationFailed (ArrayView<const char *> source, INOUT String &log) const;

		bool _ParseAnnotations (StringView source, INOUT ShaderReflection &) const;

		static void _GenerateResources (OUT TBuiltInResource& res);


	// GLSL deserializer
	private:
		bool _ProcessExternalObjects (TIntermNode* root, TIntermNode* node, INOUT ShaderReflection &result) const;

		bool _DeserializeExternalObjects (TIntermNode* node, INOUT ShaderReflection &result) const;

		bool _ProcessShaderInfo (INOUT ShaderReflection &result) const;
		
		bool _CalculateStructSize (const glslang::TType &bufferType, OUT BytesU &staticSize, OUT BytesU &arrayStride, OUT BytesU &offset) const;
		
		void _MergeWithGeometryInputPrimitive (INOUT GraphicsPipelineDesc::TopologyBits_t &topologyBits, /*TLayoutGeometry*/uint type) const;

		ND_ EImageType		_ExtractImageType (const glslang::TType &type) const;
		ND_ EVertexType		_ExtractVertexType (const glslang::TType &type) const;
		ND_ EPixelFormat	_ExtractImageFormat (/*TLayoutFormat*/uint format) const;
		ND_ EFragOutput		_ExtractFragmentOutputType (const glslang::TType &type) const;
	};


}	// AE::PipelineCompiler
