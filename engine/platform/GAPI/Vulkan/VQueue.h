// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef AE_ENABLE_VULKAN

# include "platform/GAPI/Vulkan/VulkanLoader.h"
# include "platform/GAPI/Public/Queue.h"

# include "stl/Algorithms/Cast.h"
# include "stl/Containers/FixedArray.h"
# include "stl/Containers/StaticString.h"
# include "stl/Containers/Ptr.h"
# include "stl/Math/Vec.h"
# include "threading/Common.h"

namespace AE::Vulkan
{
	using namespace AE::Graphics;

	using AE::Threading::Mutex;


	enum class EQueueFamily : uint
	{
		_Count		= 31,
		External	= VK_QUEUE_FAMILY_EXTERNAL,
		Foreign		= VK_QUEUE_FAMILY_FOREIGN_EXT,
		Ignored		= VK_QUEUE_FAMILY_IGNORED,
		Unknown		= Ignored,
	};

	// TODO: remove ?
	enum class EQueueFamilyMask : uint
	{
		All			= ~0u,
		Unknown		= 0,
	};
	AE_BIT_OPERATORS( EQueueFamilyMask );
	

	using DebugName_t	= StaticString<64>;
	using VQueuePtr		= Ptr< const struct VQueue >;



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
		VkQueueFlags		familyFlags		= {};
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
	

	
	forceinline EQueueFamilyMask&  operator |= (EQueueFamilyMask &lhs, EQueueFamily rhs)
	{
		ASSERT( uint(rhs) < 32 );
		return lhs = BitCast<EQueueFamilyMask>( uint(lhs) | SafeLeftBitShift( 1u, uint(rhs) ));
	}

	forceinline EQueueFamilyMask   operator |  (EQueueFamilyMask lhs, EQueueFamily rhs)
	{
		ASSERT( uint(rhs) < 32 );
		return BitCast<EQueueFamilyMask>( uint(lhs) | SafeLeftBitShift( 1u, uint(rhs) ));
	}

}	// AE::Vulkan

#endif	// AE_ENABLE_VULKAN
