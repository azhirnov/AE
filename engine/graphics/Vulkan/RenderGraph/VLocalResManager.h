// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef AE_ENABLE_VULKAN

# include "stl/Containers/StructView.h"
# include "graphics/Vulkan/VCommon.h"
# include "graphics/Private/EnumUtils.h"
# include "graphics/Vulkan/VResourceManager.h"
# include "graphics/Vulkan/VRenderGraph.h"
# include "graphics/Vulkan/VEnumCast.h"
# include "graphics/Vulkan/RenderGraph/VBarrierManager.h"

namespace AE::Graphics
{

	//
	// Vulkan Local Resources Manager
	//

	class VLocalResManager : public VBarrierManager
	{
	// types
	public:
		class LocalImage;
		class LocalBuffer;
		
		static constexpr bool	HasAutoSync = true;

	private:
		using Index_t		= VResourceManagerImpl::Index_t;
		using Generation_t	= VResourceManagerImpl::Generation_t;
		
		template <typename T, usize CS, usize MC>
		using PoolTmpl		= Threading::IndexedPool< VResourceBase<T>, Index_t, CS, MC >;
		
		template <typename Res, typename MainPool, usize MC>
		struct LocalResPool
		{
			PoolTmpl< Res, MainPool::capacity()/MC, MC >	pool;
			HashMap< Index_t, Index_t >						toLocal;
		};

		using LocalImages_t		= LocalResPool< LocalImage,		VResourceManagerImpl::ImagePool_t,	16 >;
		using LocalBuffers_t	= LocalResPool< LocalBuffer,	VResourceManagerImpl::BufferPool_t,	16 >;
		
		using BufferBarriers_t	= HashSet< LocalBuffer const *>;	// TODO: optimize
		using ImageBarriers_t	= HashSet< LocalImage const *>;

		class DescriptorSetBarriers;


	// variables
	private:
		VResourceManagerImpl &		_resMngr;
		uint						_cmdCounter		= 1;
		
		struct {
			BufferBarriers_t			buffers;
			ImageBarriers_t				images;
		}							_pendingBarriers;
		struct {
			LocalBuffers_t				buffers;
			LocalImages_t				images;
		}							_localRes;


	// methods
	public:
		explicit VLocalResManager (const VCommandBatch& batch);
		~VLocalResManager ();

		// api
		template <typename ID>
		ND_ auto*  Get (ID id);

		void  FlushBarriers ();
		void  TransitToDefaultState ();
		
		ND_ VDevice const&			GetDevice ()			const	{ return _resMngr.GetDevice(); }
		ND_ VStagingBufferManager&	GetStagingManager ()	const	{ return _resMngr.GetStagingManager(); }
		
		void  AddBuffer (const LocalBuffer *buf, EResourceState state, Bytes offset, Bytes size);
		void  AddBuffer (const LocalBuffer *buf, EResourceState state, ArrayView<BufferImageCopy> copy, const LocalImage* img);

		void  AddImage (const LocalImage *img, EResourceState state, VkImageLayout layout, StructView<ImageSubresourceRange> subres);
		void  AddImage (const LocalImage *img, EResourceState state, VkImageLayout layout, StructView<ImageSubresourceLayers> subresLayers);
		
		void  DescriptorsBarrier (DescriptorSetID ds, ArrayView<uint> dynamicOffsets);


	private:
		template <typename ID, typename ResPool>
		void  _FlushLocalResourceStates (ResPool &);
		
		template <typename ID, typename Res, typename MainPool, usize MC>
		ND_ Res const*  _ToLocal (ID id, INOUT LocalResPool<Res,MainPool,MC> &localRes, StringView msg);
		
		void  _AddBuffer (const LocalBuffer *buf, EResourceState state);
		void  _AddImage (const LocalImage *img, EResourceState state, VkImageLayout layout);
	};
	


	//
	// Vulkan Local Buffer
	//

	class VLocalResManager::LocalBuffer
	{
	// types
	private:
		struct PendingAccess
		{
			VkPipelineStageFlagBits	stages		= Zero;
			VkAccessFlagBits		access		= Zero;
			uint					cmdIndex	= 0;
			bool					isReadable	: 1;
			bool					isWritable	: 1;

			PendingAccess () : isReadable{false}, isWritable{false} {}
		};

		struct CurrentAccess
		{
			VkPipelineStageFlagBits	writeStages		= Zero;
			VkAccessFlagBits		writeAccess		= Zero;
			VkPipelineStageFlagBits	readStages		= VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			VkAccessFlagBits		readAccess		= Zero;
			uint					cmdIndex		= 0;
			VkAccessFlagBits		unavailable		= Zero;		// access flags that must be invalidated before reading
		};


	// variables
	private:
		Ptr<VBuffer const>		_bufferData;	// readonly access is thread safe
		mutable PendingAccess	_pendingAccess;
		mutable CurrentAccess	_currentState;
		mutable bool			_isMutable	= true;


	// methods
	public:
		LocalBuffer () {}
		~LocalBuffer ();

		bool  Create (const VBuffer *buf);
		void  Destroy (VBarrierManager &);
		bool  GetMemoryInfo (OUT VulkanMemoryObjInfo &) const;
		
		void  SetInitialState (EResourceState state) const;
		void  AddPendingState (EResourceState state, uint cmdIndex) const;
		void  CommitBarrier (VBarrierManager &) const;
		
		ND_ bool				IsMutable ()	const	{ return _isMutable; }
		ND_ VkBuffer			Handle ()		const	{ return _bufferData->Handle(); }
		ND_ VBuffer const*		ToGlobal ()		const	{ return _bufferData.get(); }
		ND_ Bytes				Size ()			const	{ return _bufferData->Description().size; }
	};



	//
	// Vulkan Local Image
	//

	class VLocalResManager::LocalImage
	{
	// types
	private:
		struct PendingAccess
		{
			VkPipelineStageFlagBits	stages		= Zero;
			VkAccessFlagBits		access		= Zero;
			VkImageLayout			layout		= Zero;
			uint					cmdIndex	= 0;
			bool					isReadable	: 1;
			bool					isWritable	: 1;

			PendingAccess () : isReadable{false}, isWritable{false} {}
		};

		struct CurrentAccess
		{
			VkPipelineStageFlagBits	writeStages		= Zero;
			VkAccessFlagBits		writeAccess		= Zero;
			VkPipelineStageFlagBits	readStages		= VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			VkAccessFlagBits		readAccess		= Zero;
			VkImageLayout			layout			= VK_IMAGE_LAYOUT_UNDEFINED;
			uint					cmdIndex		= 0;
			VkAccessFlagBits		unavailable		= Zero;		// access flags that must be invalidated before reading
		};


	// variables
	private:
		Ptr<VImage const>		_imageData;	// readonly access is thread safe
		mutable PendingAccess	_pendingAccess;
		mutable CurrentAccess	_currentState;
		mutable bool			_isMutable	= true;


	// methods
	public:
		LocalImage () {}
		~LocalImage ();

		bool  Create (const VImage *);
		void  Destroy (VBarrierManager &);
		
		void  SetInitialState (EResourceState state) const;
		void  SetInitialState (EResourceState state, EImageLayout layout) const;
		void  AddPendingState (EResourceState state, VkImageLayout layout, uint cmdIndex) const;
		void  CommitBarrier (VBarrierManager &) const;

		ND_ bool				IsMutable ()	const	{ return true; } // TODO
		ND_ VkImage				Handle ()		const	{ return _imageData->Handle(); }
		ND_ VImage const*		ToGlobal ()		const	{ return _imageData.get(); }

		ND_ ImageDesc const&	Description ()	const	{ return _imageData->Description(); }
		ND_ uint3 const			Dimension ()	const	{ return _imageData->Description().dimension; }
		ND_ VkImageAspectFlags	AspectMask ()	const	{ return _imageData->AspectMask(); }
	};

//-----------------------------------------------------------------------------


	
/*
=================================================
	AddPendingState
=================================================
*/
	inline void  VLocalResManager::LocalImage::AddPendingState (EResourceState state, VkImageLayout layout, uint cmdIndex) const
	{
		ASSERT( _isMutable );
		CHECK( _pendingAccess.layout == layout or _pendingAccess.layout == VK_IMAGE_LAYOUT_UNDEFINED );

		_pendingAccess.stages		|= EResourceState_ToPipelineStages( state );
		_pendingAccess.access		|= EResourceState_ToAccess( state );
		_pendingAccess.layout		 = layout;
		_pendingAccess.isReadable	|= EResourceState_IsReadable( state );
		_pendingAccess.isWritable	|= EResourceState_IsWritable( state );
		_pendingAccess.cmdIndex		 = Max( _pendingAccess.cmdIndex, cmdIndex );
	}
//-----------------------------------------------------------------------------


	
/*
=================================================
	AddPendingState
=================================================
*/
	inline void  VLocalResManager::LocalBuffer::AddPendingState (EResourceState state, uint cmdIndex) const
	{
		ASSERT( _isMutable );
		
		_pendingAccess.stages		|= EResourceState_ToPipelineStages( state );
		_pendingAccess.access		|= EResourceState_ToAccess( state );
		_pendingAccess.isReadable	|= EResourceState_IsReadable( state );
		_pendingAccess.isWritable	|= EResourceState_IsWritable( state );
		_pendingAccess.cmdIndex		 = Max( _pendingAccess.cmdIndex, cmdIndex );
	}
//-----------------------------------------------------------------------------


	
/*
=================================================
	Get
=================================================
*/
	template <typename ID>
	inline auto*  VLocalResManager::Get (ID id)
	{
		if constexpr( IsSameTypes< ID, BufferID >)
			return _ToLocal( id, _localRes.buffers, "failed when creating local buffer" );
		else
		if constexpr( IsSameTypes< ID, ImageID >)
			return _ToLocal( id, _localRes.images, "failed when creating local image" );
		else
			return _resMngr.GetResource( id );
	}

/*
=================================================
	_ToLocal
=================================================
*/
	template <typename ID, typename Res, typename MainPool, usize MC>
	inline Res const*  VLocalResManager::_ToLocal (ID id, INOUT LocalResPool<Res,MainPool,MC> &localRes, StringView msg)
	{
		auto[iter, inserted] = localRes.toLocal.insert({ id.Index(), Index_t(UMax) });
		Index_t&	local	 = iter->second;

		// if already exists
		if ( not inserted )
		{
			Res const*  result = &(localRes.pool[ local ].Data());
			ASSERT( result->ToGlobal() );
			return result;
		}

		auto*	res  = _resMngr.GetResource( id /*, _resources.Add( id )*/);
		if ( not res )
			return null;

		CHECK_ERR( localRes.pool.Assign( OUT local ));

		auto&	data = localRes.pool[ local ];
		
		data.Data().~Res();
		PlacementNew<Res>( &data.Data() );

		if ( not data.Create( res ))
		{
			localRes.pool.Unassign( local );
			RETURN_ERR( msg );
		}

		return &(data.Data());
	}


}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
