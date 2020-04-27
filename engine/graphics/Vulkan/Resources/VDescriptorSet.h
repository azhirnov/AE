// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef AE_ENABLE_VULKAN

# include "graphics/Public/DescriptorSet.h"
# include "graphics/Vulkan/Resources/VDescriptorSetLayout.h"
# include "graphics/Vulkan/VDescriptorManager.h"
# include "stl/Memory/LinearAllocator.h"

namespace AE::Graphics
{

	//
	// Vulkan Descriptor Set
	//

	class VDescriptorSet final
	{
	// types
	private:
		struct UpdateDescriptors
		{
			LinearAllocator<>			allocator;
			VkWriteDescriptorSet *		descriptors;
			uint						descriptorIndex;
		};

		using DynamicDataPtr	= DescriptorSet::DynamicDataPtr;
		using DescSetRef		= VDescriptorManager::DescSetRef;
		using EDescriptorType	= DescriptorSet::EDescriptorType;


	// variables
	private:
		DescSetRef				_descriptorSet;
		DescriptorSetLayoutID	_layoutId;
		HashVal					_hash;
		DynamicDataPtr			_dataPtr;
		const bool				_allowEmptyResources;
		
		DebugName_t				_debugName;
		
		RWDataRaceCheck			_drCheck;


	// methods
	public:
		VDescriptorSet () : _allowEmptyResources{false} {}
		explicit VDescriptorSet (const DescriptorSet &desc);
		explicit VDescriptorSet (INOUT DescriptorSet &desc);
		~VDescriptorSet ();

			bool Create (VResourceManager &, const VDescriptorSetLayout &, bool quiet);
			void Destroy (VResourceManager &);

		ND_ bool IsAllResourcesAlive (const VResourceManager &) const;

		ND_ bool operator == (const VDescriptorSet &rhs) const;
		
			template <typename Fn>
			void ForEachUniform (Fn&& fn) const				{ SHAREDLOCK( _drCheck );  ASSERT( _dataPtr );  _dataPtr->ForEachUniform( fn ); }

		ND_ VkDescriptorSet			Handle ()		const	{ SHAREDLOCK( _drCheck );  return _descriptorSet.handle; }
		ND_ DescriptorSetLayoutID	GetLayoutID ()	const	{ SHAREDLOCK( _drCheck );  return _layoutId; }
		ND_ HashVal					GetHash ()		const	{ SHAREDLOCK( _drCheck );  return _hash; }

		ND_ StringView				GetDebugName ()	const	{ SHAREDLOCK( _drCheck );  return _debugName; }


	private:
		bool _AddResource (VResourceManager &, bool quiet, EDescriptorType type, INOUT DescriptorSet::Buffer &, INOUT UpdateDescriptors &);
		bool _AddResource (VResourceManager &, bool quiet, EDescriptorType type, INOUT DescriptorSet::TexelBuffer &, INOUT UpdateDescriptors &);
		bool _AddResource (VResourceManager &, bool quiet, EDescriptorType type, INOUT DescriptorSet::Image &, INOUT UpdateDescriptors &);
		bool _AddResource (VResourceManager &, bool quiet, EDescriptorType type, INOUT DescriptorSet::Texture &, INOUT UpdateDescriptors &);
		bool _AddResource (VResourceManager &, bool quiet, EDescriptorType type, const DescriptorSet::Sampler &, INOUT UpdateDescriptors &);
		//bool _AddResource (VResourceManager &, bool quiet, EDescriptorType type, const DescriptorSet::RayTracingScene &, INOUT UpdateDescriptors &);
		bool _AddResource (VResourceManager &, bool quiet, EDescriptorType type, const NullUnion &, INOUT UpdateDescriptors &);
	};
	

}	// AE::Graphics


namespace std
{

	template <>
	struct hash< AE::Graphics::VDescriptorSet >
	{
		ND_ size_t  operator () (const AE::Graphics::VDescriptorSet &value) const {
			return size_t(value.GetHash());
		}
	};

}	// std

#endif	// AE_ENABLE_VULKAN
