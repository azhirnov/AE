// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Common.h"

namespace AE::STL
{
	struct UntypedAllocator;
	struct UntypedAlignedAllocator;

	template <typename T>
	struct StdAllocator;

	template <typename AllocatorType, uint MaxBlocks>
	struct LinearAllocator;

	template <typename T, typename AllocatorType, typename MaxBlocks>
	struct StdLinearAllocator;

	template <typename AllocatorType, uint MaxBlocks>
	struct UntypedLinearAllocator;

	template <typename AllocatorType, uint MaxBlocks>
	struct StackAllocator;

}	// AE::STL
