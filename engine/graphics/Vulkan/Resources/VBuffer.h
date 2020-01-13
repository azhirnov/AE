// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef AE_ENABLE_VULKAN

# include "graphics/Public/BufferDesc.h"
# include "graphics/Public/IDs.h"
# include "graphics/Vulkan/VQueue.h"

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
		using BufferViewMap_t	= HashMap< BufferViewDesc, VkBufferView >;
		//using OnRelease_t	= IFrameGraph::OnExternalBufferReleased_t;


	// variables
	private:
		VkBuffer					_buffer				= VK_NULL_HANDLE;
		BufferDesc					_desc;
		
		mutable SharedMutex			_viewMapLock;
		mutable BufferViewMap_t		_viewMap;
		
		UniqueID<MemoryID>			_memoryId;
		//VkAccessFlags				_readAccessMask		= 0;

		DebugName_t					_debugName;

		RWDataRaceCheck				_drCheck;


	// methods
	public:
		VBuffer () {}
		~VBuffer ();

		bool Create (VResourceManager &, const BufferDesc &desc, MemoryID memId, VMemoryObj &memObj, StringView dbgName);

		//bool Create (const VDevice &dev, const VulkanBufferDesc &desc, StringView dbgName, OnRelease_t &&onRelease);

		void Destroy (VResourceManager &);

		ND_ VkBufferView		GetView (const VDevice &dev, const BufferViewDesc &) const;
		
		//ND_ VulkanBufferDesc	GetApiSpecificDescription () const;

		ND_ bool				IsReadOnly ()			const;
		ND_ bool				IsOwnsHandle ()			const;

		ND_ VkBuffer			Handle ()				const	{ SHAREDLOCK( _drCheck );  return _buffer; }
		ND_ MemoryID			GetMemoryID ()			const	{ SHAREDLOCK( _drCheck );  return _memoryId; }

		ND_ BufferDesc const&	Description ()			const	{ SHAREDLOCK( _drCheck );  return _desc; }
		
		//ND_ VkAccessFlags		GetAllReadAccessMask ()	const	{ SHAREDLOCK( _drCheck );  return _readAccessMask; }

		ND_ bool				IsExclusiveSharing ()	const	{ SHAREDLOCK( _drCheck );  return _desc.queues == Default; }
		ND_ StringView			GetDebugName ()			const	{ SHAREDLOCK( _drCheck );  return _debugName; }


	private:
		bool _CreateView (const VDevice &, const BufferViewDesc &, OUT VkBufferView &) const;
	};
	

}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
