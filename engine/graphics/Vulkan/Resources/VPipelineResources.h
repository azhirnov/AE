// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#if 0

#include "graphics/Public/PipelineResources.h"
#include "graphics/Public/Pipeline.h"
#include "VDescriptorSetLayout.h"

namespace AE::Graphics
{

	//
	// Vulkan Pipeline Resources
	//

	class VPipelineResources final
	{
	// types
	private:
		struct UpdateDescriptors
		{
			LinearAllocator<>			allocator;
			VkWriteDescriptorSet *		descriptors;
			uint						descriptorIndex;
		};

		using Element_t			= Union< VkDescriptorBufferInfo, VkDescriptorImageInfo, VkAccelerationStructureNV >;
		using DynamicDataPtr	= PipelineResources::DynamicDataPtr;
		using DescriptorSet		= VDescriptorSetLayout::DescriptorSet;


	// variables
	private:
		DescriptorSet				_descriptorSet;
		RawDescriptorSetLayoutID	_layoutId;
		HashVal						_hash;
		DynamicDataPtr				_dataPtr;
		const bool					_allowEmptyResources;
		
		DebugName_t					_debugName;
		
		RWDataRaceCheck				_drCheck;


	// methods
	public:
		VPipelineResources () : _allowEmptyResources{false} {}
		explicit VPipelineResources (const PipelineResources &desc);
		explicit VPipelineResources (INOUT PipelineResources &desc);
		~VPipelineResources ();

			bool Create (VResourceManager &);
			void Destroy (VResourceManager &);

		ND_ bool IsAllResourcesAlive (const VResourceManager &) const;

		ND_ bool operator == (const VPipelineResources &rhs) const;
		
			template <typename Fn>
			void ForEachUniform (Fn&& fn) const					{ SHAREDLOCK( _drCheck );  ASSERT( _dataPtr );  _dataPtr->ForEachUniform( fn ); }

		ND_ VkDescriptorSet				Handle ()		const	{ SHAREDLOCK( _drCheck );  return _descriptorSet.first; }
		ND_ RawDescriptorSetLayoutID	GetLayoutID ()	const	{ SHAREDLOCK( _drCheck );  return _layoutId; }
		ND_ HashVal						GetHash ()		const	{ SHAREDLOCK( _drCheck );  return _hash; }

		ND_ StringView					GetDebugName ()	const	{ SHAREDLOCK( _drCheck );  return _debugName; }


	private:
		bool _AddResource (VResourceManager &, INOUT PipelineResources::Buffer &, INOUT UpdateDescriptors &);
		bool _AddResource (VResourceManager &, INOUT PipelineResources::Image &, INOUT UpdateDescriptors &);
		bool _AddResource (VResourceManager &, INOUT PipelineResources::Texture &, INOUT UpdateDescriptors &);
		bool _AddResource (VResourceManager &, const PipelineResources::Sampler &, INOUT UpdateDescriptors &);
		bool _AddResource (VResourceManager &, const PipelineResources::RayTracingScene &, INOUT UpdateDescriptors &);
		bool _AddResource (VResourceManager &, const NullUnion &, INOUT UpdateDescriptors &);
	};
	

}	// AE::Graphics


namespace std
{

	template <>
	struct hash< AE::Graphics::VPipelineResources >
	{
		ND_ size_t  operator () (const AE::Graphics::VPipelineResources &value) const {
			return size_t(value.GetHash());
		}
	};

}	// std

#endif
