// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef AE_ENABLE_VULKAN

#include "graphics/Public/RenderGraph.h"
#include "stl/Memory/LinearAllocator.h"

namespace AE::Graphics
{

	//
	// Vulkan Render Graph
	//

	class VRenderGraph final : public IRenderGraph
	{
	// types
	private:
		class VTransferContext;
		class VComputeContext;
		class VGraphicsContext;
		class VRenderContext;

		struct BaseCmd;
		struct RenderCmd;
		struct GraphicsCmd;
		struct ComputeCmd;
		struct TransferCmd;

		using VirtualResWrite_t		= StaticArray< BaseCmd*, 1u << 16 >;
		using VirtualResUsage_t		= StaticArray< EVirtualResourceUsage, 1u << 16 >;


	// variables
	private:
		LinearAllocator<>		_allocator;
		Array<BaseCmd *>		_commands;
		VirtualResWrite_t		_resWriteCmd;
		VirtualResUsage_t		_resUsage;


	// methods
	public:
		bool Add (EQueueType				queue,
				  VirtualResources_t		input,
				  VirtualResources_t		output,
				  RenderPassSetupFn_t&&		setup,
				  RenderPassDrawFn_t&&		pass,
				  StringView				dbgName) override;

		bool Add (EQueueType				queue,
				  VirtualResources_t		input,
				  VirtualResources_t		output,
				  GraphicsCommandFn_t&&		pass,
				  StringView				dbgName) override;

		bool Add (EQueueType				queue,
				  VirtualResources_t		input,
				  VirtualResources_t		output,
				  ComputeCommandFn_t&&		pass,
				  StringView				dbgName) override;

		bool Add (EQueueType				queue,
				  VirtualResources_t		input,
				  VirtualResources_t		output,
				  TransferCommandFn_t&&		pass,
				  StringView				dbgName) override;
		
		bool Flush () override;


	private:
		template <typename T>
		ND_ T*  _Add (EQueueType			queue,
					  VirtualResources_t	input,
					  VirtualResources_t	output,
					  StringView			dbgName);
	};


}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
