// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Math/Vec.h"
#include "stl/Math/Rectangle.h"
#include "stl/Math/Color.h"

#include "stl/Containers/FixedString.h"
#include "stl/Containers/FixedMap.h"
#include "stl/Containers/Union.h"

#include "stl/Stream/Stream.h"
#include "stl/Types/NamedID.h"
#include "stl/Types/NamedID_HashCollisionCheck.h"

#include "stl/Algorithms/ArrayUtils.h"

#include "threading/Primitives/DummyLock.h"

namespace AE::Serializing
{
	using namespace AE::STL;

	using SharedMutex = Threading::SharedMutex;
	
	using SerializedID = NamedID< 32, 0x400, AE_OPTIMIZE_IDS, UMax >;

}	// AE::Serializing
