// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#ifdef AE_ENABLE_VULKAN

# include "graphics/Vulkan/Resources/VPipelineLayout.h"
# include "graphics/Vulkan/VEnumCast.h"
# include "graphics/Vulkan/VResourceManager.h"

namespace AE::Graphics
{

/*
=================================================
	destructor
=================================================
*/
	VPipelineLayout::~VPipelineLayout ()
	{
		CHECK( _layout == VK_NULL_HANDLE );
	}
	
/*
=================================================
	_AddDescriptorSets
=================================================
*
	void VPipelineLayout::_AddDescriptorSets (const PipelineDescription::PipelineLayout &ppln, DSLayoutArray_t sets,
											  INOUT HashVal &hash, OUT DescriptorSets_t &setsInfo) const
	{
		setsInfo.clear();

		for (size_t i = 0; i < ppln.descriptorSets.size(); ++i)
		{
			auto&	ds  = ppln.descriptorSets[i];
			auto&	res = sets[i].second->Data();

			ASSERT( ds.id.IsValid() );
			ASSERT( ds.bindingIndex < MaxDescSets );
			ASSERT( res.Handle() );

			setsInfo.insert({ ds.id, DescSetLayout{ sets[i].first, res.Handle(), ds.bindingIndex }});
			
			// calculate hash
			hash << HashOf( ds.id );
			hash << HashOf( ds.bindingIndex );
			hash << res.GetHash();
		}
	}
	
/*
=================================================
	_AddPushConstants
=================================================
*
	void VPipelineLayout::_AddPushConstants (const PipelineDescription::PipelineLayout &ppln, INOUT HashVal &hash,
											 OUT PushConstants_t &pushConst) const
	{
		pushConst = ppln.pushConstants;

		for (auto& pc : ppln.pushConstants)
		{
			ASSERT( pc.first.IsDefined() );

			// calculate hash
			hash << PushConstantHash( pc );
		}
	}

/*
=================================================
	Create
=================================================
*/
	bool VPipelineLayout::Create (const VDevice &dev, const DescriptorSets_t &descSetLayouts, const PushConstants_t &pusConstants,
								  VkDescriptorSetLayout emptyLayout, StringView dbgName)
	{
		using VkDescriptorSetLayouts_t	= StaticArray< VkDescriptorSetLayout, GraphicsConfig::MaxDescriptorSets >;
		using VkPushConstantRanges_t	= FixedArray< VkPushConstantRange, GraphicsConfig::MaxPushConstants >;

		EXLOCK( _drCheck );
		CHECK_ERR( _layout == VK_NULL_HANDLE );

		_descriptorSets	= descSetLayouts;
		_pushConstants	= pusConstants;
		_debugName		= dbgName;

		VkDescriptorSetLayouts_t	vk_layouts;
		VkPushConstantRanges_t		vk_ranges;

		for (auto& layout : vk_layouts) {
			layout = emptyLayout;
		}

		uint	min_set = uint(vk_layouts.size());
		uint	max_set = 1;
		auto	ds_iter = _descriptorSets.begin();

		for (size_t i = 0; ds_iter != _descriptorSets.end(); ++i, ++ds_iter)
		{
			auto&	ds = ds_iter->second;
			ASSERT( vk_layouts[ ds.index ] == emptyLayout );
			ASSERT( ds.layout );

			vk_layouts[ ds.index ] = ds.layout;
			min_set = Min( min_set, ds.index );
			max_set = Max( max_set, ds.index + 1 );
		}

		for (auto& pc : _pushConstants)
		{
			VkPushConstantRange	range = {};
			range.offset		= uint( pc.second.offset );
			range.size			= uint( pc.second.size );
			range.stageFlags	= VEnumCast( pc.second.stageFlags );

			vk_ranges.push_back( range );
		}
		
		VkPipelineLayoutCreateInfo			layout_info = {};
		layout_info.sType					= VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		layout_info.setLayoutCount			= max_set;
		layout_info.pSetLayouts				= vk_layouts.data();
		layout_info.pushConstantRangeCount	= uint(vk_ranges.size());
		layout_info.pPushConstantRanges		= vk_ranges.data();

		VK_CHECK( dev.vkCreatePipelineLayout( dev.GetVkDevice(), &layout_info, null, OUT &_layout ));

		_firstDescSet = min_set;

		if ( _debugName.size() )
			dev.SetObjectName( uint64_t(_layout), _debugName, VK_OBJECT_TYPE_PIPELINE_LAYOUT );

		return true;
	}
	
/*
=================================================
	Destroy
=================================================
*/
	void VPipelineLayout::Destroy (const VDevice &dev)
	{
		EXLOCK( _drCheck );

		if ( _layout ) {
			dev.vkDestroyPipelineLayout( dev.GetVkDevice(), _layout, null );
		}

		_descriptorSets.clear();
		_pushConstants.clear();
		_debugName.clear();

		_layout			= VK_NULL_HANDLE;
		_firstDescSet	= UMax;
	}
	
/*
=================================================
	GetDescriptorSetLayout
=================================================
*/
	bool VPipelineLayout::GetDescriptorSetLayout (const DescriptorSetName &id, OUT VDescriptorSetLayoutID &layout, OUT uint &binding) const
	{
		SHAREDLOCK( _drCheck );

		auto	iter = _descriptorSets.find( id );

		if ( iter != _descriptorSets.end() )
		{
			layout	= iter->second.layoutId;
			binding	= iter->second.index;
			return true;
		}

		return false;
	}


}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
