// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef AE_ENABLE_VULKAN

# include "graphics/Public/BufferDesc.h"
# include "graphics/Public/GfxMemAllocator.h"
# include "graphics/Public/IDs.h"
# include "graphics/Vulkan/VQueue.h"
# include "graphics/Public/NativeTypes.Vulkan.h"

namespace AE::Graphics
{

	//
	// Vulkan Buffer immutable data
	//

	class VBuffer final
	{
	// types
	private:
		using BufferViewMap_t	= HashMap< BufferViewDesc, VkBufferView >;
		using MemStorage_t		= IGfxMemAllocator::Storage_t;


	// variables
	private:
		VkBuffer					_buffer				= VK_NULL_HANDLE;
		BufferDesc					_desc;
		
		mutable SharedMutex			_viewMapLock;
		mutable BufferViewMap_t		_viewMap;
		
		//VkAccessFlags				_readAccessMask		= 0;

		GfxMemAllocatorPtr			_memAllocator;
		MemStorage_t				_memStorage;

		DebugName_t					_debugName;
		bool						_canBeDestroyed		= true;

		RWDataRaceCheck				_drCheck;


	// methods
	public:
		VBuffer () {}
		~VBuffer ();

		bool  Create (VResourceManager &, const BufferDesc &desc, GfxMemAllocatorPtr allocator, StringView dbgName);
		bool  Create (const VDevice &dev, const VulkanBufferDesc &desc, StringView dbgName);

		void  Destroy (VResourceManager &);
		
		bool  GetMemoryInfo (OUT VResourceMemoryInfo &) const;
		bool  GetMemoryInfo (OUT IGfxMemAllocator::NativeMemInfo_t &) const;

		ND_ VkBufferView		GetView (const VDevice &dev, const BufferViewDesc &) const;
		
		ND_ VulkanBufferDesc	GetNativeDescription () const;

		ND_ bool				IsReadOnly ()			const;
		ND_ bool				IsOwnsHandle ()			const;

		ND_ VkBuffer			Handle ()				const	{ SHAREDLOCK( _drCheck );  return _buffer; }
		ND_ BufferDesc const&	Description ()			const	{ SHAREDLOCK( _drCheck );  return _desc; }
		//ND_ VkAccessFlags		GetAllReadAccessMask ()	const	{ SHAREDLOCK( _drCheck );  return _readAccessMask; }

		ND_ bool				IsExclusiveSharing ()	const	{ SHAREDLOCK( _drCheck );  return _desc.queues == Default; }
		ND_ StringView			GetDebugName ()			const	{ SHAREDLOCK( _drCheck );  return _debugName; }

		
		ND_ static bool	IsSupported (const VDevice &dev, const BufferDesc &desc);
		ND_ bool		IsSupported (const VDevice &dev, const BufferViewDesc &desc) const;


	private:
		bool  _CreateView (const VDevice &, const BufferViewDesc &, OUT VkBufferView &) const;
	};
	

}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
