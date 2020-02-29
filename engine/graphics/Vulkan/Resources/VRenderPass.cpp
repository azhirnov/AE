// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#ifdef AE_ENABLE_VULKAN

# include "graphics/Vulkan/Resources/VRenderPass.h"
# include "graphics/Vulkan/VResourceManager.h"
# include "graphics/Vulkan/VEnumCast.h"

namespace AE::Graphics
{

/*
=================================================
	destructor
=================================================
*/
	VRenderPass::~VRenderPass ()
	{
		EXLOCK( _drCheck );
		CHECK( not _renderPass );
	}

/*
=================================================
	constructor
=================================================
*/
	VRenderPass::VRenderPass (ArrayView<VLogicalRenderPass*> logicalPasses)
	{
		_Initialize( logicalPasses );
	}
		
	bool VRenderPass::_Initialize (ArrayView<VLogicalRenderPass*> logicalPasses)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( logicalPasses.size() == 1 );		// not supported yet

		const auto *	pass		= logicalPasses.front();
		uint			max_index	= 0;

		_attachments.resize( _attachments.capacity() );

		_subpassOutput.resize( logicalPasses.size() );
		_subpasses.resize( logicalPasses.size() );

		VkSubpassDescription&	subpass	= _subpasses[0];

		subpass.pipelineBindPoint	= VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.pColorAttachments	= _attachmentRef.end();

		_subpassOutput[0]			= pass->GetRenderPassOutput();
		

		// setup color attachments
		for (auto& rt : pass->GetColorTargets())
		{
			const VkImageLayout			layout	= rt.layout;
			VkAttachmentDescription&	desc	= _attachments[ rt.index ];

			desc.flags			= 0;			// TODO: VK_ATTACHMENT_DESCRIPTION_MAY_ALIAS_BIT
			desc.format			= VEnumCast( rt.viewDesc.format );
			desc.samples		= rt.samples;
			desc.loadOp			= rt.loadOp;
			desc.storeOp		= rt.storeOp;
			desc.initialLayout	= layout;
			desc.finalLayout	= layout;

			_attachmentRef.push_back({ rt.index, layout });
			++subpass.colorAttachmentCount;

			max_index = Max( rt.index+1, max_index );
		}

		if ( subpass.colorAttachmentCount == 0 )
			subpass.pColorAttachments = null;


		// setup depth stencil attachment
		if ( pass->GetDepthStencilTarget().IsDefined() )
		{
			const auto&					ds_target	= pass->GetDepthStencilTarget();
			const VkImageLayout			layout		= ds_target.layout;
			VkAttachmentDescription&	desc		= _attachments[max_index];

			desc.flags			= 0;
			desc.format			= VEnumCast( ds_target.viewDesc.format );
			desc.samples		= ds_target.samples;
			desc.loadOp			= ds_target.loadOp;
			desc.stencilLoadOp	= ds_target.loadOp;		// TODO: use resource state to change state
			desc.storeOp		= ds_target.storeOp;
			desc.stencilStoreOp	= ds_target.storeOp;
			desc.initialLayout	= layout;
			desc.finalLayout	= layout;

			subpass.pDepthStencilAttachment	= _attachmentRef.end();
			_attachmentRef.push_back({ max_index++, layout });
		}

		_attachments.resize( max_index );


		// setup create info
		_createInfo					= {};
		_createInfo.sType			= VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		_createInfo.flags			= 0;
		_createInfo.attachmentCount	= uint(_attachments.size());
		_createInfo.pAttachments	= _attachments.data();
		_createInfo.subpassCount	= uint(_subpasses.size());
		_createInfo.pSubpasses		= _subpasses.data();


		_CalcHash( OUT _hash, OUT _attachmentHash, OUT _subpassesHash );
		return true;
	}

/*
=================================================
	_CalcHash
=================================================
*/
	void VRenderPass::_CalcHash (OUT HashVal &mainHash, OUT HashVal &attachmentHash, OUT SubpassesHash_t &subpassesHash) const
	{
		ASSERT( _createInfo.pNext == null );
		
		const auto AttachmentRefHash = [] (const VkAttachmentReference *attachment) -> HashVal
		{
			return	attachment ?
						(HashOf( attachment->attachment ) + HashOf( attachment->layout )) :
						HashVal();
		};

		const auto AttachmentRefArrayHash = [&AttachmentRefHash] (const VkAttachmentReference* attachments, uint count) -> HashVal
		{
			if ( attachments == null or count == 0 )
				return HashVal();

			HashVal res = HashOf( count );

			for (uint i = 0; i < count; ++i) {
				res << AttachmentRefHash( attachments + i );
			}
			return res;
		};
		
		mainHash = HashVal();


		// calculate attachment hash
		attachmentHash = HashOf( _createInfo.attachmentCount );

		for (uint i = 0; i < _createInfo.attachmentCount; ++i)
		{
			const VkAttachmentDescription&	attachment = _createInfo.pAttachments[i];

			attachmentHash << HashOf( attachment.flags );
			attachmentHash << HashOf( attachment.format );
			attachmentHash << HashOf( attachment.samples );
			attachmentHash << HashOf( attachment.loadOp );
			attachmentHash << HashOf( attachment.storeOp );
			attachmentHash << HashOf( attachment.stencilLoadOp );
			attachmentHash << HashOf( attachment.stencilStoreOp );
			attachmentHash << HashOf( attachment.initialLayout );
			attachmentHash << HashOf( attachment.finalLayout );
		}
		

		// calculate subpasses hash
		subpassesHash.resize( _createInfo.subpassCount );

		for (uint i = 0; i < _createInfo.subpassCount; ++i)
		{
			const VkSubpassDescription&	subpass = _createInfo.pSubpasses[i];
			HashVal &					hash	= subpassesHash[i];
			
			hash  = HashOf( _subpassOutput[i] );
			hash << HashOf( subpass.flags );
			hash << HashOf( subpass.pipelineBindPoint );
			hash << AttachmentRefArrayHash( subpass.pInputAttachments, subpass.inputAttachmentCount );
			hash << AttachmentRefArrayHash( subpass.pColorAttachments, subpass.colorAttachmentCount );
			hash << AttachmentRefArrayHash( subpass.pResolveAttachments, subpass.colorAttachmentCount );
			hash << AttachmentRefHash( subpass.pDepthStencilAttachment );

			for (uint j = 0; j < subpass.preserveAttachmentCount; ++j) {
				hash << HashOf( subpass.pPreserveAttachments[j] );
			}
			mainHash << hash;
		}
		

		// calculate main hash
		mainHash << HashOf( _createInfo.flags );
		mainHash << HashOf( _createInfo.subpassCount );
		mainHash << HashOf( _createInfo.dependencyCount );
		mainHash << attachmentHash;

		for (uint i = 0; i < _createInfo.dependencyCount; ++i)
		{
			const VkSubpassDependency&	dep = _createInfo.pDependencies[i];
			
			mainHash << HashOf( dep.srcSubpass );
			mainHash << HashOf( dep.dstSubpass );
			mainHash << HashOf( dep.srcStageMask );
			mainHash << HashOf( dep.dstStageMask );
			mainHash << HashOf( dep.srcAccessMask );
			mainHash << HashOf( dep.dstAccessMask );
			mainHash << HashOf( dep.dependencyFlags );
		}
	}

/*
=================================================
	Create
=================================================
*/
	bool VRenderPass::Create (const VDevice &dev, StringView dbgName)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( _renderPass == VK_NULL_HANDLE );

		VK_CHECK( dev.vkCreateRenderPass( dev.GetVkDevice(), &_createInfo, null, OUT &_renderPass ));
		
		_debugName = dbgName;

		if ( _debugName.size() )
			dev.SetObjectName( BitCast<uint64_t>(_renderPass), _debugName, VK_OBJECT_TYPE_RENDER_PASS );

		return true;
	}

/*
=================================================
	Destroy
=================================================
*/
	void VRenderPass::Destroy (VResourceManager &resMngr)
	{
		EXLOCK( _drCheck );

		if ( _renderPass ) {
			auto&	dev = resMngr.GetDevice();
			dev.vkDestroyRenderPass( dev.GetVkDevice(), _renderPass, null );
		}

		_renderPass		= VK_NULL_HANDLE;
		_createInfo		= Default;
		_hash			= Default;
		_attachmentHash	= Default;

		_subpassesHash.clear();
		_subpassOutput.clear();

		_attachments.clear();
		_attachmentRef.clear();
		_inputAttachRef.clear();
		_resolveAttachRef.clear();
		_subpasses.clear();
		_dependencies.clear();
		_preserves.clear();

		_debugName.clear();
	}
}	// AE::Graphics
//-----------------------------------------------------------------------------



/*
=================================================
	operator ==
=================================================
*/
	inline bool operator == (const VkAttachmentDescription &lhs, const VkAttachmentDescription &rhs)
	{
		return	lhs.flags			== rhs.flags			and
				lhs.format			== rhs.format			and
				lhs.samples			== rhs.samples			and
				lhs.loadOp			== rhs.loadOp			and
				lhs.storeOp			== rhs.storeOp			and
				lhs.stencilLoadOp	== rhs.stencilLoadOp	and
				lhs.stencilStoreOp	== rhs.stencilStoreOp	and
				lhs.initialLayout	== rhs.initialLayout	and
				lhs.finalLayout		== rhs.finalLayout;
	}
	
/*
=================================================
	operator ==
=================================================
*/
	inline bool operator == (const VkAttachmentReference &lhs, const VkAttachmentReference &rhs)
	{
		return	lhs.attachment	== rhs.attachment	and
				lhs.layout		== rhs.layout;
	}

/*
=================================================
	operator ==
=================================================
*/
	inline bool operator == (const VkSubpassDescription &lhs, const VkSubpassDescription &rhs)
	{
		using AttachView	= AE::STL::ArrayView< VkAttachmentReference >;
		using PreserveView	= AE::STL::ArrayView< uint32_t >;

		auto	lhs_resolve_attachments = lhs.pResolveAttachments ? AttachView{lhs.pResolveAttachments, lhs.colorAttachmentCount} : AttachView{};
		auto	rhs_resolve_attachments = rhs.pResolveAttachments ? AttachView{rhs.pResolveAttachments, rhs.colorAttachmentCount} : AttachView{};

		return	lhs.flags															== rhs.flags														and
				lhs.pipelineBindPoint												== rhs.pipelineBindPoint											and
				AttachView{lhs.pInputAttachments, lhs.inputAttachmentCount}			== AttachView{rhs.pInputAttachments, rhs.inputAttachmentCount}		and
				AttachView{lhs.pColorAttachments, lhs.colorAttachmentCount}			== AttachView{rhs.pColorAttachments, rhs.colorAttachmentCount}		and
				lhs_resolve_attachments												== rhs_resolve_attachments	and
				not lhs.pDepthStencilAttachment										== not rhs.pDepthStencilAttachment									and
				(not lhs.pDepthStencilAttachment or *lhs.pDepthStencilAttachment	== *rhs.pDepthStencilAttachment)									and
				PreserveView{lhs.pPreserveAttachments, lhs.preserveAttachmentCount}	== PreserveView{rhs.pPreserveAttachments, rhs.preserveAttachmentCount};
	}

/*
=================================================
	operator ==
=================================================
*/
	inline bool operator == (const VkSubpassDependency &lhs, const VkSubpassDependency &rhs)
	{
		return	lhs.srcSubpass		== rhs.srcSubpass		and
				lhs.dstSubpass		== rhs.dstSubpass		and
				lhs.srcStageMask	== rhs.srcStageMask		and
				lhs.dstStageMask	== rhs.dstStageMask		and
				lhs.srcAccessMask	== rhs.srcAccessMask	and
				lhs.dstAccessMask	== rhs.dstAccessMask	and
				lhs.dependencyFlags	== rhs.dependencyFlags;
	}
//-----------------------------------------------------------------------------


namespace AE::Graphics
{
/*
=================================================
	operator ==
=================================================
*/
	bool VRenderPass::operator == (const VRenderPass &rhs) const
	{
		SHAREDLOCK( _drCheck );
		SHAREDLOCK( rhs._drCheck );

		using AttachView	= ArrayView< VkAttachmentDescription >;
		using SubpassView	= ArrayView< VkSubpassDescription >;
		using DepsView		= ArrayView< VkSubpassDependency >;
		return	_hash																== rhs._hash																	and
				_attachmentHash														== rhs._attachmentHash															and
				_subpassesHash														== rhs._subpassesHash															and
				_createInfo.flags													== rhs._createInfo.flags														and
				AttachView{_createInfo.pAttachments, _createInfo.attachmentCount}	== AttachView{rhs._createInfo.pAttachments, rhs._createInfo.attachmentCount}	and
				SubpassView{_createInfo.pSubpasses, _createInfo.subpassCount}		== SubpassView{rhs._createInfo.pSubpasses, rhs._createInfo.subpassCount}		and
				DepsView{_createInfo.pDependencies, _createInfo.dependencyCount}	== DepsView{rhs._createInfo.pDependencies, rhs._createInfo.dependencyCount};
	}

}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
