// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef AE_ENABLE_VULKAN
# include "graphics/Vulkan/VRenderGraph.h"

# include "graphics/Vulkan/RenderGraph/VDrawContext.h"
# include "graphics/Vulkan/RenderGraph/VTransferContext.h"
# include "graphics/Vulkan/RenderGraph/VComputeContext.h"
# include "graphics/Vulkan/RenderGraph/VGraphicsContext.h"

# include "graphics/Vulkan/RenderGraph/VDummyLocalResManager.h"
# include "graphics/Vulkan/RenderGraph/VLocalResManager.h"
# include "graphics/Vulkan/RenderGraph/VLocalResRangesManager.h"

# include "graphics/Vulkan/RenderGraph/VCmdBufferDebugger.h"


namespace AE::Graphics::_hidden_
{
	
	// transfer context
	template <typename ResMngrType>
	using _VDirectTransferCtxImpl	= _VTransferContextImpl< _VDirectTransferCtx<ResMngrType>, ITransferContext >;
	
	template <EBarrierPlacement E>	struct VDirectTransferCtx;
	template <>						struct VDirectTransferCtx< EBarrierPlacement::Manual >				{ using type = _VDirectTransferCtxImpl< VDummyLocalResManager  >; };
	template <>						struct VDirectTransferCtx< EBarrierPlacement::PerResource >			{ using type = _VDirectTransferCtxImpl< VLocalResManager       >; };
	template <>						struct VDirectTransferCtx< EBarrierPlacement::PerRange >			{ using type = _VDirectTransferCtxImpl< VLocalResRangesManager >; };
	//template <>					struct VDirectTransferCtx< EBarrierPlacement::CommandReordering >	{ using type = _VDirectTransferCtxImpl<  >; };

	template <typename ResMngrType>
	using _VIndirectTransferCtxImpl	= _VTransferContextImpl< _VIndirectCtxHelper< ResMngrType, _VBaseIndirectTrasferCtx >, ITransferContext >;
	
	template <EBarrierPlacement E>	struct VIndirectTransferCtx;
	template <>						struct VIndirectTransferCtx< EBarrierPlacement::Manual >			{ using type = _VIndirectTransferCtxImpl< VDummyLocalResManager  >; };
	template <>						struct VIndirectTransferCtx< EBarrierPlacement::PerResource >		{ using type = _VIndirectTransferCtxImpl< VLocalResManager       >; };
	template <>						struct VIndirectTransferCtx< EBarrierPlacement::PerRange >			{ using type = _VIndirectTransferCtxImpl< VLocalResRangesManager >; };
	//template <>					struct VIndirectTransferCtx< EBarrierPlacement::CommandReordering >	{ using type = _VIndirectTransferCtxImpl<  >; };


	// compute context
	template <typename ResMngrType>
	using _VDirectComputeCtxImpl	= _VComputeContextImpl< _VDirectComputeCtx<ResMngrType>, IComputeContext >;
	
	template <EBarrierPlacement E>	struct VDirectComputeCtx;
	template <>						struct VDirectComputeCtx< EBarrierPlacement::Manual >				{ using type = _VDirectComputeCtxImpl< VDummyLocalResManager  >; };
	template <>						struct VDirectComputeCtx< EBarrierPlacement::PerResource >			{ using type = _VDirectComputeCtxImpl< VLocalResManager       >; };
	template <>						struct VDirectComputeCtx< EBarrierPlacement::PerRange >				{ using type = _VDirectComputeCtxImpl< VLocalResRangesManager >; };
	//template <>					struct VDirectComputeCtx< EBarrierPlacement::CommandReordering >	{ using type = _VDirectComputeCtxImpl<  >; };

	template <typename ResMngrType>
	using _VIndirectComputeCtxImpl	= _VComputeContextImpl< _VIndirectCtxHelper< ResMngrType, _VBaseIndirectComputeCtx >, IComputeContext >;
	
	template <EBarrierPlacement E>	struct VIndirectComputeCtx;
	template <>						struct VIndirectComputeCtx< EBarrierPlacement::Manual >				{ using type = _VIndirectComputeCtxImpl< VDummyLocalResManager  >; };
	template <>						struct VIndirectComputeCtx< EBarrierPlacement::PerResource >		{ using type = _VIndirectComputeCtxImpl< VLocalResManager       >; };
	template <>						struct VIndirectComputeCtx< EBarrierPlacement::PerRange >			{ using type = _VIndirectComputeCtxImpl< VLocalResRangesManager >; };
	//template <>					struct VIndirectComputeCtx< EBarrierPlacement::CommandReordering >	{ using type = _VIndirectComputeCtxImpl<  >; };


	// graphics context
	template <typename ResMngrType>
	using _VDirectGraphicsCtxImpl	= _VGraphicsContextImpl< _VDirectGraphicsCtx<ResMngrType>, IGraphicsContext >;
	
	template <EBarrierPlacement E>	struct VDirectGraphicsCtx;
	template <>						struct VDirectGraphicsCtx< EBarrierPlacement::Manual >				{ using type = _VDirectGraphicsCtxImpl< VDummyLocalResManager  >; };
	template <>						struct VDirectGraphicsCtx< EBarrierPlacement::PerResource >			{ using type = _VDirectGraphicsCtxImpl< VLocalResManager       >; };
	template <>						struct VDirectGraphicsCtx< EBarrierPlacement::PerRange >			{ using type = _VDirectGraphicsCtxImpl< VLocalResRangesManager >; };
	//template <>					struct VDirectGraphicsCtx< EBarrierPlacement::CommandReordering >	{ using type = _VDirectGraphicsCtxImpl<  >; };

	template <typename ResMngrType>
	using _VIndirectGraphicsCtxImpl	= _VGraphicsContextImpl< _VIndirectCtxHelper< ResMngrType, _VBaseIndirectGraphicsCtx >, IGraphicsContext >;
	
	template <EBarrierPlacement E>	struct VIndirectGraphicsCtx;
	template <>						struct VIndirectGraphicsCtx< EBarrierPlacement::Manual >			{ using type = _VIndirectGraphicsCtxImpl< VDummyLocalResManager  >; };
	template <>						struct VIndirectGraphicsCtx< EBarrierPlacement::PerResource >		{ using type = _VIndirectGraphicsCtxImpl< VLocalResManager       >; };
	template <>						struct VIndirectGraphicsCtx< EBarrierPlacement::PerRange >			{ using type = _VIndirectGraphicsCtxImpl< VLocalResRangesManager >; };
	//template <>					struct VIndirectGraphicsCtx< EBarrierPlacement::CommandReordering >	{ using type = _VIndirectGraphicsCtxImpl<  >; };


	// draw context
	template <typename ResMngrType>
	using _VDirectDrawCtxImpl		= _VDrawContextImpl< _VDirectDrawCtx<ResMngrType> >;
	
	template <EBarrierPlacement E>	struct VDirectDrawCtx;
	template <>						struct VDirectDrawCtx< EBarrierPlacement::Manual >				{ using type = _VDirectDrawCtxImpl< VDummyLocalResManager  >; };
	//template <>					struct VDirectDrawCtx< EBarrierPlacement::PerResource >			{ using type = _VDirectDrawCtxImpl< VLocalResManager       >; };
	//template <>					struct VDirectDrawCtx< EBarrierPlacement::PerRange >			{ using type = _VDirectDrawCtxImpl< VLocalResRangesManager >; };
	//template <>					struct VDirectDrawCtx< EBarrierPlacement::CommandReordering >	{ using type = _VDirectDrawCtxImpl<  >; };

	template <typename ResMngrType>
	using _VIndirectDrawCtxImpl		= _VDrawContextImpl< _VDrawCtxHelper< ResMngrType, _VBaseIndirectDrawCtx >>;

	template <EBarrierPlacement E>	struct VIndirectDrawCtx;
	template <>						struct VIndirectDrawCtx< EBarrierPlacement::Manual >			{ using type = _VIndirectDrawCtxImpl< VDummyLocalResManager  >; };
	//template <>					struct VIndirectDrawCtx< EBarrierPlacement::PerResource >		{ using type = _VIndirectDrawCtxImpl< VLocalResManager       >; };
	//template <>					struct VIndirectDrawCtx< EBarrierPlacement::PerRange >			{ using type = _VIndirectDrawCtxImpl< VLocalResRangesManager >; };
	//template <>					struct VIndirectDrawCtx< EBarrierPlacement::CommandReordering >	{ using type = _VIndirectDrawCtxImpl<  >; };


}	// AE::Graphics::_hidden_


namespace AE::Graphics
{
	// Direct draw context
	template <EBarrierPlacement E>	using VDirectDrawContext		= typename Graphics::_hidden_::VDirectDrawCtx<E>::type;
	using VDirectRenderContextNoBarriers							= VDirectDrawContext< EBarrierPlacement::Manual >;
	
	// Indirect draw context
	template <EBarrierPlacement E>	using VIndirectDrawContext		= typename Graphics::_hidden_::VIndirectDrawCtx<E>::type;
	using VIndirectRenderContextNoBarriers							= VIndirectDrawContext< EBarrierPlacement::Manual >;
	

	// Direct transfer context
	template <EBarrierPlacement E>	using VDirectTransferContext	= typename Graphics::_hidden_::VDirectTransferCtx<E>::type;
	using VDirectTransferContextNoBarriers							= VDirectTransferContext< EBarrierPlacement::Manual >;
	using VDirectTransferContextWithBarriers						= VDirectTransferContext< EBarrierPlacement::PerResource >;
	using VDirectTransferContextWithRangeBarriers					= VDirectTransferContext< EBarrierPlacement::PerRange >;
	
	// Indirect transfer context
	template <EBarrierPlacement E>	using VIndirectTransferContext	= typename Graphics::_hidden_::VIndirectTransferCtx<E>::type;
	using VIndirectTransferContextNoBarriers						= VIndirectTransferContext< EBarrierPlacement::Manual >;
	using VIndirectTransferContextWithBarriers						= VIndirectTransferContext< EBarrierPlacement::PerResource >;
	using VIndirectTransferContextWithRangeBarriers					= VIndirectTransferContext< EBarrierPlacement::PerRange >;

	// Direct compute context
	template <EBarrierPlacement E>	using VDirectComputeContext		= typename Graphics::_hidden_::VDirectComputeCtx<E>::type;
	using VDirectComputeContextNoBarriers							= VDirectComputeContext< EBarrierPlacement::Manual >;
	using VDirectComputeContextWithBarriers							= VDirectComputeContext< EBarrierPlacement::PerResource >;
	using VDirectComputeContextWithRangeBarriers					= VDirectComputeContext< EBarrierPlacement::PerRange >;

	// Indirect compute context
	template <EBarrierPlacement E>	using VIndirectComputeContext	= typename Graphics::_hidden_::VIndirectComputeCtx<E>::type;
	using VIndirectComputeContextNoBarriers							= VIndirectComputeContext< EBarrierPlacement::Manual >;
	using VIndirectComputeContextWithBarriers						= VIndirectComputeContext< EBarrierPlacement::PerResource >;
	using VIndirectComputeContextWithRangeBarriers					= VIndirectComputeContext< EBarrierPlacement::PerRange >;

	// Direct graphics context
	template <EBarrierPlacement E>	using VDirectGraphicsContext	= typename Graphics::_hidden_::VDirectGraphicsCtx<E>::type;
	using VDirectGraphicsContextNoBarriers							= VDirectGraphicsContext< EBarrierPlacement::Manual >;
	using VDirectGraphicsContextWithBarriers						= VDirectGraphicsContext< EBarrierPlacement::PerResource >;
	using VDirectGraphicsContextWithRangeBarriers					= VDirectGraphicsContext< EBarrierPlacement::PerRange >;

	// Indirect graphics context
	template <EBarrierPlacement E>	using VIndirectGraphicsContext	= typename Graphics::_hidden_::VIndirectGraphicsCtx<E>::type;
	using VIndirectGraphicsContextNoBarriers						= VIndirectGraphicsContext< EBarrierPlacement::Manual >;
	using VIndirectGraphicsContextWithBarriers						= VIndirectGraphicsContext< EBarrierPlacement::PerResource >;
	using VIndirectGraphicsContextWithRangeBarriers					= VIndirectGraphicsContext< EBarrierPlacement::PerRange >;


}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
