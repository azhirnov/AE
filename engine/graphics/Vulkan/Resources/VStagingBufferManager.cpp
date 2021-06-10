// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#ifdef AE_ENABLE_VULKAN

# include "stl/Algorithms/StringUtils.h"

# include "graphics/Vulkan/Resources/VStagingBufferManager.h"
# include "graphics/Vulkan/VResourceManager.h"
# include "graphics/Vulkan/VEnumCast.h"
# include "graphics/Private/EnumToString.h"

namespace AE::Graphics
{
	
/*
=================================================
	constructor
=================================================
*/
	VStagingBufferManager::VStagingBufferManager (VResourceManagerImpl &resMngr) :
		_resMngr{ resMngr }
	{
		STATIC_ASSERT( SizePerQueue_t{}.max_size() == QueueCount );
	}
	
/*
=================================================
	destructor
=================================================
*/
	VStagingBufferManager::~VStagingBufferManager ()
	{
		CHECK( _staticMemoryForWrite == VK_NULL_HANDLE );
		CHECK( _staticMemoryForRead  == VK_NULL_HANDLE );
		CHECK( _staticBuffersForWrite.empty() );
		CHECK( _staticBuffersForRead.empty() );
	}

/*
=================================================
	Initialize
=================================================
*/
	bool  VStagingBufferManager::Initialize (const GraphicsCreateInfo &info)
	{
		Deinitialize();

		CHECK_ERR( _CheckHostVisibleMemory( OUT _maxHostMemory ));
		CHECK_ERR( _CreateStaticBuffers( info ));

		return true;
	}
	
/*
=================================================
	Deinitialize
=================================================
*/
	void  VStagingBufferManager::Deinitialize ()
	{
		auto&	dev = _resMngr.GetDevice();

		if ( _staticMemoryForWrite != VK_NULL_HANDLE )
		{
			dev.vkUnmapMemory( dev.GetVkDevice(), _staticMemoryForWrite );
			dev.vkFreeMemory( dev.GetVkDevice(), _staticMemoryForWrite, null );
			_staticMemoryForWrite		= VK_NULL_HANDLE;
			_staticMemoryFlagsForWrite	= Zero;
		}

		if ( _staticMemoryForRead != VK_NULL_HANDLE )
		{
			dev.vkUnmapMemory( dev.GetVkDevice(), _staticMemoryForRead );
			dev.vkFreeMemory( dev.GetVkDevice(), _staticMemoryForRead, null );
			_staticMemoryForRead		= VK_NULL_HANDLE;
			_staticMemoryFlagsForRead	= Zero;
		}

		for (auto& sb : _staticBuffersForWrite) {
			_resMngr.ReleaseResource( sb.buffer );
		}
		for (auto& sb : _staticBuffersForRead) {
			_resMngr.ReleaseResource( sb.buffer );
		}
		_staticBuffersForWrite.clear();
		_staticBuffersForRead.clear();

		_writeStaticSize		= Default;
		_readStaticSize			= Default;
		_maxHostMemory			= 0_b;
		_maxWriteDynamicSize	= 0_b;
		_maxReadDynamicSize		= 0_b;
	}
	
/*
=================================================
	OnNextFrame
=================================================
*/
	void  VStagingBufferManager::OnNextFrame (uint frameId)
	{
		for (uint q = 0; q < QueueCount; ++q)
		{
			const uint	i = frameId * QueueCount + q;

			_staticBuffersForWrite[i].size.store( 0, EMemoryOrder::Relaxed );
			_staticBuffersForRead[i].size.store( 0, EMemoryOrder::Relaxed );
		}
	}
	
/*
=================================================
	GetStagingBuffer
----
	reqSize		- needed memory size, but allocation can be lesser
	blockSize	- granularity of memory allocation
	offsetAlign - alignment of start address in buffer
=================================================
*/
	bool  VStagingBufferManager::GetStagingBuffer (OUT StagingBufferResult& res, Bytes reqSize, Bytes blockSize, Bytes offsetAlign,
												   uint frameId, EStagingHeapType heap, EQueueType queue, bool upload)
	{
		ASSERT( uint(queue) < QueueCount );
		ASSERT( reqSize >= blockSize );

		const auto	AllocStatic = [reqSize, blockSize, offsetAlign] (OUT StagingBufferResult& res, StaticBuffer& sb) -> bool
		{
			uint	expected	= 0;
			uint	new_size	= 0;
			uint	offset		= 0;

			do {
				if ( expected + uint(blockSize) > sb.capacity )
					return false;

				offset		= AlignToLarger( expected, uint(offsetAlign) );
				new_size	= Min( uint(reqSize), AlignToSmaller( sb.capacity - offset, uint(blockSize) ));

				if ( new_size == 0 )
					return false;
			}
			while ( not sb.size.compare_exchange_weak( INOUT expected, new_size ));

			res.buffer			= sb.bufferHandle;
			res.bufferOffset	= Bytes{ offset };
			res.mapped			= sb.mapped + res.bufferOffset;
			res.memoryOffset	= Bytes{ sb.memOffset + offset };
			res.size			= Bytes{ new_size };
			return true;
		};

		BEGIN_ENUM_CHECKS();
		switch ( heap )
		{
			case EStagingHeapType::Static :
			{
				if ( upload )
				{
					const uint	i	= frameId * QueueCount + uint(queue);
					auto&		sb	= _staticBuffersForWrite[i];
				
					res.memory		= _staticMemoryForWrite;
					res.memFlags	= _staticMemoryFlagsForWrite;
					return AllocStatic( OUT res, sb );
				}
				else
				{
					const uint	i	= frameId * QueueCount + uint(queue);
					auto&		sb	= _staticBuffersForRead[i];

					res.memory		= _staticMemoryForRead;
					res.memFlags	= _staticMemoryFlagsForRead;
					return AllocStatic( OUT res, sb );
				}
			}

			case EStagingHeapType::Dynamic :
			{
				RETURN_ERR( "not supported yet" );
			}
		}
		END_ENUM_CHECKS();
		RETURN_ERR( "unknown staging heap type" );
	}

/*
=================================================
	_CheckHostVisibleMemory
=================================================
*/
	bool  VStagingBufferManager::_CheckHostVisibleMemory (OUT Bytes& totalHostMem) const
	{
		auto&	dev		= _resMngr.GetDevice();
		auto&	props	= dev.GetProperties().memoryProperties;
		
		VkMemoryRequirements	mem_req = {};
		{
			VkBuffer			id;
			VkBufferCreateInfo	info = {};
			info.sType	= VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			info.pNext	= null;
			info.flags	= 0;
			info.usage	= VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			info.size	= 64 << 10;
			info.sharingMode			= VK_SHARING_MODE_EXCLUSIVE;
			info.pQueueFamilyIndices	= null;
			info.queueFamilyIndexCount	= 0;

			VK_CHECK( dev.vkCreateBuffer( dev.GetVkDevice(), &info, null, OUT &id ));
			dev.vkGetBufferMemoryRequirements( dev.GetVkDevice(), id, OUT &mem_req );
			dev.vkDestroyBuffer( dev.GetVkDevice(), id, null );
		}

		BitSet<VK_MAX_MEMORY_HEAPS>		cached_heaps;
		BitSet<VK_MAX_MEMORY_HEAPS>		cocherent_heaps;

		for (uint i = 0; i < props.memoryTypeCount; ++i)
		{
			auto&	mt = props.memoryTypes[i];

			if ( AllBits( mem_req.memoryTypeBits, 1u << i ))
			{
				if ( AllBits( mt.propertyFlags, VK_MEMORY_PROPERTY_HOST_CACHED_BIT ))
					cached_heaps[ mt.heapIndex ] = true;
			
				if ( AllBits( mt.propertyFlags, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT ))
					cocherent_heaps[ mt.heapIndex ] = true;
			}
		}

		totalHostMem = 0_b;
		for (uint i = 0; i < props.memoryHeapCount; ++i)
		{
			if ( cached_heaps[i] or cocherent_heaps[i] )
				totalHostMem += props.memoryHeaps[i].size;
		}

		return true;
	}
	
/*
=================================================
	_CreateStaticBuffers
=================================================
*/
	bool  VStagingBufferManager::_CreateStaticBuffers (const GraphicsCreateInfo &info)
	{
		auto&	dev = _resMngr.GetDevice();
		
		_writeStaticSize	= info.staging.writeStaticSize;
		_readStaticSize		= info.staging.readStaticSize;

		_staticBuffersForWrite.resize( info.maxFrames * QueueCount );
		_staticBuffersForRead.resize( info.maxFrames * QueueCount );

		VkBufferCreateInfo		buf_info = {};
		buf_info.sType			= VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		buf_info.flags			= 0;
		buf_info.usage			= VEnumCast( EBufferUsage::Transfer );
		buf_info.size			= 1 << 10;
		buf_info.sharingMode	= VK_SHARING_MODE_EXCLUSIVE;

		VkMemoryRequirements	mem_req = {};
		{
			VkBuffer	buf = VK_NULL_HANDLE;
			VK_CHECK( dev.vkCreateBuffer( dev.GetVkDevice(), &buf_info, null, OUT &buf ));

			dev.vkGetBufferMemoryRequirements( dev.GetVkDevice(), buf, &mem_req );
			dev.vkDestroyBuffer( dev.GetVkDevice(), buf, null );
		}

		Bytes	total_write_size;
		Bytes	total_read_size;
		void*	mapped_write_mem	= null;
		void*	mapped_read_mem		= null;
		{
			for (uint i = 0; i < QueueCount; ++i)
			{
				total_write_size = AlignToLarger( total_write_size + _writeStaticSize[i], mem_req.alignment );
				total_read_size  = AlignToLarger( total_read_size  + _readStaticSize[i],  mem_req.alignment );
			}
			total_write_size *= info.maxFrames;
			total_read_size  *= info.maxFrames;

			VkMemoryAllocateInfo	alloc_info = {};
			alloc_info.sType			= VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			alloc_info.allocationSize	= VkDeviceSize( total_write_size );
			CHECK_ERR( dev.GetMemoryTypeIndex( mem_req.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, Zero,
											   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, OUT alloc_info.memoryTypeIndex ));
			VK_CHECK( dev.vkAllocateMemory( dev.GetVkDevice(), &alloc_info, null, OUT &_staticMemoryForWrite ));
			VK_CHECK( dev.vkMapMemory( dev.GetVkDevice(), _staticMemoryForWrite, 0, alloc_info.allocationSize, 0, OUT &mapped_write_mem ));
			_staticMemoryFlagsForWrite = VkMemoryPropertyFlagBits( dev.GetProperties().memoryProperties.memoryTypes[alloc_info.memoryTypeIndex].propertyFlags );
			
			alloc_info.allocationSize	= VkDeviceSize( total_read_size );
			CHECK_ERR( dev.GetMemoryTypeIndex( mem_req.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_MEMORY_PROPERTY_HOST_CACHED_BIT,
											   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, OUT alloc_info.memoryTypeIndex ));
			VK_CHECK( dev.vkAllocateMemory( dev.GetVkDevice(), &alloc_info, null, OUT &_staticMemoryForRead ));
			VK_CHECK( dev.vkMapMemory( dev.GetVkDevice(), _staticMemoryForRead, 0, alloc_info.allocationSize, 0, OUT &mapped_read_mem ));
			_staticMemoryFlagsForRead = VkMemoryPropertyFlagBits( dev.GetProperties().memoryProperties.memoryTypes[alloc_info.memoryTypeIndex].propertyFlags );
		}

		const auto	CreateStaticBuffer = [this, &dev, &buf_info, &mem_req] (StaticBuffer& sb, Bytes& offset, Bytes size, VkDeviceMemory mem, void* mapped, usize idx, const char* name) -> bool
		{
			if ( size == 0 )
				return true;

			VkBuffer	buf = VK_NULL_HANDLE;
			buf_info.size = VkDeviceSize(size);
			VK_CHECK( dev.vkCreateBuffer( dev.GetVkDevice(), &buf_info, null, OUT &buf ));

			offset = AlignToLarger( offset, mem_req.alignment );
			VK_CHECK( dev.vkBindBufferMemory( dev.GetVkDevice(), buf, mem, VkDeviceSize(offset) ));

			VulkanBufferDesc	desc;
			desc.buffer			= buf;
			desc.size			= size;
			desc.usage			= VkBufferUsageFlagBits( buf_info.usage );
			desc.canBeDestroyed	= true;

			sb.buffer = _resMngr.CreateBuffer( desc, String{name} << " [f:" << ToString(idx / QueueCount) << "] [q:" << ToString(EQueueType(idx % QueueCount)) << "]" );
			CHECK_ERR( sb.buffer );

			sb.bufferHandle = _resMngr.GetResource( sb.buffer )->Handle();
			sb.size.store( 0, EMemoryOrder::Relaxed );
			sb.capacity		= CheckCast<uint>( size );
			sb.memOffset	= CheckCast<uint>( offset );
			sb.mapped		= mapped + offset;

			offset += size;
			return true;
		};
		
		Bytes	write_mem_offset;
		Bytes	read_mem_offset;

		for (usize i = 0; i < _staticBuffersForWrite.size(); ++i)
		{
			CHECK_ERR( CreateStaticBuffer( _staticBuffersForWrite[i], write_mem_offset, _writeStaticSize[i % QueueCount], _staticMemoryForWrite, mapped_write_mem, i, "SSWB" ));
			CHECK_ERR( CreateStaticBuffer( _staticBuffersForRead[i],  read_mem_offset,  _readStaticSize[i % QueueCount],  _staticMemoryForRead,  mapped_read_mem,  i, "SSRB" ));
		}
		
		ASSERT( total_write_size == write_mem_offset );
		ASSERT( total_read_size  == read_mem_offset  );

		return true;
	}


}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
