// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef AE_ENABLE_VULKAN

# include "graphics/Vulkan/VCommon.h"
# include "graphics/Private/EnumUtils.h"
# include "graphics/Vulkan/VResourceManager.h"
# include "graphics/Vulkan/VRenderGraph.h"
# include "graphics/Vulkan/VEnumCast.h"
# include "graphics/Vulkan/RenderGraph/VBarrierManager.h"

namespace AE::Graphics
{

	//
	// Dummy Local Resources Manager
	//

	class VDummyLocalResManager : public VBarrierManager
	{
	// types
	public:
		static constexpr bool	HasAutoSync = false;


	// variables
	private:
		VResourceManagerImpl &	_resMngr;


	// methods
	public:
		VDummyLocalResManager (const VCommandBatch &batch) :
			VBarrierManager{ batch },
			_resMngr{ RenderGraph().GetResourceManager() }
		{}

		void  TransitToDefaultState ()
		{}

		void  DescriptorsBarrier (DescriptorSetID, ArrayView<uint>)
		{
		}

		template <typename ID>
		ND_ auto*					Get (ID id)						{ return _resMngr.GetResource( id ); }
		
		ND_ VDevice const&			GetDevice ()			const	{ return _resMngr.GetDevice(); }
		ND_ VStagingBufferManager&	GetStagingManager ()	const	{ return _resMngr.GetStagingManager(); }
	};


}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
