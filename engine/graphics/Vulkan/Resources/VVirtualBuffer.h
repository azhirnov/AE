// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef AE_ENABLE_VULKAN

# include "graphics/Vulkan/VCommon.h"

namespace AE::Graphics
{

	//
	// Vulkan Virtual Buffer
	//

	class VVirtualBuffer
	{
	// variables
	private:
		VirtualBufferDesc	_desc;

		DebugName_t			_debugName;


	// methods
	public:
		VVirtualBuffer () {}
		~VVirtualBuffer () {}


		bool Create (const VirtualBufferDesc &desc, StringView dbgName)
		{
			_desc		= desc;
			_debugName	= dbgName;
			return true;
		}


		void Destroy (const VResourceManager &)
		{}


		ND_ VirtualBufferDesc const&	Description ()	const	{ return _desc; }
	};


}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
