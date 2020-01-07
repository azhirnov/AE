// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef AE_ENABLE_VULKAN

# include "graphics/Public/BufferDesc.h"
# include "graphics/Public/IDs.h"
# include "../VQueue.h"

namespace AE::Graphics
{

	//
	// Vulkan Buffer immutable data
	//

	class VBuffer final
	{
		friend class VBufferUnitTest;

	// types
	private:
		//using OnRelease_t	= IFrameGraph::OnExternalBufferReleased_t;


	// variables
	private:
		VkBuffer				_buffer				= VK_NULL_HANDLE;
		MemoryID				_memoryId;
		BufferDesc				_desc;
		EQueueFamilyMask		_queueFamilyMask	= Default;
		VkAccessFlags			_readAccessMask		= 0;

		DebugName_t				_debugName;
		//OnRelease_t				_onRelease;

		RWDataRaceCheck			_drCheck;


	// methods
	public:
		VBuffer () {}
		VBuffer (VBuffer &&) = default;
		~VBuffer ();

		bool Create (VResourceManager &, const BufferDesc &desc, MemoryID memId, VMemoryObj &memObj,
					 EQueueFamilyMask queueFamilyMask, StringView dbgName);

		//bool Create (const VDevice &dev, const VulkanBufferDesc &desc, StringView dbgName, OnRelease_t &&onRelease);

		void Destroy (VResourceManager &);

		//void Merge (BufferViewMap_t &, OUT AppendableVkResources_t) const;

		//ND_ VkBufferView		GetView (const HashedBufferViewDesc &) const;
		
		//ND_ VulkanBufferDesc	GetApiSpecificDescription () const;

		ND_ bool				IsReadOnly ()			const;
		ND_ bool				IsOwnsHandle ()			const;

		ND_ VkBuffer			Handle ()				const	{ SHAREDLOCK( _drCheck );  return _buffer; }
		ND_ MemoryID			GetMemoryID ()			const	{ SHAREDLOCK( _drCheck );  return _memoryId; }

		ND_ BufferDesc const&	Description ()			const	{ SHAREDLOCK( _drCheck );  return _desc; }
		ND_ BytesU				Size ()					const	{ SHAREDLOCK( _drCheck );  return _desc.size; }
		
		ND_ VkAccessFlags		GetAllReadAccessMask ()	const	{ SHAREDLOCK( _drCheck );  return _readAccessMask; }

		ND_ bool				IsExclusiveSharing ()	const	{ SHAREDLOCK( _drCheck );  return _queueFamilyMask == Default; }
		ND_ EQueueFamilyMask	GetQueueFamilyMask ()	const	{ SHAREDLOCK( _drCheck );  return _queueFamilyMask; }
		ND_ StringView			GetDebugName ()			const	{ SHAREDLOCK( _drCheck );  return _debugName; }
	};
	

}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
