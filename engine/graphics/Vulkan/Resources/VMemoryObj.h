// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef AE_ENABLE_VULKAN

# include "stl/Containers/UntypedStorage.h"
# include "graphics/Public/ResourceEnums.h"
# include "../VCommon.h"

namespace AE::Graphics
{

	//
	// Vulkan Memory Object
	//

	class VMemoryObj final
	{
	// types
	public:
		using Storage_t = UntypedStorage< sizeof(uint64_t) * 4, alignof(uint64_t) >;

		struct MemoryInfo
		{
			VkDeviceMemory			mem			= VK_NULL_HANDLE;
			VkMemoryPropertyFlags	flags		= 0;
			BytesU					offset;
			BytesU					size;
			void *					mappedPtr	= null;
		};


	// variables
	private:
		Storage_t				_storage;
		DebugName_t				_debugName;
		
		RWDataRaceCheck			_drCheck;


	// methods
	public:
		VMemoryObj () {}
		VMemoryObj (VMemoryObj &&) = default;
		~VMemoryObj ();

		bool Create (EMemoryType memType, StringView dbgName);
		void Destroy (VResourceManager &);

		//bool AllocateForImage (VMemoryManager &, VkImage);
		//bool AllocateForBuffer (VMemoryManager &, VkBuffer);
		//bool AllocateForAccelStruct (VMemoryManager &, VkAccelerationStructureNV);

		//bool GetInfo (VMemoryManager &, OUT MemoryInfo &) const;

		//ND_ MemoryDesc const&	Description ()	const	{ SHAREDLOCK( _drCheck );  return _desc; }
		//ND_ EMemoryTypeExt	MemoryType ()		const	{ SHAREDLOCK( _drCheck );  return EMemoryTypeExt(_desc.type); }
	};


}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
