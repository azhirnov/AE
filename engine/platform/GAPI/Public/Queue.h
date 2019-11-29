#pragma once

#include "platform/GAPI/Public/Common.h"

namespace AE::Graphics
{

	enum class EQueueType : uint
	{
		Graphics,			// also supports compute and transfer commands
		AsyncCompute,		// separate compute queue
		AsyncTransfer,		// separate transfer queue
		//Present,			// queue must support present, may be a separate queue
		_Count,
		Unknown			= ~0u,
	};

}	// AE::Graphics
