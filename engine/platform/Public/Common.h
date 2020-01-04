// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "threading/TaskSystem/TaskScheduler.h"
#include "threading/Primitives/DataRaceCheck.h"

#include "stl/Containers/FixedArray.h"
#include "stl/Containers/FixedMap.h"
#include "stl/Algorithms/ArrayUtils.h"

#include "stl/Math/Vec.h"
#include "stl/Math/Rectangle.h"

#include "stl/Stream/Stream.h"
#include "stl/CompileTime/TypeList.h"
#include "stl/Types/Noncopyable.h"


#ifdef __INTELLISENSE__
#	define PLATFORM_ANDROID
#endif

namespace AE::App
{
	using namespace AE::STL;

	using Threading::Atomic;
	using Threading::EMemoryOrder;
	using Threading::ThreadID;
	using Threading::DataRaceCheck;

	
	class IApplication;
	class IWindow;
	class IVRDevice;


	using ApplicationPtr	= SharedPtr< IApplication >;
	using WindowPtr			= SharedPtr< IWindow >;
	using VRDevicePtr		= SharedPtr< IVRDevice >;


}	// AE::App


// check definitions
#ifdef AE_CPP_DETECT_MISSMATCH

#  ifdef AE_ENABLE_GLFW
#	pragma detect_mismatch( "AE_ENABLE_GLFW", "1" )
#  else
#	pragma detect_mismatch( "AE_ENABLE_GLFW", "0" )
#  endif

#endif	// AE_CPP_DETECT_MISSMATCH
