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

	// TODO: remove ?
	enum class EQueueFamilyMask : uint
	{
		All			= ~0u,
		Unknown		= 0,
	};
	AE_BIT_OPERATORS( EQueueFamilyMask );
	

	using DebugName_t	= FixedString<64>;
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

}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
