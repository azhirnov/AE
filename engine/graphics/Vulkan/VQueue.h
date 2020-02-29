// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef AE_ENABLE_VULKAN

# include "graphics/Public/Queue.h"
# include "graphics/Vulkan/VCommon.h"

namespace AE::Graphics
{
	enum class EQueueFamily : uint
	{
		_Count		= 31,
		External	= VK_QUEUE_FAMILY_EXTERNAL,
		Foreign		= VK_QUEUE_FAMILY_FOREIGN_EXT,
		Ignored		= VK_QUEUE_FAMILY_IGNORED,
		Unknown		= Ignored,
	};

	using DebugName_t			= FixedString<64>;
	using VQueuePtr				= Ptr< const struct VQueue >;
	using VQueueFamilyIndices_t	= FixedArray< uint, uint(EQueueType::_Count) >;



	//
	// Vulkan Queue
	//

	struct VQueue
	{
	// variables
		mutable Mutex		guard;			// use when call vkQueueSubmit, vkQueueWaitIdle, vkQueueBindSparse, vkQueuePresentKHR
		VkQueue				handle			= VK_NULL_HANDLE;
		EQueueType			type			= Default;
		EQueueFamily		familyIndex		= Default;
		VkQueueFlagBits		familyFlags		= Zero;
		uint				queueIndex		= UMax;
		float				priority		= 0.0f;
		uint3				minImageTransferGranularity;
		DebugName_t			debugName;
		
	// methods
		VQueue () {}

		VQueue (VQueue &&other) :
			handle{other.handle}, type{other.type}, familyIndex{other.familyIndex}, familyFlags{other.familyFlags},
			queueIndex{other.queueIndex}, priority{other.priority}, minImageTransferGranularity{other.minImageTransferGranularity},
			debugName{other.debugName}
		{}
	};
	

}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
