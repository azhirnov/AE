// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef AE_ENABLE_VULKAN

# include "graphics/Public/RenderPassDesc.h"
# include "graphics/Vulkan/Resources/VLogicalRenderPass.h"

namespace AE::Graphics
{

	//
	// Vulkan Render Pass
	//

	class VRenderPass final
	{
	// types
	private:
		static constexpr uint	maxColorAttachments	= GraphicsConfig::MaxColorBuffers;
		static constexpr uint	maxAttachments		= GraphicsConfig::MaxAttachments;
		static constexpr uint	maxSubpasses		= GraphicsConfig::MaxRenderPassSubpasses;
		static constexpr uint	maxDependencies		= maxSubpasses * 2;

		using Attachments_t			= FixedArray< VkAttachmentDescription, maxAttachments >;
		using AttachmentsRef_t		= FixedArray< VkAttachmentReference, maxAttachments * maxSubpasses >;
		using AttachmentsRef2_t		= FixedArray< VkAttachmentReference, maxSubpasses >;
		using Subpasses_t			= FixedArray< VkSubpassDescription, maxSubpasses >;
		using Dependencies_t		= FixedArray< VkSubpassDependency, maxDependencies >;
		using Preserves_t			= FixedArray< uint, maxColorAttachments * maxSubpasses >;
		using SubpassesHash_t		= FixedArray< HashVal, maxSubpasses >;
		using SubpassOutput_t		= FixedArray< VRenderPassOutputID, maxSubpasses >;


	// variables
	private:
		VkRenderPass			_renderPass		= VK_NULL_HANDLE;

		HashVal					_hash;
		HashVal					_attachmentHash;
		SubpassesHash_t			_subpassesHash;
		SubpassOutput_t			_subpassOutput;
		
		VkRenderPassCreateInfo	_createInfo		= {};
		Attachments_t			_attachments;
		AttachmentsRef_t		_attachmentRef;
		AttachmentsRef_t		_inputAttachRef;
		AttachmentsRef2_t		_resolveAttachRef;
		Subpasses_t				_subpasses;
		Dependencies_t			_dependencies;
		Preserves_t				_preserves;
		
		DebugName_t				_debugName;
		
		RWDataRaceCheck			_drCheck;


	// methods
	public:
		VRenderPass () {}
		VRenderPass (ArrayView<VLogicalRenderPass*> logicalPasses);
		~VRenderPass ();

		bool Create (const VDevice &dev, StringView dbgName);
		void Destroy (VResourceManager &);

		ND_ bool operator == (const VRenderPass &rhs) const;

		ND_ VkRenderPass					Handle ()			const	{ SHAREDLOCK( _drCheck );  return _renderPass; }
		ND_ VkRenderPassCreateInfo const&	GetCreateInfo ()	const	{ SHAREDLOCK( _drCheck );  return _createInfo; }
		ND_ HashVal							GetHash ()			const	{ SHAREDLOCK( _drCheck );  return _hash; }
		ND_ SubpassOutput_t const			GetSubpassOutput ()	const	{ SHAREDLOCK( _drCheck );  return _subpassOutput; }


	private:
		bool  _Initialize (ArrayView<VLogicalRenderPass*> logicalPasses);
		void  _CalcHash (OUT HashVal &hash, OUT HashVal &attachmentHash, OUT SubpassesHash_t &subpassesHash) const;
	};


}	// AE::Graphics


namespace std
{
	template <>
	struct hash< AE::Graphics::VRenderPass > {
		ND_ size_t  operator () (const AE::Graphics::VRenderPass &value) const {
			return size_t(value.GetHash());
		}
	};

}	// std

#endif	// AE_ENABLE_VULKAN
