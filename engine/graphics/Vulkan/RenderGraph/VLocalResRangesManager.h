// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef AE_ENABLE_VULKAN

# include "stl/Containers/StructView.h"

# include "graphics/Private/ResourceDataRange.h"
# include "graphics/Private/ImageDataRange.h"

# include "graphics/Vulkan/VCommon.h"
# include "graphics/Private/EnumUtils.h"
# include "graphics/Vulkan/VResourceManager.h"
# include "graphics/Vulkan/VRenderGraph.h"
# include "graphics/Vulkan/VEnumCast.h"
# include "graphics/Vulkan/RenderGraph/VBarrierManager.h"

namespace AE::Graphics
{

	//
	// Vulkan Local Resources Manager with taking into accound to ranges
	//

	class VLocalResRangesManager : public VBarrierManager
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
		explicit VLocalResRangesManager (const VCommandBatch& batch);
		~VLocalResRangesManager ();

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
		
		void  _AddImage (const LocalImage *img, EResourceState state, VkImageLayout layout, const ImageViewDesc &desc);
	};
	
	

	//
	// Vulkan Local Buffer
	//

	class VLocalResRangesManager::LocalBuffer
	{
	// types
	public:
		using BufferRange = ResourceDataRange< VkDeviceSize >;
		
		struct BufferState
		{
		// variables
			EResourceState	state		= Default;
			BufferRange		range;
			uint			cmdIndex	= 0;

		// methods
			BufferState () {}

			BufferState (EResourceState state, VkDeviceSize begin, VkDeviceSize end, uint cmdIndex) :
				state{state}, range{begin, end}, cmdIndex{cmdIndex} {}
		};

	private:
		struct BufferAccess
		{
		// variables
			BufferRange				range;
			VkPipelineStageFlagBits	stages		= Zero;
			VkAccessFlagBits		access		= Zero;
			uint					cmdIndex	= 0;
			bool					isReadable : 1;
			bool					isWritable : 1;

		// methods
			BufferAccess () : isReadable{false}, isWritable{false} {}
		};

		using AccessRecords_t	= Array< BufferAccess >;	// TODO: fixed size array or custom allocator
		using AccessIter_t		= AccessRecords_t::iterator;


	// variables
	private:
		Ptr<VBuffer const>			_bufferData;	// readonly access is thread safe

		mutable AccessRecords_t		_pendingAccesses;
		mutable AccessRecords_t		_accessForWrite;
		mutable AccessRecords_t		_accessForRead;
		mutable bool				_isMutable		= true;


	// methods
	public:
		LocalBuffer () {}
		LocalBuffer (LocalBuffer &&) = delete;
		LocalBuffer (const LocalBuffer &) = delete;
		~LocalBuffer ();

		bool  Create (const VBuffer *);
		void  Destroy (VBarrierManager &);
		bool  GetMemoryInfo (OUT VulkanMemoryObjInfo &) const;
		
		void  SetInitialState (EResourceState state) const;
		void  AddPendingState (const BufferState &state) const;
		void  CommitBarrier (VBarrierManager &barrierMngr) const;
		
		ND_ bool				IsMutable ()	const	{ return _isMutable; }
		ND_ VkBuffer			Handle ()		const	{ return _bufferData->Handle(); }
		ND_ VBuffer const*		ToGlobal ()		const	{ return _bufferData.get(); }

		ND_ BufferDesc const&	Description ()	const	{ return _bufferData->Description(); }
		ND_ Bytes				Size ()			const	{ return Description().size; }


	private:
		ND_ static AccessIter_t	_FindFirstAccess (AccessRecords_t &arr, const BufferRange &range);
			static void			_ReplaceAccessRecords (INOUT AccessRecords_t &arr, AccessIter_t iter, const BufferAccess &barrier);
			static AccessIter_t	_EraseAccessRecords (INOUT AccessRecords_t &arr, AccessIter_t iter, const BufferRange &range);
	};

	

	//
	// Vulkan Local Image
	//
	class VLocalResRangesManager::LocalImage
	{
	// types
	public:
		using ImageRange	= ImageDataRange;

		struct ImageState
		{
			EResourceState			state		= Default;
			VkImageLayout			layout		= Zero;
			VkImageAspectFlagBits	aspect		= Zero;
			ImageRange				range;
			uint					cmdIndex	= 0;
			
			ImageState () {}

			ImageState (EResourceState state, VkImageLayout layout, const ImageRange &range, VkImageAspectFlagBits aspect, uint cmdIndex) :
				state{state}, layout{layout}, aspect{aspect}, range{range}, cmdIndex{cmdIndex} {}
		};

	private:
		using SubRange	= ImageRange::SubRange_t;

		struct ImageAccess
		{
			SubRange				range;
			VkImageLayout			layout			= Zero;
			VkPipelineStageFlagBits	stages			= Zero;
			VkAccessFlagBits		access			= Zero;
			uint					cmdIndex		= 0;
			bool					isReadable		: 1;
			bool					isWritable		: 1;
			bool					invalidateBefore : 1;
			bool					invalidateAfter	 : 1;

			ImageAccess () : isReadable{false}, isWritable{false}, invalidateBefore{false}, invalidateAfter{false} {}
		};

		using ImageViewMap_t	= VImage::ImageViewMap_t;
		using AccessRecords_t	= Array< ImageAccess >;		// TODO: fixed size array or custom allocator
		using AccessIter_t		= AccessRecords_t::iterator;

		
	// variables
	private:
		Ptr<VImage const>			_imageData;		// readonly access is thread safe
		VkImageLayout				_finalLayout	= VK_IMAGE_LAYOUT_GENERAL;

		mutable AccessRecords_t		_pendingAccesses;
		mutable AccessRecords_t		_accessForReadWrite;
		mutable bool				_isMutable		= true;


	// methods
	public:
		LocalImage () {}
		LocalImage (LocalImage &&) = delete;
		LocalImage (const LocalImage &) = delete;
		~LocalImage ();

		bool  Create (const VImage *);
		void  Destroy (VBarrierManager &);
		
		void  SetInitialState (EResourceState state) const;
		void  SetInitialState (EResourceState state, EImageLayout layout) const;
		void  AddPendingState (const ImageState &) const;
		void  CommitBarrier (VBarrierManager &barrierMngr) const;
		
		ND_ bool				IsMutable ()		const	{ return _isMutable; }
		ND_ VkImage				Handle ()			const	{ return _imageData->Handle(); }
		ND_ VImage const*		ToGlobal ()			const	{ return _imageData.get(); }

		ND_ ImageDesc const&	Description ()		const	{ return _imageData->Description(); }
		ND_ VkImageAspectFlags	AspectMask ()		const	{ return _imageData->AspectMask(); }
		ND_ uint3 const&		Dimension ()		const	{ return Description().dimension; }
		ND_ uint const			ArrayLayers ()		const	{ return Description().arrayLayers.Get(); }
		ND_ uint const			MipmapLevels ()		const	{ return Description().maxLevel.Get(); }


	private:
		ND_ static AccessIter_t	_FindFirstAccess (AccessRecords_t &arr, const SubRange &range);
			static void			_ReplaceAccessRecords (INOUT AccessRecords_t &arr, AccessIter_t iter, const ImageAccess &barrier);

	};
	
//-----------------------------------------------------------------------------



/*
=================================================
	Get
=================================================
*/
	template <typename ID>
	inline auto*  VLocalResRangesManager::Get (ID id)
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
	inline Res const*  VLocalResRangesManager::_ToLocal (ID id, INOUT LocalResPool<Res,MainPool,MC> &localRes, StringView msg)
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
