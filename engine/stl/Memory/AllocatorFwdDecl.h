// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Common.h"

namespace AE::STL
{
	struct UntypedAllocator;
	struct UntypedAlignedAllocator;

	template <typename T>
	struct StdAllocator;

	template <typename AllocatorType, uint MaxBlocks, bool ThreadSafe>
	struct LinearAllocator;

	template <typename T, typename AllocatorType, typename MaxBlocks>
	struct StdLinearAllocator;

	template <typename AllocatorType, uint MaxBlocks, bool ThreadSafe>
	struct UntypedLinearAllocator;

	template <typename AllocatorType, uint MaxBlocks, bool ThreadSafe>
	struct StackAllocator;

}	// AE::STL
