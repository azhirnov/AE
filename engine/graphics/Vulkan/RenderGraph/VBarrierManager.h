// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef AE_ENABLE_VULKAN

# include "graphics/Vulkan/VCommon.h"

namespace AE::Graphics
{

	//
	// Vulkan Pipeline Barriers Manager
	//

	class VBarrierManager
	{
	// types
	public:
		struct PipelineBarrier
		{
			VkPipelineStageFlagBits			srcStageMask;
			VkPipelineStageFlagBits			dstStageMask;
			VkMemoryBarrier const *			memoryBarriers;
			VkBufferMemoryBarrier const *	bufferBarriers;
			VkImageMemoryBarrier const *	imageBarriers;
			uint							memoryBarrierCount;
			uint							bufferBarrierCount;
			uint							imageBarrierCount;
		};


	private:
		// TODO: custom allocator
		using ImageMemoryBarriers_t		= Array< VkImageMemoryBarrier >;
		using BufferMemoryBarriers_t	= Array< VkBufferMemoryBarrier >;


	// variables
	private:
		VkPipelineStageFlagBits		_srcStageMask		= Zero;
		VkPipelineStageFlagBits		_dstStageMask		= Zero;

		ImageMemoryBarriers_t		_imageBarriers;
		BufferMemoryBarriers_t		_bufferBarriers;
		VkMemoryBarrier				_memoryBarrier		= {};

		//MemoryRanges_t			_flushMemRanges;


	// methods
	public:
		explicit VBarrierManager (const VCommandBatch &);
		~VBarrierManager ();

		ND_ bool  GetBarriers (OUT PipelineBarrier &) const;

		void  ClearBarriers ();

		void  AddBufferBarrier (VkPipelineStageFlagBits		srcStageMask,
								VkPipelineStageFlagBits		dstStageMask,
								const VkBufferMemoryBarrier	&barrier);
		
		void  AddImageBarrier (VkPipelineStageFlagBits		srcStageMask,
							   VkPipelineStageFlagBits		dstStageMask,
							   const VkImageMemoryBarrier	&barrier);

		void  AddMemoryBarrier (VkPipelineStageFlagBits		srcStageMask,
								VkPipelineStageFlagBits		dstStageMask,
								const VkMemoryBarrier		&barrier);
	};

	
	
/*
=================================================
	constructor
=================================================
*/
	inline VBarrierManager::VBarrierManager (const VCommandBatch &)
	{
		_imageBarriers.reserve( 32 );
		_bufferBarriers.reserve( 64 );
		_memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
	}
	
/*
=================================================
	destructor
=================================================
*/
	inline VBarrierManager::~VBarrierManager ()
	{
		ASSERT( _imageBarriers.empty() );
		ASSERT( _bufferBarriers.empty() );
	}

/*
=================================================
	GetBarriers
=================================================
*/
	inline bool  VBarrierManager::GetBarriers (OUT PipelineBarrier &result) const
	{
		const uint	mem_count = !!(_memoryBarrier.srcAccessMask | _memoryBarrier.dstAccessMask);

		if ( mem_count | _imageBarriers.size() | _bufferBarriers.size() )
		{
			result.srcStageMask			= _srcStageMask;
			result.dstStageMask			= _dstStageMask;
			result.bufferBarriers		= _bufferBarriers.data();
			result.bufferBarrierCount	= uint(_bufferBarriers.size());
			result.imageBarriers		= _imageBarriers.data();
			result.imageBarrierCount	= uint(_imageBarriers.size());
			result.memoryBarriers		= mem_count ? &_memoryBarrier : null;
			result.memoryBarrierCount	= mem_count;
			return true;
		}
		return false;
	}

/*
=================================================
	ClearBarriers
=================================================
*/
	inline void  VBarrierManager::ClearBarriers ()
	{
		_imageBarriers.clear();
		_bufferBarriers.clear();

		_memoryBarrier.srcAccessMask = _memoryBarrier.dstAccessMask = 0;

		_srcStageMask = _dstStageMask = Zero;
	}
	
/*
=================================================
	AddBufferBarrier
=================================================
*/
	inline void  VBarrierManager::AddBufferBarrier (VkPipelineStageFlagBits		srcStageMask,
													VkPipelineStageFlagBits		dstStageMask,
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
	inline void  VBarrierManager::AddImageBarrier (VkPipelineStageFlagBits		srcStageMask,
												   VkPipelineStageFlagBits		dstStageMask,
												   const VkImageMemoryBarrier	&barrier)
	{
		_srcStageMask |= srcStageMask;
		_dstStageMask |= dstStageMask;

		_imageBarriers.push_back( barrier );
	}
	
/*
=================================================
	AddMemoryBarrier
=================================================
*/
	inline void  VBarrierManager::AddMemoryBarrier (VkPipelineStageFlagBits		srcStageMask,
													VkPipelineStageFlagBits		dstStageMask,
													const VkMemoryBarrier		&barrier)
	{
		ASSERT( barrier.pNext == null );
		_srcStageMask				 |= srcStageMask;
		_dstStageMask				 |= dstStageMask;
		_memoryBarrier.srcAccessMask |= barrier.srcAccessMask;
		_memoryBarrier.dstAccessMask |= barrier.dstAccessMask;
	}


}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
