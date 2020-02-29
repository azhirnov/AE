// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef AE_ENABLE_VULKAN

# include "graphics/Public/RenderGraph.h"
# include "graphics/Vulkan/Resources/VCommandPool.h"
# include "graphics/Vulkan/Resources/VResourceMap.h"

namespace AE::Graphics
{

	//
	// Vulkan Baked Command Buffer
	//

	class VBakedCommandBuffer final
	{
	// types
	public:
		using Barriers_t	= Array< Pair< GfxResourceID, EResourceState >>;


	// variables
	private:
		VkCommandBuffer			_cmdBuf		= VK_NULL_HANDLE;
		VCommandPoolPtr			_cmdPool;
		RenderPassID			_renderPassId;

		mutable Barriers_t		_barriers;
		mutable VResourceMap	_resources;

		RWDataRaceCheck			_drCheck;


	// methods
	public:
		VBakedCommandBuffer () {}
		~VBakedCommandBuffer ();

		bool  Create (const VDevice &dev, const VCommandPoolPtr &pool, RenderPassID rp);
		void  Destroy (VResourceManager &resMngr);


		void  AddBarrier (GfxResourceID id, EResourceState state) const
		{
			EXLOCK( _drCheck );
			_barriers.emplace_back( id, state );
		}

		void  AddBarriers (ArrayView< Pair<GfxResourceID, EResourceState>> barriers) const
		{
			EXLOCK( _drCheck );
			_barriers.insert( _barriers.end(), barriers.begin(), barriers.end() );
		}

		void  SetResources (VResourceMap &&res) const
		{
			EXLOCK( _drCheck );
			_resources = std::move(res);
		}

		ND_ VkCommandBuffer		GetCommands ()		const	{ SHAREDLOCK( _drCheck );  return _cmdBuf; }
		ND_ RenderPassID		GetRenderPass ()	const	{ SHAREDLOCK( _drCheck );  return _renderPassId; }
		ND_ Barriers_t const&	Barriers ()			const	{ SHAREDLOCK( _drCheck );  return _barriers; }
		ND_ VResourceMap &		GetResources ()		const	{ SHAREDLOCK( _drCheck );  return _resources; }
	};
	

}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
