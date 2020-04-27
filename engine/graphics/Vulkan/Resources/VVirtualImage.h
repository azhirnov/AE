// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef AE_ENABLE_VULKAN

# include "graphics/Vulkan/VCommon.h"

namespace AE::Graphics
{

	//
	// Vulkan Virtual Image
	//

	class VVirtualImage
	{
	// variables
	private:
		VirtualImageDesc	_desc;

		DebugName_t			_debugName;
		
		RWDataRaceCheck		_drCheck;


	// methods
	public:
		VVirtualImage () {}
		~VVirtualImage () {}


		bool  Create (const VirtualImageDesc &desc, StringView dbgName)
		{
			EXLOCK( _drCheck );

			_desc		= desc;
			_debugName	= dbgName;
			return true;
		}


		void  Destroy (const VResourceManager &)
		{
			EXLOCK( _drCheck );
		}


		ND_ VirtualImageDesc const&		Description ()	const	{ SHAREDLOCK( _drCheck );  return _desc; }
		ND_ StringView					GetDebugName ()	const	{ SHAREDLOCK( _drCheck );  return _debugName; }
	};


}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
