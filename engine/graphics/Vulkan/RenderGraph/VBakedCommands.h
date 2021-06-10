// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef AE_ENABLE_VULKAN
# include "graphics/Vulkan/VCommon.h"

namespace AE::Graphics
{

	//
	// Vulkan Baked Commands base class
	//

	class VBakedCommands
	{
	// types
	public:
		using Allocator_t	= UntypedAllocatorBaseAlign<8>;
		using ExecuteFn_t	= bool (*) (VulkanDeviceFn fn, VkCommandBuffer cmdbuf, void* root);


	// variables
	protected:
		void*			_root	= null;
		ExecuteFn_t		_exec	= null;


	// methods
	public:
		VBakedCommands ()
		{}

		VBakedCommands (void* root, ExecuteFn_t exec) :
			_root{ root },
			_exec{ exec }
		{}

		VBakedCommands (VBakedCommands &&other) :
			_root{ other._root },
			_exec{ other._exec }
		{
			other._root = null;
		}

		void  Destroy ()
		{
			if ( _root )
			{
				Allocator_t::Deallocate( _root );
				_root = null;
			}
		}
		
		bool  Execute (VulkanDeviceFn fn, VkCommandBuffer cmdbuf) const
		{
			if ( _root == null or _exec == null )
				return false;

			return _exec( fn, cmdbuf, _root );
		}

		ND_ bool  IsValid () const
		{
			return _root != null;
		}
	};


}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
