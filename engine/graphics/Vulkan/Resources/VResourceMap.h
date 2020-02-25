// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef AE_ENABLE_VULKAN

# include "graphics/Vulkan/VCommon.h"

namespace AE::Graphics
{

	//
	// Resource Map
	//

	struct VResourceMap
	{
	// types
	private:
		struct Resource
		{
		// types
			enum class EType
			{
				Buffer,
				Image,
				GraphicsPipeline,
				ComputePipeline,
				MeshPipeline,
				RayTracingPipeline,
				DescriptorSet,
				RenderPass,
				Framebuffer,
				BackedCommandBuffer,
			};

		// constants
			static constexpr uint		IndexOffset			= 0;
			static constexpr uint		GenerationOffset	= 16;
			static constexpr uint		ResTypeOffset		= 32;
			static constexpr uint		RefCountOffset		= 48;
			static constexpr uint64_t	IndexMask			= 0xFFFF;
			static constexpr uint64_t	GenerationMask		= 0xFFFF;
			static constexpr uint64_t	ResTypeMask			= 0xFF;
			static constexpr uint64_t	RefCountMask		= 0xFFFF;

			STATIC_ASSERT( ((IndexMask << IndexOffset) | (GenerationMask << GenerationOffset) | (ResTypeMask << ResTypeOffset)) == 0xFF'FFFF'FFFFull );

		// variables
			mutable uint64_t	value	= UMax;
			
		// methods
			Resource () {}
			
			Resource (uint64_t index, uint64_t generation, EType type, uint refCount = 1) :
				value{ (index << IndexOffset) | (generation << GenerationOffset) | (uint64_t(type) << ResTypeOffset) | (uint64_t(refCount) << RefCountOffset) } {}

			ND_ bool  operator == (const Resource &rhs)	const	{ return value == rhs.value; }

			ND_ uint16_t	Index ()					const	{ return uint16_t((value >> IndexOffset) & IndexMask); }
			ND_ uint16_t	Generation ()				const	{ return uint16_t((value >> GenerationOffset) & GenerationMask); }
			ND_ EType		ResType ()					const	{ return EType((value >> ResTypeOffset) & ResTypeMask); }
			ND_ uint		RefCount ()					const	{ return (value >> RefCountOffset) & RefCountMask; }
			ND_ size_t		GetHash ()					const	{ return std::hash<decltype(value)>{}( value & ~(RefCountMask << RefCountOffset) ); }

			void IncRefCount () const
			{
				uint64_t	cnt = RefCount() + 1;
				value &= ~(RefCountMask << RefCountOffset);
				value |= (cnt << RefCountOffset);
			}
		};

		struct ResourceHash {
			ND_ size_t  operator () (const Resource &x) const {
				return x.GetHash();
			}
		};
		
		using ResourceMap_t = HashSet< Resource, ResourceHash >;		// TODO: custom allocator


	// variables
	private:
		ResourceMap_t	_items;


	// methods
	public:
		VResourceMap ()		{}
		~VResourceMap ()	{ CHECK( _items.empty() ); }

		bool  Add (VBufferID id);
		bool  Add (VImageID id);
		bool  Add (GraphicsPipelineID id);
		bool  Add (MeshPipelineID id);
		bool  Add (ComputePipelineID id);
		bool  Add (RayTracingPipelineID id);
		bool  Add (RenderPassID id);
		bool  Add (VFramebufferID id);
		bool  Add (BakedCommandBufferID id);

		void  Release (VResourceManager &);

		void  Merge (INOUT VResourceMap &);
	};

	
/*
=================================================
	Add
=================================================
*/
	inline bool  VResourceMap::Add (VBufferID id)
	{
		return _items.insert(Resource{ id.Index(), id.Generation(), Resource::EType::Buffer }).second;
	}

	inline bool  VResourceMap::Add (VImageID id)
	{
		return _items.insert(Resource{ id.Index(), id.Generation(), Resource::EType::Image }).second;
	}

	inline bool  VResourceMap::Add (GraphicsPipelineID id)
	{
		return _items.insert(Resource{ id.Index(), id.Generation(), Resource::EType::GraphicsPipeline }).second;
	}

	inline bool  VResourceMap::Add (MeshPipelineID id)
	{
		return _items.insert(Resource{ id.Index(), id.Generation(), Resource::EType::MeshPipeline }).second;
	}

	inline bool  VResourceMap::Add (ComputePipelineID id)
	{
		return _items.insert(Resource{ id.Index(), id.Generation(), Resource::EType::ComputePipeline }).second;
	}

	inline bool  VResourceMap::Add (RayTracingPipelineID id)
	{
		return _items.insert(Resource{ id.Index(), id.Generation(), Resource::EType::RayTracingPipeline }).second;
	}
	
	inline bool  VResourceMap::Add (RenderPassID id)
	{
		return _items.insert(Resource{ id.Index(), id.Generation(), Resource::EType::RenderPass }).second;
	}

	inline bool  VResourceMap::Add (VFramebufferID id)
	{
		return _items.insert(Resource{ id.Index(), id.Generation(), Resource::EType::Framebuffer }).second;
	}

	inline bool  VResourceMap::Add (BakedCommandBufferID id)
	{
		return _items.insert(Resource{ id.Index(), id.Generation(), Resource::EType::BackedCommandBuffer }).second;
	}


}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
