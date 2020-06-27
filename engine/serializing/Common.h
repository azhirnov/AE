// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Math/Vec.h"
#include "stl/Math/Rectangle.h"
#include "stl/Math/Color.h"

#include "stl/Containers/FixedString.h"
#include "stl/Containers/FixedMap.h"
#include "stl/Containers/Union.h"

#include "stl/Stream/Stream.h"
#include "stl/Utils/NamedID.h"
#include "stl/Utils/NamedID_HashCollisionCheck.h"

#include "stl/Algorithms/ArrayUtils.h"

#include "threading/Primitives/DummyLock.h"

// for NamedID
#define AE_SERIALIZE_HASH_ONLY	1

namespace AE::Serializing
{
	using namespace AE::STL;

	using SharedMutex = Threading::SharedMutex;
	
	using SerializedID = NamedID< 32, 0x400, AE_OPTIMIZE_IDS, UMax >;
	
	struct Serializer;
	struct Deserializer;
	class ObjectFactory;

}	// AE::Serializing
