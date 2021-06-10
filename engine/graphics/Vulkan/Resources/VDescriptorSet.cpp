// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#ifdef AE_ENABLE_VULKAN

# include "graphics/Vulkan/Resources/VDescriptorSet.h"
# include "graphics/Vulkan/VResourceManager.h"
# include "graphics/Vulkan/VEnumCast.h"
# include "graphics/Private/DescriptorSetHelper.h"

namespace AE::Graphics
{

/*
=================================================
	constructor
=================================================
*/
	VDescriptorSet::VDescriptorSet (const DescriptorSet &desc) :
		_allowEmptyResources{ desc.IsEmptyResourcesAllowed() }
	{
		EXLOCK( _drCheck );
		
		_dataPtr	= DescriptorSetHelper::CloneDynamicData( desc );
		_layoutId	= desc.GetLayout();
		_hash		= HashOf( _layoutId );
		
		if ( _dataPtr )
			_hash << _dataPtr->CalcHash();
	}
	
/*
=================================================
	constructor
=================================================
*/
	VDescriptorSet::VDescriptorSet (INOUT DescriptorSet &desc) :
		_dataPtr{ DescriptorSetHelper::RemoveDynamicData( INOUT desc )},
		_allowEmptyResources{ desc.IsEmptyResourcesAllowed() }
	{
		EXLOCK( _drCheck );
		
		_layoutId	= _dataPtr->layoutId;
		_hash		= HashOf( _layoutId );
		
		if ( _dataPtr )
			_hash << _dataPtr->CalcHash();
	}

/*
=================================================
	destructor
=================================================
*/
	VDescriptorSet::~VDescriptorSet ()
	{
		CHECK( not _descriptorSet.handle );
	}
	
/*
=================================================
	Create
=================================================
*/
	bool  VDescriptorSet::Create (VResourceManagerImpl &resMngr, const VDescriptorSetLayout &dsLayout, Bool quiet)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( not _descriptorSet.handle );
		CHECK_ERR( _dataPtr );
	
		CHECK_ERR( resMngr.GetDescriptorManager().AllocDescriptorSet( _layoutId, OUT _descriptorSet ));
		
		auto&	dev = resMngr.GetDevice();

		UpdateDescriptors	update;
		update.descriptors		= update.allocator.Alloc< VkWriteDescriptorSet >( dsLayout.GetMaxIndex() + 1 );
		update.descriptorIndex	= 0;

		_dataPtr->ForEachUniform( [&](auto&, auto type, auto& data) { _AddResource( resMngr, quiet, type, data, INOUT update ); });
		
		dev.vkUpdateDescriptorSets( dev.GetVkDevice(), update.descriptorIndex, update.descriptors, 0, null );
		return true;
	}

/*
=================================================
	Destroy
=================================================
*/
	void  VDescriptorSet::Destroy (VResourceManagerImpl &resMngr)
	{
		EXLOCK( _drCheck );

		if ( _descriptorSet.handle ) {
			resMngr.GetDescriptorManager().DeallocDescriptorSet( _layoutId, _descriptorSet );
		}

		// release reference only if descriptor set was created
		if ( _layoutId and _descriptorSet.handle )
		{
			UniqueID<DescriptorSetLayoutID>	temp{_layoutId};
			resMngr.ReleaseResource( temp );
		}

		_dataPtr.reset();
		_descriptorSet	= { VK_NULL_HANDLE, UMax };
		_layoutId		= Default;
		_hash			= Default;
	}
	
/*
=================================================
	IsAllResourcesAlive
=================================================
*/
	bool  VDescriptorSet::IsAllResourcesAlive (const VResourceManagerImpl &resMngr) const
	{
		SHAREDLOCK( _drCheck );

		struct Visitor
		{
			VResourceManagerImpl const&	resMngr;
			bool					alive	= true;
			
			Visitor (const VResourceManagerImpl &resMngr) : resMngr{resMngr}
			{}

			void operator () (const UniformName &, EDescriptorType, const DescriptorSet::Buffer &buf)
			{
				for (uint i = 0; i < buf.elementCount; ++i)
				{
					auto&	elem = buf.elements[i];
					alive &= not elem.bufferId or resMngr.IsResourceAlive( elem.bufferId );
				}
			}

			void operator () (const UniformName &, EDescriptorType, const DescriptorSet::TexelBuffer &buf)
			{
				for (uint i = 0; i < buf.elementCount; ++i)
				{
					auto&	elem = buf.elements[i];
					alive &= not elem.bufferId or resMngr.IsResourceAlive( elem.bufferId );
				}
			}

			void operator () (const UniformName &, EDescriptorType, const DescriptorSet::Image &img)
			{
				for (uint i = 0; i < img.elementCount; ++i)
				{
					auto&	elem = img.elements[i];
					alive &= not elem.imageId or resMngr.IsResourceAlive( elem.imageId );
				}
			}

			void operator () (const UniformName &, EDescriptorType, const DescriptorSet::Texture &tex)
			{
				for (uint i = 0; i < tex.elementCount; ++i)
				{
					auto&	elem = tex.elements[i];
					alive &= not elem.imageId or (resMngr.IsResourceAlive( elem.imageId ) and resMngr.IsAlive( elem.sampler ));
				}
			}

			void operator () (const UniformName &, EDescriptorType, const DescriptorSet::Sampler &samp)
			{
				for (uint i = 0; i < samp.elementCount; ++i)
				{
					auto&	elem = samp.elements[i];
					alive &= resMngr.IsAlive( elem.sampler );
				}
			}

			/*void operator () (const UniformName &, EDescriptorType, const DescriptorSet::RayTracingScene &rts)
			{
				for (uint i = 0; i < rts.elementCount; ++i)
				{
					auto&	elem = rts.elements[i];
					alive &= not elem.sceneId or resMngr.IsAlive( elem.sceneId );
				}
			}*/
		};

		Visitor	vis{ resMngr };
		ForEachUniform( vis );

		return vis.alive;
	}

/*
=================================================
	operator ==
=================================================
*/
	bool  VDescriptorSet::operator == (const VDescriptorSet &rhs) const
	{
		SHAREDLOCK( _drCheck );
		SHAREDLOCK( rhs._drCheck );

		return	_hash		==  rhs._hash		and
				_layoutId	==  rhs._layoutId	and
				_dataPtr	and rhs._dataPtr	and
				*_dataPtr	== *rhs._dataPtr;
	}
	
/*
=================================================
	_AddResource
=================================================
*/
	bool  VDescriptorSet::_AddResource (VResourceManagerImpl &resMngr, bool quiet, EDescriptorType type, INOUT DescriptorSet::Buffer &buf, INOUT UpdateDescriptors &list)
	{
		ASSERT( type == EDescriptorType::UniformBuffer or
				type == EDescriptorType::StorageBuffer );

		auto*	info = list.allocator.Alloc< VkDescriptorBufferInfo >( buf.elementCount );

		for (uint i = 0; i < buf.elementCount; ++i)
		{
			const auto&		elem	= buf.elements[i];
			VBuffer const*	buffer	= resMngr.GetResource( elem.bufferId );

			if ( not buffer )
			{
				CHECK( not quiet or (buffer or _allowEmptyResources) );
				continue;
			}

			info[i].buffer	= buffer->Handle();
			info[i].offset	= VkDeviceSize(elem.offset);
			info[i].range	= VkDeviceSize(elem.size);
		}

		const bool	is_uniform	= ((buf.state & EResourceState::_StateMask) == EResourceState::UniformRead);
		const bool	is_dynamic	= buf.dynamicOffsetIndex != UMax;

		VkWriteDescriptorSet&	wds = list.descriptors[list.descriptorIndex++];
		wds = {};
		wds.sType			= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		wds.descriptorType	= type == EDescriptorType::UniformBuffer ? (is_dynamic ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC : VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) :
							  type == EDescriptorType::StorageBuffer ? (is_dynamic ? VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER) :
																		VK_DESCRIPTOR_TYPE_MAX_ENUM;
		wds.descriptorCount	= buf.elementCount;
		wds.dstBinding		= buf.index;
		wds.dstSet			= _descriptorSet.handle;
		wds.pBufferInfo		= info;

		return true;
	}
	
/*
=================================================
	_AddResource
=================================================
*/
	bool  VDescriptorSet::_AddResource (VResourceManagerImpl &resMngr, bool quiet, EDescriptorType type, INOUT DescriptorSet::TexelBuffer &texbuf, INOUT UpdateDescriptors &list)
	{
		ASSERT( type == EDescriptorType::UniformTexelBuffer or
				type == EDescriptorType::StorageTexelBuffer );

		auto*	info = list.allocator.Alloc< VkBufferView >( texbuf.elementCount );

		for (uint i = 0; i < texbuf.elementCount; ++i)
		{
			const auto&		elem	= texbuf.elements[i];
			VBuffer const*	buffer	= resMngr.GetResource( elem.bufferId );

			if ( not buffer )
			{
				CHECK( not quiet or (buffer or _allowEmptyResources) );
				continue;
			}

			info[i] = buffer->GetView( resMngr.GetDevice(), elem.desc );
		}

		VkWriteDescriptorSet&	wds = list.descriptors[list.descriptorIndex++];
		wds = {};
		wds.sType				= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		wds.descriptorType		= type == EDescriptorType::UniformTexelBuffer ? VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER :
								  type == EDescriptorType::StorageTexelBuffer ? VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER :
																				VK_DESCRIPTOR_TYPE_MAX_ENUM;
		wds.descriptorCount		= texbuf.elementCount;
		wds.dstBinding			= texbuf.index;
		wds.dstSet				= _descriptorSet.handle;
		wds.pTexelBufferView	= info;

		return true;
	}

/*
=================================================
	_AddResource
=================================================
*/
	bool  VDescriptorSet::_AddResource (VResourceManagerImpl &resMngr, bool quiet, EDescriptorType type, INOUT DescriptorSet::Image &img, INOUT UpdateDescriptors &list)
	{
		ASSERT( type == EDescriptorType::StorageImage or
				type == EDescriptorType::SampledImage or
				type == EDescriptorType::SubpassInput or
				type == EDescriptorType::CombinedImage_ImmutableSampler );

		auto*	info = list.allocator.Alloc< VkDescriptorImageInfo >( img.elementCount );

		for (uint i = 0; i < img.elementCount; ++i)
		{
			auto&			elem	= img.elements[i];
			VImage const*	image	= resMngr.GetResource( elem.imageId );

			if ( not image )
			{
				CHECK( not quiet or (image or _allowEmptyResources) );
				continue;
			}

			info[i].imageLayout	= EResourceState_ToImageLayout( img.state, image->AspectMask() );
			info[i].imageView	= image->GetView( resMngr.GetDevice(), not elem.hasDesc, INOUT elem.desc );
			info[i].sampler		= VK_NULL_HANDLE;
		}		
		
		VkWriteDescriptorSet&	wds = list.descriptors[list.descriptorIndex++];
		wds = {};
		wds.sType			= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		wds.descriptorType	= type == EDescriptorType::StorageImage ? VK_DESCRIPTOR_TYPE_STORAGE_IMAGE :
							  type == EDescriptorType::SampledImage ? VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE :
							  type == EDescriptorType::SubpassInput ? VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT :
							  type == EDescriptorType::CombinedImage_ImmutableSampler ? VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER :
																	  VK_DESCRIPTOR_TYPE_MAX_ENUM;
		wds.descriptorCount	= img.elementCount;
		wds.dstBinding		= img.index;
		wds.dstSet			= _descriptorSet.handle;
		wds.pImageInfo		= info;

		return true;
	}
	
/*
=================================================
	_AddResource
=================================================
*/
	bool  VDescriptorSet::_AddResource (VResourceManagerImpl &resMngr, bool quiet, EDescriptorType type, INOUT DescriptorSet::Texture &tex, INOUT UpdateDescriptors &list)
	{
		ASSERT( type == EDescriptorType::CombinedImage );

		auto*	info = list.allocator.Alloc< VkDescriptorImageInfo >( tex.elementCount );

		for (uint i = 0; i < tex.elementCount; ++i)
		{
			auto&			elem	= tex.elements[i];
			VImage const*	image	= resMngr.GetResource( elem.imageId );
			VkSampler		sampler	= resMngr.GetVkSampler( elem.sampler );

			if ( not (image and sampler) )
			{
				CHECK( not quiet or ((image and sampler) or _allowEmptyResources) );
				continue;
			}

			info[i].imageLayout	= EResourceState_ToImageLayout( tex.state, image->AspectMask() );
			info[i].imageView	= image->GetView( resMngr.GetDevice(), not elem.hasDesc, INOUT elem.desc );
			info[i].sampler		= sampler;
		}
		
		VkWriteDescriptorSet&	wds = list.descriptors[list.descriptorIndex++];
		wds = {};
		wds.sType			= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		wds.descriptorType	= VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		wds.descriptorCount	= tex.elementCount;
		wds.dstBinding		= tex.index;
		wds.dstSet			= _descriptorSet.handle;
		wds.pImageInfo		= info;

		return true;
	}
	
/*
=================================================
	_AddResource
=================================================
*/
	bool  VDescriptorSet::_AddResource (VResourceManagerImpl &resMngr, bool quiet, EDescriptorType type, const DescriptorSet::Sampler &samp, INOUT UpdateDescriptors &list)
	{
		ASSERT( type == EDescriptorType::Sampler );

		auto*	info = list.allocator.Alloc< VkDescriptorImageInfo >( samp.elementCount );

		for (uint i = 0; i < samp.elementCount; ++i)
		{
			auto&		elem	= samp.elements[i];
			VkSampler	sampler = resMngr.GetVkSampler( elem.sampler );

			if ( not sampler )
			{
				CHECK( not quiet or (sampler or _allowEmptyResources) );
				continue;
			}

			info[i].imageLayout	= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			info[i].imageView	= VK_NULL_HANDLE;
			info[i].sampler		= sampler;
		}
		
		VkWriteDescriptorSet&	wds = list.descriptors[list.descriptorIndex++];
		wds = {};
		wds.sType			= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		wds.descriptorType	= VK_DESCRIPTOR_TYPE_SAMPLER;
		wds.descriptorCount	= samp.elementCount;
		wds.dstBinding		= samp.index;
		wds.dstSet			= _descriptorSet.handle;
		wds.pImageInfo		= info;

		return true;
	}
	
/*
=================================================
	_AddResource
=================================================
*
	bool  VDescriptorSet::_AddResource (VResourceManagerImpl &resMngr, bool quiet, EDescriptorType type, const DescriptorSet::RayTracingScene &rtScene, INOUT UpdateDescriptors &list)
	{
		auto*	tlas = list.allocator.Alloc<VkAccelerationStructureNV>( rtScene.elementCount );

		for (uint i = 0; i < rtScene.elementCount; ++i)
		{
			auto&					elem	= rtScene.elements[i];
			VRayTracingScene const*	scene	= resMngr.GetResource( elem.sceneId );
			CHECK( scene or _allowEmptyResources );

			tlas[i] = scene ? scene->Handle() : VK_NULL_HANDLE;
		}

		auto& 	top_as = *list.allocator.Alloc<VkWriteDescriptorSetAccelerationStructureNV>( 1 );
		top_as = {};
		top_as.sType						= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_NV;
		top_as.accelerationStructureCount	= rtScene.elementCount;
		top_as.pAccelerationStructures		= tlas;
		
		
		VkWriteDescriptorSet&	wds = list.descriptors[list.descriptorIndex++];
		wds = {};
		wds.sType			= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		wds.pNext			= &top_as;
		wds.descriptorType	= VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV;
		wds.descriptorCount	= rtScene.elementCount;
		wds.dstBinding		= rtScene.index;
		wds.dstSet			= _descriptorSet.handle;

		return true;
	}

/*
=================================================
	_AddResource
=================================================
*/
	bool  VDescriptorSet::_AddResource (VResourceManagerImpl &, bool quiet, EDescriptorType type, const NullUnion &, INOUT UpdateDescriptors &)
	{
		if ( not quiet )
		{
			ASSERT( type == EDescriptorType::Unknown );
			CHECK( _allowEmptyResources and "uninitialized uniform!" );
		}
		return false;
	}


}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
