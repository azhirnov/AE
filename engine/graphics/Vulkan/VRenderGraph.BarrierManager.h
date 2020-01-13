// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

namespace AE::Graphics
{

	//
	// Barrier Manager
	//

	class VRenderGraph::BarrierManager
	{
	// types
	private:
		struct MemRange
		{
			VkDeviceMemory		handle	= VK_NULL_HANDLE;
			VkDeviceSize		offset	= 0;
			VkDeviceSize		size	= 0;

			ND_ bool  operator == (const MemRange &) const;
		};

		struct MemRangeHash {
			ND_ size_t  operator () (const MemRange &) const;
		};

		// TODO: custom allocator
		using ImageMemoryBarriers_t		= Array< VkImageMemoryBarrier >;
		using BufferMemoryBarriers_t	= Array< VkBufferMemoryBarrier >;
		using MemoryRanges_t			= HashSet< MemRange, MemRangeHash >;	// TODO: flat map


	// variables
	private:
		ImageMemoryBarriers_t		_imageBarriers;
		BufferMemoryBarriers_t		_bufferBarriers;
		//VkMemoryBarrier			_memoryBarrier;

		VkPipelineStageFlags		_srcStageMask		= 0;
		VkPipelineStageFlags		_dstStageMask		= 0;
		VkDependencyFlags			_dependencyFlags	= 0;

		MemoryRanges_t				_flushMemRanges;
		MemoryRanges_t				_invalidateMemRanges;


	// methods
	public:
		BarrierManager ();

		// device cache & sync
		void Commit (const VDevice &dev, VkCommandBuffer cmd);
		void ClearBarriers ();

		void AddBufferBarrier (VkPipelineStageFlags			srcStageMask,
							   VkPipelineStageFlags			dstStageMask,
							   const VkBufferMemoryBarrier	&barrier);
		
		void AddImageBarrier (VkPipelineStageFlags			srcStageMask,
							  VkPipelineStageFlags			dstStageMask,
							  VkDependencyFlags				dependencyFlags,
							  const VkImageMemoryBarrier	&barrier);


		// host cache control
		void FlushMemory (VkDeviceMemory mem, VkDeviceSize offset, VkDeviceSize size);
		void InvalidateMemory (VkDeviceMemory mem, VkDeviceSize offset, VkDeviceSize size);

		void FlushMemoryRanges (const VDevice &dev);
		void InvalidateMemoryRanges (const VDevice &dev);
	};
//-----------------------------------------------------------------------------

	
	
	inline bool  VRenderGraph::BarrierManager::MemRange::operator == (const MemRange &rhs) const
	{
		return (handle == rhs.handle) & (offset == rhs.offset) & (size == rhs.size);
	}

	inline size_t  VRenderGraph::BarrierManager::MemRangeHash::operator () (const MemRange &value) const
	{
		return size_t(HashOf( value.handle ) + HashOf( value.offset ) + HashOf( value.size ));
	}
//-----------------------------------------------------------------------------



/*
=================================================
	constructor
=================================================
*/
	VRenderGraph::BarrierManager::BarrierManager ()
	{
		_imageBarriers.reserve( 32 );
		_bufferBarriers.reserve( 64 );
	}
	
/*
=================================================
	Commit
=================================================
*/
	void VRenderGraph::BarrierManager::Commit (const VDevice &dev, VkCommandBuffer cmd)
	{
		if ( _bufferBarriers.size() | _imageBarriers.size() )
		{
			dev.vkCmdPipelineBarrier( cmd, _srcStageMask, _dstStageMask, _dependencyFlags,
										0, null,
										uint(_bufferBarriers.size()), _bufferBarriers.data(),
										uint(_imageBarriers.size()), _imageBarriers.data() );
			ClearBarriers();
		}
	}
	
/*
=================================================
	ClearBarriers
=================================================
*/
	void VRenderGraph::BarrierManager::ClearBarriers ()
	{
		_imageBarriers.clear();
		_bufferBarriers.clear();

		_srcStageMask = _dstStageMask = 0;
		_dependencyFlags = 0;
	}
	
/*
=================================================
	AddBufferBarrier
=================================================
*/
	void VRenderGraph::BarrierManager::AddBufferBarrier (VkPipelineStageFlags			srcStageMask,
														VkPipelineStageFlags			dstStageMask,
														const VkBufferMemoryBarrier	&barrier)
	{
		_srcStageMask |= srcStageMask;
		_dstStageMask |= dstStageMask;

		_bufferBarriers.push_back( barrier );
	}
		
/*
=================================================
	AddImageBarrier
=================================================
*/
	void VRenderGraph::BarrierManager::AddImageBarrier (VkPipelineStageFlags			srcStageMask,
														VkPipelineStageFlags			dstStageMask,
														VkDependencyFlags				dependencyFlags,
														const VkImageMemoryBarrier	&barrier)
	{
		_srcStageMask		|= srcStageMask;
		_dstStageMask		|= dstStageMask;
		_dependencyFlags	|= dependencyFlags;

		_imageBarriers.push_back( barrier );
	}
	
/*
=================================================
	FlushMemory
=================================================
*/
	void VRenderGraph::BarrierManager::FlushMemory (VkDeviceMemory mem, VkDeviceSize offset, VkDeviceSize size)
	{
		_flushMemRanges.insert(MemRange{ mem, offset, size });
	}
	
/*
=================================================
	InvalidateMemory
=================================================
*/
	void VRenderGraph::BarrierManager::InvalidateMemory (VkDeviceMemory mem, VkDeviceSize offset, VkDeviceSize size)
	{
		_invalidateMemRanges.insert(MemRange{ mem, offset, size });
	}
	
/*
=================================================
	FlushMemoryRanges
=================================================
*/
	void VRenderGraph::BarrierManager::FlushMemoryRanges (const VDevice &dev)
	{
		for (auto& range : _flushMemRanges)
		{
			VkMappedMemoryRange	mem_range = {};
			mem_range.sType		= VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			mem_range.memory	= range.handle;
			mem_range.offset	= range.offset;
			mem_range.size		= range.size;

			VK_CALL( dev.vkFlushMappedMemoryRanges( dev.GetVkDevice(), 1, &mem_range ));
		}
		_flushMemRanges.clear();
	}
	
/*
=================================================
	InvalidateMemoryRanges
=================================================
*/
	void VRenderGraph::BarrierManager::InvalidateMemoryRanges (const VDevice &dev)
	{
		for (auto& range : _invalidateMemRanges)
		{
			VkMappedMemoryRange	mem_range = {};
			mem_range.sType		= VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			mem_range.memory	= range.handle;
			mem_range.offset	= range.offset;
			mem_range.size		= range.size;

			VK_CALL( dev.vkInvalidateMappedMemoryRanges( dev.GetVkDevice(), 1, &mem_range ));
		}
		_invalidateMemRanges.clear();
	}


}	// AE::Graphics
