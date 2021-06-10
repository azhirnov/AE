// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'
/*
	Graphics_Config constants is not used here because they may be different for different platforms.
*/

#pragma once

#include "serializing/ISerializable.h"
#include "graphics/Public/IDs.h"
#include "graphics/Public/EResourceState.h"
#include "graphics/Public/ShaderEnums.h"
#include "graphics/Public/RenderStateEnums.h"
#include "graphics/Public/ResourceEnums.h"
#include "graphics/Public/VertexInputState.h"
#include "graphics/Public/DescriptorSet.h"

namespace AE::PipelineCompiler
{
	using namespace AE::Graphics;
	
	enum class DescrSetUID		 : uint { Unknown = ~0u };
	enum class PipelineLayoutUID : uint { Unknown = ~0u };
	enum class RenderPassUID	 : uint { Unknown = ~0u };


	enum class ShaderUID : uint
	{
		SPIRV		= 1u << 28,
		_Mask		= 7u << 28,
		Unknown		= ~0u
	};
	AE_BIT_OPERATORS( ShaderUID );


	enum class PipelineUID : uint
	{
		Graphics		= 1u << 28,
		Mesh			= 2u << 28,
		Compute			= 3u << 28,
		RayTracing		= 4u << 28,
		_Mask			= 7u << 28,
		Unknown			= ~0u
	};
	AE_BIT_OPERATORS( PipelineUID );


	enum class EImageType : uint
	{
		Unknown				= 0,

		_TexMask			= 0xF,
		Img1D				= 1,
		Img1DArray			= 2,
		Img2D				= 3,
		Img2DArray			= 4,
		Img2DMS				= 5,
		Img2DMSArray		= 6,
		ImgCube				= 7,
		ImgCubeArray		= 8,
		Img3D				= 9,
		
		_ValMask			= 0xF << 4,
		Float				= 1 << 4,
		//SNorm				= 2 << 4,
		//UNorm				= 3 << 4,
		Int					= 5 << 4,
		UInt				= 6 << 4,
		
		_QualMask			= 0xF << 8,
		Shadow				= 1 << 8,
		SRGB				= 2 << 9,

		// default
		Sampler1D				= Img1D			| Float,
		Sampler2D				= Img2D			| Float,
		Sampler3D				= Img3D			| Float,
		Sampler1DArray			= Img1DArray	| Float,
		Sampler2DArray			= Img2DArray	| Float,
		SamplerCube				= ImgCube		| Float,
		SamplerCubeArray		= ImgCubeArray	| Float,
		Sampler2DMS				= Img2DMS		| Float,
		Sampler2DMSArray		= Img2DMSArray	| Float,
		
		Sampler1DShadow			= Img1D			| Float | Shadow,
		Sampler2DShadow			= Img2D			| Float | Shadow,
		Sampler1DArrayShadow	= Img1DArray	| Float | Shadow,
		Sampler2DArrayShadow	= Img2DArray	| Float | Shadow,
		SamplerCubeShadow		= ImgCube		| Float | Shadow,
		SamplerCubeArrayShadow	= ImgCubeArray	| Float | Shadow,

		ISampler1D				= Img1D			| Int,
		ISampler2D				= Img2D			| Int,
		ISampler3D				= Img3D			| Int,
		ISampler1DArray			= Img1DArray	| Int,
		ISampler2DArray			= Img2DArray	| Int,
		ISamplerCube			= ImgCube		| Int,
		ISamplerCubeArray		= ImgCubeArray	| Int,
		ISampler2DMS			= Img2DMS		| Int,
		ISampler2DMSArray		= Img2DMSArray	| Int,

		USampler1D				= Img1D			| UInt,
		USampler2D				= Img2D			| UInt,
		USampler3D				= Img3D			| UInt,
		USampler1DArray			= Img1DArray	| UInt,
		USampler2DArray			= Img2DArray	| UInt,
		USamplerCube			= ImgCube		| UInt,
		USamplerCubeArray		= ImgCubeArray	| UInt,
		USampler2DMS			= Img2DMS		| UInt,
		USampler2DMSArray		= Img2DMSArray	| UInt,
	};
	AE_BIT_OPERATORS( EImageType );



	//
	// Descriptor Set Layout description
	//

	class DescriptorSetLayoutDesc final : public Serializing::ISerializable
	{
	// types
	public:
		using EDescriptorType	= DescriptorSet::EDescriptorType;
		using BindingIndex_t	= DescriptorSet::BindingIndex_t;
		using ArraySize_t		= DescriptorSet::ArraySize_t;
		
		struct Buffer
		{
			EResourceState		state;
			uint				dynamicOffsetIndex;
			Bytes				staticSize;
			Bytes				arrayStride;
		};

		struct TexelBuffer
		{
			EResourceState		state;
		};

		struct Image
		{
			EResourceState		state;
			EImageType			type;
		};

		struct Sampler
		{
			EShaderStages		stageFlags;
		};

		struct ImmutableSampler : Sampler
		{
			usize				offsetInStorage;		// in 'samplerStorage'
		};

		struct ImageWithSampler
		{
			Image				image;
			usize				samplerOffsetInStorage;	// in 'samplerStorage'
		};

		struct SubpassInput : Image
		{
			// TODO
		};

		struct RayTracingScene
		{};

		struct Uniform
		{
			EDescriptorType		type	= Default;
			BindingIndex_t		index;
			ArraySize_t			arraySize;				// must be > 0
			union {
				Buffer				buffer;
				TexelBuffer			texelBuffer;
				Image				storageImage;
				Image				sampledImage;
				Image				combinedImage;
				ImageWithSampler	combinedImageWithSampler;
				SubpassInput		subpassInput;
				Sampler				sampler;
				ImmutableSampler	immutableSampler;
			};

			Uniform () {}
		};

		using Uniforms_t	= Array<Pair< UniformName, Uniform >>;
		using Samplers_t	= Array< SamplerName >;


	// variables
	public:
		Uniforms_t		uniforms;
		Samplers_t		samplerStorage;


	// methods
	public:
		DescriptorSetLayoutDesc () {}

		bool Merge (const DescriptorSetLayoutDesc &other, OUT usize &merged);
		void SortUniforms ();

		// ISerializable
		bool Serialize (Serializing::Serializer &) const override;
		bool Deserialize (Serializing::Deserializer const&) override;
	};



	//
	// Push Constants
	//

	class PushConstants final : public Serializing::ISerializable
	{
	// types
	public:
		struct PushConst
		{
			EShaderStages		stageFlags;
			TBytes<ushort>		offset;
			TBytes<ushort>		size;

			PushConst () {}
			PushConst (EShaderStages stages, Bytes offset, Bytes size) : stageFlags{stages}, offset{offset}, size{size} {}
		};
		using PushConstMap_t = FixedMap< PushConstantName, PushConst, 8 >;


	// variables
	public:
		PushConstMap_t		items;


	// methods
	public:
		PushConstants () {}

		bool Merge (const PushConstants &);

		// ISerializable
		bool Serialize (Serializing::Serializer &) const override;
		bool Deserialize (Serializing::Deserializer const&) override;
	};



	//
	// Pipeline Layout description
	//

	class PipelineLayoutDesc final : public Serializing::ISerializable
	{
	// types
	public:
		struct DescSetLayout
		{
			DescrSetUID		uid;
			uint			index;
		};
		using DescrSetMap_t = FixedMap< DescriptorSetName, DescSetLayout, 8 >;


	// variables
	public:
		DescrSetMap_t		descrSets;
		PushConstants		pushConstants;


	// methods
	public:
		PipelineLayoutDesc () {}

		// ISerializable
		bool Serialize (Serializing::Serializer &) const override;
		bool Deserialize (Serializing::Deserializer const&) override;
	};



	//
	// Graphics Pipeline description
	//

	class GraphicsPipelineDesc final : public Serializing::ISerializable
	{
	// types
	public:
		using Shaders_t			= FixedMap< EShader, ShaderUID, 8 >;	// TODO: shader alternatives
		using TopologyBits_t	= BitSet< uint(EPrimitive::_Count) >;
		using VertexAttrib		= VertexInputState::VertexAttrib;
		using VertexAttribs_t	= FixedArray< VertexAttrib, 16 >;
		using SpecValues_t		= FixedMap< SpecializationName, /*bool/int/uint/float*/uint, 8 >;


	// variables
	public:
		PipelineLayoutUID		layout					= Default;
		RenderPassUID			renderPass				= Default;
		Shaders_t				shaders;
		TopologyBits_t			supportedTopology;
		VertexAttribs_t			vertexAttribs;
		SpecValues_t			specialization;
		uint					patchControlPoints		= 0;
		bool					earlyFragmentTests		= true;


	// methods
	public:
		GraphicsPipelineDesc () {}

		// ISerializable
		bool Serialize (Serializing::Serializer &) const override;
		bool Deserialize (Serializing::Deserializer const&) override;
	};



	//
	// Compute Pipeline description
	//

	class ComputePipelineDesc final : public Serializing::ISerializable
	{
	// types
	public:
		using SpecValues_t	= GraphicsPipelineDesc::SpecValues_t;


	// variables
	public:
		static constexpr uint	UNDEFINED_SPECIALIZATION = UMax;
		
		PipelineLayoutUID		layout					= Default;
		ShaderUID				shader					= Default;
		uint3					defaultLocalGroupSize;
		uint3					localSizeSpec			{UNDEFINED_SPECIALIZATION};
		SpecValues_t			specialization;


	// methods
	public:
		ComputePipelineDesc () {}
		
		// ISerializable
		bool Serialize (Serializing::Serializer &) const override;
		bool Deserialize (Serializing::Deserializer const&) override;
	};



	//
	// Mesh Pipeline description
	//

	class MeshPipelineDesc final : public Serializing::ISerializable
	{
	// types
	public:
		using Shaders_t			= FixedMap< EShader, ShaderUID, 4 >;	// TODO: shader alternatives
		using SpecValues_t		= GraphicsPipelineDesc::SpecValues_t;


	// variables
	public:
		static constexpr uint	UNDEFINED_SPECIALIZATION = UMax;
		
		PipelineLayoutUID		layout					= Default;
		RenderPassUID			renderPass				= Default;
		Shaders_t				shaders;
		EPrimitive				topology				= Default;
		uint					maxVertices				= 0;
		uint					maxIndices				= 0;
		uint					defaultTaskGroupSize	= 0;
		uint					taskSizeSpec			= UNDEFINED_SPECIALIZATION;
		uint					defaultMeshGroupSize	= 0;
		uint					meshSizeSpec			= UNDEFINED_SPECIALIZATION;
		SpecValues_t			specialization;
		bool					earlyFragmentTests		= true;
		

	// methods
	public:
		MeshPipelineDesc () {}
		
		// ISerializable
		bool Serialize (Serializing::Serializer &) const override;
		bool Deserialize (Serializing::Deserializer const&) override;
	};

	

	//
	// SPIRV Shader Code
	//

	class SpirvShaderCode final : public Serializing::ISerializable
	{
	// types
	public:
		using SpecConstants_t = FixedMap< SpecializationName, uint, 8 >;

	// variables
	public:
		uint				version = 0;
		Array<uint>			code;
		SpecConstants_t		spec;


	// methods
	public:
		SpirvShaderCode () {}
		SpirvShaderCode (uint ver, Array<uint> &&code, const SpecConstants_t &spec) : version{ver}, code{std::move(code)}, spec{spec} {}

		// ISerializable
		bool Serialize (Serializing::Serializer &) const override;
		bool Deserialize (Serializing::Deserializer const&) override;
	};



	//
	// Render Pass Info
	//
	
	class RenderPassInfo final : public Serializing::ISerializable
	{
	// types
	public:
		struct FragmentOutput
		{
			uint			index	= UMax;
			EFragOutput		type	= Default;

			bool operator == (const FragmentOutput &rhs) const	{ return (index == rhs.index) & (type == rhs.type); }
		};
		using FragmentOutputs_t	= FixedMap< RenderTargetName, FragmentOutput, 8 >;


	// variables
	public:
		FragmentOutputs_t	fragmentOutputs;


	// methods
	public:
		RenderPassInfo () {}

		// ISerializable
		bool Serialize (Serializing::Serializer &) const override;
		bool Deserialize (Serializing::Deserializer const&) override;
	};



	//
	// Pipeline Storage
	//

	class PipelineStorage final : public Serializing::ISerializable
	{
	// types
	private:
		using Hash_t					= usize;

		using DescriptorSetLayouts_t	= Array< DescriptorSetLayoutDesc >;
		using DSLayoutMap_t				= HashMultiMap< Hash_t, DescrSetUID >;

		using PipelineLayouts_t			= Array< PipelineLayoutDesc >;
		using PipelineLayoutMap_t		= HashMultiMap< Hash_t, PipelineLayoutUID >;

		using GraphicsPipelines_t		= Array< GraphicsPipelineDesc >;
		using GraphicsPipelineMap_t		= HashMultiMap< Hash_t, PipelineUID >;
		
		using MeshPipelines_t			= Array< MeshPipelineDesc >;
		using MeshPipelineDescMap_t		= HashMultiMap< Hash_t, PipelineUID >;

		using ComputePipelines_t		= Array< ComputePipelineDesc >;
		using ComputePipelineMap_t		= HashMultiMap< Hash_t, PipelineUID >;

		using SpirvShaders_t			= Array< SpirvShaderCode >;
		using SpirvShaderMap_t			= HashMultiMap< Hash_t, ShaderUID >;
		using SpecConstants_t			= SpirvShaderCode::SpecConstants_t;
		
		using RenderPasses_t			= Array< RenderPassInfo >;
		using RenderPassMap_t			= HashMap< RenderPassName, RenderPassUID >;

		using PipelineMap_t				= HashMap< PipelineName, PipelineUID >;

	public:
		enum class EMarker : uint
		{
			DescrSetLayouts		= 1 << 5,
			PipelineLayouts		= 2 << 5,
			RenderPasses		= 3 << 5,
			RenderPassNames		= 4 << 5,
			
			PipelineNames		= 1 << 10,
			GraphicsPipelines	= 2 << 10,
			MeshPipelines		= 3 << 10,
			ComputePipelines	= 4 << 10,
			RayTracingPipelines	= 5 << 10,

			SpirvShaders		= 1 << 15,
			GlslShaders			= 2 << 15,
		};

		static constexpr uint	Version = 1;


	// variables
	private:
		DescriptorSetLayouts_t		_dsLayouts;
		DSLayoutMap_t				_dsLayoutMap;

		PipelineLayouts_t			_pplnLayouts;
		PipelineLayoutMap_t			_pplnLayoutMap;

		GraphicsPipelines_t			_graphicsPipelines;
		GraphicsPipelineMap_t		_gpipelineMap;
		
		MeshPipelines_t				_meshPipelines;
		MeshPipelineDescMap_t		_mpipelineMap;

		ComputePipelines_t			_computePipelines;
		ComputePipelineMap_t		_cpipelineMap;

		PipelineMap_t				_pipelineMap;

		RenderPasses_t				_renderPasses;
		RenderPassMap_t				_renderPassMap;

		SpirvShaders_t				_spirvShaders;
		SpirvShaderMap_t			_spirvShaderMap;


	// methods
	public:
		PipelineStorage ();

		ND_ DescrSetUID			AddDescriptorSetLayout (DescriptorSetLayoutDesc &&);
		ND_ PipelineLayoutUID	AddPipelineLayout (PipelineLayoutDesc &&);
		ND_ PipelineUID			AddPipeline (const PipelineName &name, GraphicsPipelineDesc &&);
		ND_ PipelineUID			AddPipeline (const PipelineName &name, MeshPipelineDesc &&);
		ND_ PipelineUID			AddPipeline (const PipelineName &name, ComputePipelineDesc &&);
		ND_ ShaderUID			AddShader (uint version, Array<uint> &&spirv, const SpecConstants_t &spec);
		ND_ RenderPassUID		AddRenderPass (const RenderPassName &name, const RenderPassInfo &info);


		// ISerializable
		bool Serialize (Serializing::Serializer &) const override;
		bool Deserialize (Serializing::Deserializer const&) override { return false; }
	};


}	// AE::PipelineCompiler
